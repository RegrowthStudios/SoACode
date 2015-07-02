#include "stdafx.h"
#include "SpaceSystemLoader.h"

#include "Constants.h"
#include "Errors.h"
#include "SoaOptions.h"
#include "OrbitComponentUpdater.h"
#include "PlanetData.h"
#include "PlanetLoader.h"
#include "ProgramGenDelegate.h"
#include "SoaState.h"
#include "SpaceSystemAssemblages.h"
#include "SpaceSystemLoadStructs.h"

#include <Vorb/RPC.h>
#include <Vorb/graphics/GpuMemory.h>
#include <Vorb/graphics/ImageIO.h>
#include <Vorb/io/keg.h>
#include <Vorb/ui/GameWindow.h>

void SpaceSystemLoader::loadStarSystem(SpaceSystemLoadParams& pr) {
    pr.ioManager->setSearchDirectory((pr.dirPath + "/").c_str());

    // Load the path color scheme
    loadPathColors(pr);

    // Load the system
    loadSystemProperties(pr);

    // Set up binary masses
    initBinaries(pr);

    // Set up parent connections and orbits
    initOrbits(pr);
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

bool SpaceSystemLoader::loadPathColors(SpaceSystemLoadParams& pr) {
    nString data;
    if (!pr.ioManager->readFileToString("PathColors.yml", data)) {
        pError("Couldn't find " + pr.dirPath + "/PathColors.yml");
    }

    keg::ReadContext context;
    context.env = keg::getGlobalEnvironment();
    context.reader.init(data.c_str());
    keg::Node node = context.reader.getFirst();
    if (keg::getType(node) != keg::NodeType::MAP) {
        fprintf(stderr, "Failed to load %s\n", (pr.dirPath + "/PathColors.yml").c_str());
        context.reader.dispose();
        return false;
    }

    bool goodParse = true;
    auto f = makeFunctor<Sender, const nString&, keg::Node>([&](Sender, const nString& name, keg::Node value) {
        PathColorKegProps props;
        keg::Error err = keg::parse((ui8*)&props, value, context, &KEG_GLOBAL_TYPE(PathColorKegProps));
        if (err != keg::Error::NONE) {
            fprintf(stderr, "Failed to parse node %s in PathColors.yml\n", name.c_str());
            goodParse = false;
        }
        f32v4 base = f32v4(props.base) / 255.0f;
        f32v4 hover = f32v4(props.hover) / 255.0f;
        pr.spaceSystem->pathColorMap[name] = std::make_pair(base, hover);
    });

    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();
    return goodParse;
}

bool SpaceSystemLoader::loadSystemProperties(SpaceSystemLoadParams& pr) {
    nString data;
    if (!pr.ioManager->readFileToString("SystemProperties.yml", data)) {
        pError("Couldn't find " + pr.dirPath + "/SystemProperties.yml");
    }

    keg::ReadContext context;
    context.env = keg::getGlobalEnvironment();
    context.reader.init(data.c_str());
    keg::Node node = context.reader.getFirst();
    if (keg::getType(node) != keg::NodeType::MAP) {
        fprintf(stderr, "Failed to load %s\n", (pr.dirPath + "/SystemProperties.yml").c_str());
        context.reader.dispose();
        return false;
    }

    bool goodParse = true;
    auto f = makeFunctor<Sender, const nString&, keg::Node>([&](Sender, const nString& name, keg::Node value) {
        // Parse based on the name
        if (name == "description") {
            pr.spaceSystem->systemDescription = keg::convert<nString>(value);
        } else if (name == "age") {
            pr.spaceSystem->age = keg::convert<f32>(value);
        } else {
            SystemBodyKegProperties properties;
            keg::Error err = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(SystemBodyKegProperties));
            if (err != keg::Error::NONE) {
                fprintf(stderr, "Failed to parse node %s in SystemProperties.yml\n", name.c_str());
                goodParse = false;
            }

            // Allocate the body
            SystemBody* body = new SystemBody;
            body->name = name;
            body->parentName = properties.par;
            body->properties = properties;
            if (properties.path.size()) {
                loadBodyProperties(pr, properties.path, &properties, body);
            } else {
                // Make default orbit (used for barycenters)
                SpaceSystemAssemblages::createOrbit(pr.spaceSystem, &properties, body, 0.0);
            }
            if (properties.type == SpaceObjectType::BARYCENTER) {
                pr.barycenters[name] = body;
            }
            pr.systemBodies[name] = body;
        }
    });
    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();
    return goodParse;
}

