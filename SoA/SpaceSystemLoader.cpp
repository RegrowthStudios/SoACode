#include "stdafx.h"
#include "SpaceSystemLoader.h"

#include "Constants.h"
#include "Errors.h"
#include "SoaOptions.h"
#include "SpaceBodyComponentUpdater.h"
#include "PlanetGenData.h"
#include "PlanetGenLoader.h"
#include "ProgramGenDelegate.h"
#include "SoaState.h"
#include "SpaceSystemAssemblages.h"
#include "SpaceSystemLoadStructs.h"

#include <Vorb/RPC.h>
#include <Vorb/graphics/GpuMemory.h>
#include <Vorb/graphics/ImageIO.h>
#include <Vorb/graphics/Texture.h>
#include <Vorb/io/keg.h>
#include <Vorb/ui/GameWindow.h>


/************************************************************************/
/* File Path Constants                                                  */
/************************************************************************/

#define FILE_SS_TREES             "trees.yml";
#define FILE_SS_FLORA             "flora.yml";
#define FILE_SS_TERRAIN_GEN       "terrain_gen.yml";
#define FILE_SS_PATH_COLORS       "path_colors.yml";
#define FILE_SS_SYSTEM_PROPERTIES "system_properties.yml";
#define FILE_SS_VERSION           "version.dat";
#define FILE_SS_BODY_PROPERTIES   "properties.yml"

/************************************************************************/

void SpaceSystemLoader::init(const SoaState* soaState) {
    // TODO(Ben): Don't need all this
    m_soaState = soaState;
    m_spaceSystem = soaState->spaceSystem;
    m_ioManager = soaState->systemIoManager;
    m_threadpool = soaState->threadPool;
    m_bodyLoader.init(m_ioManager);
}

void SpaceSystemLoader::loadStarSystemProperties(const nString& path) {
    m_ioManager->setSearchDirectory((path + "/").c_str());

    // Load the path color scheme
    //loadPathColors(); // This is rendering.

    // Load the system properties
    loadSystemProperties();

    // Set up binary masses
    initBarycenters();

    // Set up parent connections and orbits
    initOrbits();

    // Set up component system
    initComponents();

    // Free excess memory
    std::set<SystemBodyProperties*>().swap(m_calculatedOrbits);
}

// Only used in SoaEngine::loadPathColors
struct PathColorKegProps {
    ui8v4 base = ui8v4(0);
    ui8v4 hover = ui8v4(0);
};
KEG_TYPE_DEF_SAME_NAME(PathColorKegProps, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, PathColorKegProps, base, UI8_V4);
    KEG_TYPE_INIT_ADD_MEMBER(kt, PathColorKegProps, hover, UI8_V4);
}

//bool SpaceSystemLoader::loadPathColors() {
//    nString data;
//    if (!m_ioManager->readFileToString("PathColors.yml", data)) {
//        pError("Couldn't find " + m_ioManager->getSearchDirectory().getString() + "/PathColors.yml");
//    }
//
//    keg::ReadContext context;
//    context.env = keg::getGlobalEnvironment();
//    context.reader.init(data.c_str());
//    keg::Node node = context.reader.getFirst();
//    if (keg::getType(node) != keg::NodeType::MAP) {
//        fprintf(stderr, "Failed to load %s\n", (m_ioManager->getSearchDirectory().getString() + "/PathColors.yml").c_str());
//        context.reader.dispose();
//        return false;
//    }
//
//    bool goodParse = true;
//    auto f = makeFunctor([&](Sender, const nString& name, keg::Node value) {
//        PathColorKegProps props;
//        keg::Error err = keg::parse((ui8*)&props, value, context, &KEG_GLOBAL_TYPE(PathColorKegProps));
//        if (err != keg::Error::NONE) {
//            fprintf(stderr, "Failed to parse node %s in PathColors.yml\n", name.c_str());
//            goodParse = false;
//        }
//        f32v4 base = f32v4(props.base) / 255.0f;
//        f32v4 hover = f32v4(props.hover) / 255.0f;
//        m_spaceSystem->pathColorMap[name] = std::make_pair(base, hover);
//    });
//
//    context.reader.forAllInMap(node, f);
//    delete f;
//    context.reader.dispose();
//    return goodParse;
//}