bool SpaceSystemLoader::loadBodyProperties(SpaceSystemLoadParams& pr, const nString& filePath,
                                   const SystemBodyKegProperties* sysProps, SystemBody* body) {

#define KEG_CHECK \
    if (error != keg::Error::NONE) { \
        fprintf(stderr, "keg error %d for %s\n", (int)error, filePath); \
        goodParse = false; \
        return;  \
            }

    keg::Error error;
    nString data;
    pr.ioManager->readFileToString(filePath.c_str(), data);

    keg::ReadContext context;
    context.env = keg::getGlobalEnvironment();
    context.reader.init(data.c_str());
    keg::Node node = context.reader.getFirst();
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to load " + filePath;
        context.reader.dispose();
        return false;
    }

    bool goodParse = true;
    bool foundOne = false;
    auto f = makeFunctor<Sender, const nString&, keg::Node>([&](Sender, const nString& type, keg::Node value) {
        if (foundOne) return;

        // Parse based on type
        if (type == "planet") {
            PlanetKegProperties properties;
            error = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(PlanetKegProperties));
            KEG_CHECK;

            // Use planet loader to load terrain and biomes
            if (properties.generation.length()) {
                properties.planetGenData = pr.planetLoader->loadPlanet(properties.generation, pr.glrpc);
            } else {
                properties.planetGenData = nullptr;
                // properties.planetGenData = pr.planetLoader->getRandomGenData(properties.density, pr.glrpc);
                properties.atmosphere = pr.planetLoader->getRandomAtmosphere();
            }

            // Set the radius for use later
            if (properties.planetGenData) {
                properties.planetGenData->radius = properties.diameter / 2.0;
            }

            SpaceSystemAssemblages::createPlanet(pr.spaceSystem, sysProps, &properties, body, pr.threadpool);
            body->type = SpaceBodyType::PLANET;
        } else if (type == "star") {
            StarKegProperties properties;
            error = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(StarKegProperties));
            KEG_CHECK;
            SpaceSystemAssemblages::createStar(pr.spaceSystem, sysProps, &properties, body);
            body->type = SpaceBodyType::STAR;
        } else if (type == "gasGiant") {
            GasGiantKegProperties properties;
            error = keg::parse((ui8*)&properties, value, context, &KEG_GLOBAL_TYPE(GasGiantKegProperties));
            KEG_CHECK;
            createGasGiant(pr, sysProps, &properties, body);
            body->type = SpaceBodyType::GAS_GIANT;
        }

        pr.bodyLookupMap[body->name] = body->entity;

        //Only parse the first
        foundOne = true;
    });
    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();

    return goodParse;
}

void SpaceSystemLoader::initBinaries(SpaceSystemLoadParams& pr) {
    for (auto& it : pr.barycenters) {
        SystemBody* bary = it.second;

        initBinary(pr, bary);
    }
}

void SpaceSystemLoader::initBinary(SpaceSystemLoadParams& pr, SystemBody* bary) {
    // Don't update twice
    if (bary->isBaryCalculated) return;
    bary->isBaryCalculated = true;

    // Need two components or its not a binary
    if (bary->properties.comps.size() != 2) return;

    // A component
    auto& bodyA = pr.systemBodies.find(std::string(bary->properties.comps[0]));
    if (bodyA == pr.systemBodies.end()) return;
    auto& aProps = bodyA->second->properties;

    // B component
    auto& bodyB = pr.systemBodies.find(std::string(bary->properties.comps[1]));
    if (bodyB == pr.systemBodies.end()) return;
    auto& bProps = bodyB->second->properties;

    { // Set orbit parameters relative to A component
        bProps.ref = bodyA->second->name;
        bProps.td = 1.0f;
        bProps.tf = 1.0f;
        bProps.e = aProps.e;
        bProps.i = aProps.i;
        bProps.n = aProps.n;
        bProps.p = aProps.p + 180.0;
        bProps.a = aProps.a;
        auto& oCmp = pr.spaceSystem->m_orbitCT.getFromEntity(bodyB->second->entity);
        oCmp.e = bProps.e;
        oCmp.i = bProps.i * DEG_TO_RAD;
        oCmp.p = bProps.p * DEG_TO_RAD;
        oCmp.o = bProps.n * DEG_TO_RAD;
        oCmp.startMeanAnomaly = bProps.a * DEG_TO_RAD;
    }

    // Get the A mass
    auto& aSgCmp = pr.spaceSystem->m_sphericalGravityCT.getFromEntity(bodyA->second->entity);
    f64 massA = aSgCmp.mass;
    // Recurse if child is a non-constructed binary
    if (massA == 0.0) {
        initBinary(pr, bodyA->second);
        massA = aSgCmp.mass;
    }

    // Get the B mass
    auto& bSgCmp = pr.spaceSystem->m_sphericalGravityCT.getFromEntity(bodyB->second->entity);
    f64 massB = bSgCmp.mass;
    // Recurse if child is a non-constructed binary
    if (massB == 0.0) {
        initBinary(pr, bodyB->second);
        massB = bSgCmp.mass;
    }

    // Set the barycenter mass
    bary->mass = massA + massB;

    auto& barySgCmp = pr.spaceSystem->m_sphericalGravityCT.getFromEntity(bary->entity);
    barySgCmp.mass = bary->mass;

    { // Calculate A orbit
        SystemBody* body = bodyA->second;
        body->parent = bary;
        bary->children.push_back(body);
        f64 massRatio = massB / (massA + massB);
        calculateOrbit(pr, body->entity,
                       barySgCmp.mass,
                       body, massRatio);
    }

    { // Calculate B orbit
        SystemBody* body = bodyB->second;
        body->parent = bary;
        bary->children.push_back(body);
        f64 massRatio = massA / (massA + massB);
        calculateOrbit(pr, body->entity,
                       barySgCmp.mass,
                       body, massRatio);
    }

    { // Set orbit colors from A component
        auto& oCmp = pr.spaceSystem->m_orbitCT.getFromEntity(bodyA->second->entity);
        auto& baryOCmp = pr.spaceSystem->m_orbitCT.getFromEntity(bary->entity);
        baryOCmp.pathColor[0] = oCmp.pathColor[0];
        baryOCmp.pathColor[1] = oCmp.pathColor[1];
    }
}

void recursiveInclinationCalc(OrbitComponentTable& ct, SystemBody* body, f64 inclination) {
    for (auto& c : body->children) {
        OrbitComponent& orbitC = ct.getFromEntity(c->entity);
        orbitC.i += inclination;
        recursiveInclinationCalc(ct, c, orbitC.i);
    }
}

void SpaceSystemLoader::initOrbits(SpaceSystemLoadParams& pr) {
    // Set parent connections
    for (auto& it : pr.systemBodies) {
        SystemBody* body = it.second;
        const nString& parent = body->parentName;
        if (parent.length()) {
            // Check for parent
            auto& p = pr.systemBodies.find(parent);
            if (p != pr.systemBodies.end()) {
                // Set up parent connection
                body->parent = p->second;
                p->second->children.push_back(body);
            }
        }
    }

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
    for (auto& it : pr.systemBodies) {
        SystemBody* body = it.second;
        // Calculate the orbit using parent mass
        if (body->parent) {
            calculateOrbit(pr, body->entity,
                           pr.spaceSystem->m_sphericalGravityCT.getFromEntity(body->parent->entity).mass,
                           body);
        }
    }
}