bool SpaceSystemLoader::loadSystemProperties() {
    // Read in system properties file
    nString data;
    if (!m_ioManager->readFileToString("system_properties.yml", data)) {
        pError("Couldn't find " + m_ioManager->getSearchDirectory().getString() + "/SystemProperties.yml");
    }

    // Init keg reader context
    keg::ReadContext context;
    context.env = keg::getGlobalEnvironment();
    context.reader.init(data.c_str());
    keg::Node node = context.reader.getFirst();
    if (keg::getType(node) != keg::NodeType::MAP) {
        fprintf(stderr, "Failed to load %s\n", (m_ioManager->getSearchDirectory().getString() + "/SystemProperties.yml").c_str());
        context.reader.dispose();
        return false;
    }

    // Set up parser
    bool goodParse = true;
    auto f = makeFunctor([this, &goodParse, &context](Sender, const nString& name, keg::Node value) {
        // Parse based on the name
        if (name == "description") {
            m_spaceSystem->m_systemDescription = keg::convert<nString>(value);
        } else if (name == "age") {
            m_spaceSystem->m_age = keg::convert<f32>(value);
        } else {
      
            // Find or allocate body
            SystemBodyProperties* body;
            auto& it = m_systemBodies.find(name);
            if (it == m_systemBodies.end()) {
                // Allocate the body
                body = new SystemBodyProperties();
                m_systemBodies[name] = body;
                body->name = name;
            } else {
                body = it->second;
            }

            std::cout << name << std::endl;

            // Parse orbit
            keg::Error err = keg::parse((ui8*)body, value, context, &KEG_GLOBAL_TYPE(SystemBodyProperties));
            if (err != keg::Error::NONE) {
                fprintf(stderr, "Failed to parse node %s in SystemProperties.yml\n", name.c_str());
                goodParse = false;
                delete body;
            }

            // Set up parent connection
            // At this point, name is actually parent name, to save space
            if (body->name.length()) {
                // Check for parent
                auto& p = m_systemBodies.find(body->name);
                if (p != m_systemBodies.end()) {
                    // Grab existing parent
                    body->parent = p->second;
                } else {
                    // Allocate new parent
                    body->parent = new SystemBodyProperties;
                    m_systemBodies[body->name] = body->parent;
                }
                // Register as child of parent
                body->parent->children.push_back(body);
            }

            // Set actual name, overwriting parent name
            body->name = name;
           
            // Special cases
            switch (body->bodyType) {
                case SpaceBodyType::BARYCENTER:
                    body->genType = SpaceBodyGenerationType::NONE;
                    m_barycenters[name] = body;
                    break;
                case SpaceBodyType::STAR:
                    body->genType = SpaceBodyGenerationType::STAR;
                    break;
                default:
                    break;
            }
        }
    });

    // Parse file
    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();

    return goodParse;
}