void SpaceSystemLoader::createGasGiant(SpaceSystemLoadParams& pr,
                               const SystemBodyKegProperties* sysProps,
                               GasGiantKegProperties* properties,
                               SystemBody* body) {

    // Load the texture
    VGTexture colorMap = 0;
    if (properties->colorMap.size()) {
        vio::Path colorPath;
        if (!pr.ioManager->resolvePath(properties->colorMap, colorPath)) {
            fprintf(stderr, "Failed to resolve %s\n", properties->colorMap.c_str());
            return;
        }
        // Make the functor for loading texture
        auto f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
            vg::ScopedBitmapResource b = vg::ImageIO().load(colorPath);
            if (b.data) {
                colorMap = vg::GpuMemory::uploadTexture(&b, vg::TexturePixelType::UNSIGNED_BYTE,
                                                        vg::TextureTarget::TEXTURE_2D,
                                                        &vg::SamplerState::LINEAR_CLAMP);
            } else {
                fprintf(stderr, "Failed to load %s\n", properties->colorMap.c_str());
            }
        });
        // Check if we need to do RPC
        if (pr.glrpc) {
            vcore::RPC rpc;
            rpc.data.f = f;
            pr.glrpc->invoke(&rpc, true);
        } else {
            f->invoke(0, nullptr);
        }
        delete f;
    }

    // Load the rings
    if (properties->rings.size()) {

        auto f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
            for (size_t i = 0; i < properties->rings.size(); i++) {
                auto& r = properties->rings[i];
                // Resolve the path
                vio::Path ringPath;
                if (!pr.ioManager->resolvePath(r.colorLookup, ringPath)) {
                    fprintf(stderr, "Failed to resolve %s\n", r.colorLookup.c_str());
                    return;
                }
                // Load the texture
                vg::ScopedBitmapResource b = vg::ImageIO().load(ringPath);
                if (b.data) {
                    r.texture = vg::GpuMemory::uploadTexture(&b, vg::TexturePixelType::UNSIGNED_BYTE,
                                                             vg::TextureTarget::TEXTURE_2D,
                                                             &vg::SamplerState::LINEAR_CLAMP);
                } else {
                    fprintf(stderr, "Failed to load %s\n", r.colorLookup.c_str());
                }
            }
        });
        if (pr.glrpc) {
            vcore::RPC rpc;
            rpc.data.f = f;
            pr.glrpc->invoke(&rpc, true);
        } else {
            f->invoke(0, nullptr);
        }
    }
    SpaceSystemAssemblages::createGasGiant(pr.spaceSystem, sysProps, properties, body, colorMap);
}

void computeRef(SpaceSystemLoadParams& pr, SystemBody* body) {
    if (!body->properties.ref.empty()) {
        SpaceSystem* spaceSystem = pr.spaceSystem;
        OrbitComponent& orbitC = spaceSystem->m_orbitCT.getFromEntity(body->entity);
        // Find reference body
        auto it = pr.systemBodies.find(body->properties.ref);
        if (it != pr.systemBodies.end()) {
            SystemBody* ref = it->second;
            // Recursively compute ref if needed
            if (!ref->hasComputedRef) computeRef(pr, ref);
            // Calculate period using reference body
            orbitC.t = ref->properties.t * body->properties.tf / body->properties.td;
            body->properties.t = orbitC.t;
            // Handle trojans
            if (body->properties.trojan == TrojanType::L4) {
                body->properties.a = ref->properties.a + 60.0f;
                orbitC.startMeanAnomaly = body->properties.a * DEG_TO_RAD;
            } else if (body->properties.trojan == TrojanType::L5) {
                body->properties.a = ref->properties.a - 60.0f;
                orbitC.startMeanAnomaly = body->properties.a * DEG_TO_RAD;
            }
        } else {
            fprintf(stderr, "Failed to find ref body %s\n", body->properties.ref.c_str());
        }
    }
    body->hasComputedRef = true;
}