bool SpaceSystemLoader::loadSecondaryProperties(SystemBodyProperties* body) {
    // Helper error checking macro
#define KEG_CHECK \
    if (error != keg::Error::NONE) { \
        fprintf(stderr, "keg error %d for %s\n", (int)error, body->path); \
        goodParse = false; \
        return;  \
    }

    nString path = body->path + '/' + FILE_SS_BODY_PROPERTIES;

    keg::Error error;
    nString data;
    m_ioManager->readFileToString(path.c_str(), data);

    // Set up keg context
    keg::ReadContext context;
    context.env = keg::getGlobalEnvironment();
    context.reader.init(data.c_str());
    keg::Node node = context.reader.getFirst();
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to load " + path;
        context.reader.dispose();
        return false;
    }

    bool goodParse = true;
    bool foundOne = false;
    auto f = makeFunctor([&](Sender, const nString& type, keg::Node value) {
        if (foundOne) return;

        // Parse based on type
        if (type == "planet") {
            PlanetProperties* properties = new PlanetProperties;
            body->genType = SpaceBodyGenerationType::PLANET;
            body->genTypeProperties = properties;
           
            error = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(PlanetProperties));
            KEG_CHECK;

            // This should be deferred
            // Use planet loader to load terrain and biomes
           // if (properties.generation.length()) {
          //      properties.planetGenData = m_planetLoader.loadPlanetGenData(properties.generation);
          //  } else {
          //      properties.planetGenData = nullptr;
                // properties.planetGenData = pr.planetLoader->getRandomGenData(properties.density, pr.glrpc);
         //       properties.atmosphere = m_planetLoader.getRandomAtmosphere();
         //   }

            // Set the radius for use later
            //if (properties.planetGenData) {
          //      properties.planetGenData->radius = properties.diameter / 2.0;
          //  }
        } else if (type == "star") {
            StarProperties* properties = new StarProperties;
            body->genType = SpaceBodyGenerationType::STAR;
            body->genTypeProperties = properties;

            error = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(StarProperties));
            KEG_CHECK;
        } else if (type == "gasGiant") {
            GasGiantProperties* properties = new GasGiantProperties;
            body->genType = SpaceBodyGenerationType::GAS_GIANT;
            body->genTypeProperties = properties;

            error = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(GasGiantProperties));
            KEG_CHECK;
            // Get full path for color map
          //  if (properties.colorMap.size()) {
          //      vio::Path colorPath;
          //      if (!m_iom->resolvePath(properties.colorMap, colorPath)) {
          //          fprintf(stderr, "Failed to resolve %s\n", properties.colorMap.c_str());
          //      }
          //      properties.colorMap = colorPath.getString();
          //  }
            // Get full path for rings
          //  if (properties.rings.size()) {
          //      for (size_t i = 0; i < properties.rings.size(); i++) {
          //          auto& r = properties.rings[i];
          //          // Resolve the path
          //          vio::Path ringPath;
          //          if (!m_iom->resolvePath(r.colorLookup, ringPath)) {
          //              fprintf(stderr, "Failed to resolve %s\n", r.colorLookup.c_str());
          //          }
          //          r.colorLookup = ringPath.getString();
          //      }
          //  }
        }

        //Only parse the first
        foundOne = true;
    });
    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();

    return goodParse;
}

void SpaceSystemLoader::initBarycenters() {
    for (auto& it : m_barycenters) {
        SystemBodyProperties* bary = it.second;

        initBarycenter(bary);
    }
}