void SpaceSystemLoader::calculateOrbit(SpaceSystemLoadParams& pr, vecs::EntityID entity, f64 parentMass,
                               SystemBody* body, f64 binaryMassRatio /* = 0.0 */) {
    SpaceSystem* spaceSystem = pr.spaceSystem;
    OrbitComponent& orbitC = spaceSystem->m_orbitCT.getFromEntity(entity);

    // If the orbit was already calculated, don't do it again.
    if (orbitC.isCalculated) return;
    orbitC.isCalculated = true;

    // Provide the orbit component with it's parent
    pr.spaceSystem->m_orbitCT.getFromEntity(body->entity).parentOrbId =
        pr.spaceSystem->m_orbitCT.getComponentID(body->parent->entity);

    computeRef(pr, body);

    f64 t = orbitC.t;
    auto& sgCmp = spaceSystem->m_sphericalGravityCT.getFromEntity(entity);
    f64 mass = sgCmp.mass;
    f64 diameter = sgCmp.radius * 2.0;

    if (binaryMassRatio > 0.0) { // Binary orbit
        orbitC.a = pow((t * t) * M_G * parentMass /
                       (4.0 * (M_PI * M_PI)), 1.0 / 3.0) * KM_PER_M * binaryMassRatio;
    } else { // Regular orbit
        // Calculate semi-major axis
        orbitC.a = pow((t * t) * M_G * (mass + parentMass) /
                       (4.0 * (M_PI * M_PI)), 1.0 / 3.0) * KM_PER_M;
    }

    // Calculate semi-minor axis
    orbitC.b = orbitC.a * sqrt(1.0 - orbitC.e * orbitC.e);

    // Set parent pass
    orbitC.parentMass = parentMass;

    // TODO(Ben): Doesn't work right for binaries due to parentMass
    { // Check tidal lock
        f64 ns = log10(0.003 * pow(orbitC.a, 6.0) * pow(diameter + 500.0, 3.0) / (mass * orbitC.parentMass) * (1.0 + (f64)1e20 / (mass + orbitC.parentMass)));
        if (ns < 0) {
            // It is tidally locked so lock the rotational period
            spaceSystem->m_axisRotationCT.getFromEntity(entity).period = t;
        }
    }

    { // Make the ellipse mesh with stepwise simulation
        OrbitComponentUpdater updater;
        static const int NUM_VERTS = 2880;
        orbitC.verts.resize(NUM_VERTS + 1);
        f64 timePerDeg = orbitC.t / (f64)NUM_VERTS;
        NamePositionComponent& npCmp = spaceSystem->m_namePositionCT.get(orbitC.npID);
        f64v3 startPos = npCmp.position;
        for (int i = 0; i < NUM_VERTS; i++) {

            if (orbitC.parentOrbId) {
                OrbitComponent* pOrbC = &spaceSystem->m_orbitCT.get(orbitC.parentOrbId);
                updater.updatePosition(orbitC, i * timePerDeg, &npCmp,
                                       pOrbC,
                                       &spaceSystem->m_namePositionCT.get(pOrbC->npID));
            } else {
                updater.updatePosition(orbitC, i * timePerDeg, &npCmp);
            }

            OrbitComponent::Vertex vert;
            vert.position = npCmp.position;
            vert.angle = 1.0f - (f32)i / (f32)NUM_VERTS;
            orbitC.verts[i] = vert;
        }
        orbitC.verts.back() = orbitC.verts.front();
        npCmp.position = startPos;
    }
}