void SpaceSystemLoader::initBarycenter(SystemBodyProperties* bary) {
    // Don't update twice
    if (bary->isBaryCalculated) return;
    bary->isBaryCalculated = true;

    // Need two components or its not a barycenter
    if (bary->comps.size() != 2) return;

    // A component
    auto& bodyAIt = m_systemBodies.find(std::string(bary->comps[0]));
    if (bodyAIt == m_systemBodies.end()) return;
    SystemBodyProperties* propsA = bodyAIt->second;

    // B component
    auto& bodyBIt = m_systemBodies.find(std::string(bary->comps[1]));
    if (bodyBIt == m_systemBodies.end()) return;
    SystemBodyProperties* propsB = bodyBIt->second;

    { // Set orbit parameters relative to A component
        propsB->ref = propsA->name;
        propsB->td = 1.0f;
        propsB->tf = 1.0f;
        propsB->e = propsA->e;
        propsB->i = propsA->i;
        propsB->n = propsA->n;
        propsB->p = propsA->p + 180.0;
        propsB->a = propsA->a;
        // TODO(Ben): Components aren't set up yet.
        auto& oCmp = m_spaceSystem->spaceBody.getFromEntity(propsB->entity);
        oCmp.e = propsB->e;
        oCmp.i = propsB->i * DEG_TO_RAD;
        oCmp.p = propsB->p * DEG_TO_RAD;
        oCmp.n = propsB->n * DEG_TO_RAD;
        oCmp.startMeanAnomaly = propsB->a * DEG_TO_RAD;
    }

    // Recurse if child is a non-constructed binary
    if (propsA->mass == 0.0) {
        initBarycenter(propsA);
    }

    // Recurse if child is a non-constructed binary
    if (propsB->mass == 0.0) {
        initBarycenter(propsB);
    }

    // Set the barycenter mass
    bary->mass = propsA->mass + propsB->mass;

    { // Calculate A orbit
        f64 massRatio = propsB->mass / (propsA->mass + propsB->mass);
        calculateOrbit(propsA,
                       bary->mass,
                       massRatio);
    }

    { // Calculate B orbit
        f64 massRatio = propsA->mass / (propsA->mass + propsB->mass);
        calculateOrbit(propsB,
                       bary->mass,
                       massRatio);
    }

    { // Set orbit colors from A component // This is rendering
    //    auto& oCmp = m_spaceSystem->orbit.getFromEntity(bodyA->second->entity);
    //    auto& baryOCmp = m_spaceSystem->orbit.getFromEntity(bary->entity);
    //    baryOCmp.pathColor[0] = oCmp.pathColor[0];
    //    baryOCmp.pathColor[1] = oCmp.pathColor[1];
    }
}

void recursiveInclinationCalc(SpaceBodyComponentTable& ct, SystemBodyProperties* body, f64 inclination) {
    for (auto& c : body->children) {
        SpaceBodyComponent& orbitC = ct.getFromEntity(c->entity);
        orbitC.i += inclination;
        recursiveInclinationCalc(ct, c, orbitC.i);
    }
}

void SpaceSystemLoader::initOrbits() {
    

    // Child propagation for inclination
    // TODO(Ben): Do this right
    /* for (auto& it : pr.systemBodies) {
    SystemBody* body = it.second;
    if (!body->parent) {
    recursiveInclinationCalc(pr.spaceSystem->m_orbitCT, body,
    pr.spaceSystem->m_orbitCT.getFromEntity(body->entity).i);
    }
    }*/

    // Finally, calculate the orbits
    for (auto& it : m_systemBodies) {
        SystemBodyProperties* body = it.second;
        // Calculate the orbit using parent mass
        if (body->parent) {
            calculateOrbit(body, body->parent->mass);
        }
    }
}

void SpaceSystemLoader::initComponents() {

    for (auto& it : m_systemBodies) {
        SystemBodyProperties* body = it.second;

        // Add the entity and component
        vecs::EntityID e = m_spaceSystem->addEntity();
        vecs::ComponentID cmpID = m_spaceSystem->addComponent(SPACE_SYSTEM_CT_SPACEBODY_NAME, e);
       
        // Fill out the body component
        SpaceBodyComponent& cmp = m_spaceSystem->spaceBody.get(cmpID);
        cmp.diameter = body->diameter;
        cmp.mass = body->mass;

        cmp.major = body->major;
        cmp.minor = body->minor;
        cmp.e = body->e;
        cmp.t = body->t;
        cmp.n = body->n * DEG_TO_RAD;
        cmp.p = body->p * DEG_TO_RAD;
        cmp.i = body->i * DEG_TO_RAD;
        cmp.startMeanAnomaly = body->a * DEG_TO_RAD;
        cmp.currentMeanAnomaly = cmp.startMeanAnomaly;
        cmp.type = body->bodyType;
    }
}

void SpaceSystemLoader::computeRef(SystemBodyProperties* body) {
    if (!body->ref.empty()) {
        SpaceBodyComponent& orbitC = m_spaceSystem->spaceBody.getFromEntity(body->entity);
        // Find reference body
        auto it = m_systemBodies.find(body->ref);
        if (it != m_systemBodies.end()) {
            SystemBodyProperties* ref = it->second;
            // Recursively compute ref if needed
            if (!ref->hasComputedRef) computeRef(ref);
            // Calculate period using reference body
            orbitC.t = ref->t * body->tf / body->td;
            body->t = orbitC.t;
            // Handle trojans
            if (body->trojan == TrojanType::L4) {
                body->a = ref->a + 60.0f;
                orbitC.startMeanAnomaly = body->a * DEG_TO_RAD;
            } else if (body->trojan == TrojanType::L5) {
                body->a = ref->a - 60.0f;
                orbitC.startMeanAnomaly = body->a * DEG_TO_RAD;
            }
        } else {
            fprintf(stderr, "Failed to find ref body %s\n", body->ref.c_str());
        }
    }
    body->hasComputedRef = true;
}

void SpaceSystemLoader::calculateOrbit(SystemBodyProperties* body,
                                       f64 parentMass,
                                       f64 binaryMassRatio /* = 0.0 */) {

    // If the orbit was already calculated, don't do it again.
    if (m_calculatedOrbits.find(body) != m_calculatedOrbits.end()) return;
    m_calculatedOrbits.insert(body);

    computeRef(body);

    f64 t = body->t;
    f64 diameter = body->diameter;

    if (binaryMassRatio > 0.0) { // Binary orbit
        body->major = pow((t * t) * M_G * parentMass /
                       (4.0 * (M_PI * M_PI)), 1.0 / 3.0) * KM_PER_M * binaryMassRatio;
    } else { // Regular orbit
        // Calculate semi-major axis
        body->major = pow((t * t) * M_G * (body->mass + parentMass) /
                       (4.0 * (M_PI * M_PI)), 1.0 / 3.0) * KM_PER_M;
    }

    // Calculate semi-minor axis
    body->minor = body->major * sqrt(1.0 - body->e * body->e);

    // TODO(Ben): Doesn't work right for binaries due to parentMass
    if (body->parent) { // Check tidal lock
        f64 parentMass = body->parent->mass;
        if (parentMass) {
            f64 ns = log10(0.003 * pow(body->a, 6.0) * pow(diameter + 500.0, 3.0) / (body->mass * parentMass) * (1.0 + (f64)1e20 / (body->mass + parentMass)));
            if (ns < 0) {
                // It is tidally locked so lock the rotational period
                body->rotationalPeriod = t;
            }
        }
    }

    { // Make the ellipse mesh with stepwise simulation
        // TODO(Ben): This is rendering
        /*    OrbitComponentUpdater updater;
            static const int NUM_VERTS = 2880;
            orbitC.verts.resize(NUM_VERTS + 1);
            f64 timePerDeg = orbitC.t / (f64)NUM_VERTS;
            NamePositionComponent& npCmp = m_spaceSystem->namePosition.get(orbitC.npID);
            f64v3 startPos = npCmp.position;
            for (int i = 0; i < NUM_VERTS; i++) {

            if (orbitC.parentOrbId) {
            OrbitComponent* pOrbC = &m_spaceSystem->orbit.get(orbitC.parentOrbId);
            updater.updatePosition(orbitC, i * timePerDeg, &npCmp,
            pOrbC,
            &m_spaceSystem->namePosition.get(pOrbC->npID));
            } else {
            updater.updatePosition(orbitC, i * timePerDeg, &npCmp);
            }

            OrbitComponent::Vertex vert;
            vert.position = npCmp.position;
            vert.angle = 1.0f - (f32)i / (f32)NUM_VERTS;
            orbitC.verts[i] = vert;
            }
            orbitC.verts.back() = orbitC.verts.front();
            npCmp.position = startPos;*/
    }
}
