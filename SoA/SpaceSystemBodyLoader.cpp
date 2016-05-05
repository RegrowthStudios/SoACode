#include "stdafx.h"
#include "SpaceSystemBodyLoader.h"

#include "SpaceSystemAssemblages.h"
#include "SoAState.h"

#include <Vorb/io/IOManager.h>

/************************************************************************/
/* File Strings                                                         */
/************************************************************************/

const nString FILE_SS_BODY_PROPERTIES = "properties.yml";
const nString SS_GENTYPE_PLANET = "planet";
const nString SS_GENTYPE_STAR = "star";
const nString SS_GENTYPE_GASGIANT = "gasGiant";

/************************************************************************/


void SpaceSystemBodyLoader::init(vio::IOManager* iom) {
    m_iom = iom;
    m_planetLoader.init(iom);
}

bool SpaceSystemBodyLoader::loadBody(const SoaState* soaState, const nString& filePath,
                                     SystemBodyProperties* body,
                                     vcore::RPCManager* glrpc /* = nullptr */) {

#define PARSE(outData, type) \
    keg::parse((ui8*)outData, value, context, &KEG_GLOBAL_TYPE(type)); \
    if (error != keg::Error::NONE) { \
        fprintf(stderr, "keg error %d for %s\n", (int)error, filePath); \
        goodParse = false; \
        return;  \
    } 

    
    keg::Error error;
    nString data;
    m_iom->readFileToString(filePath.c_str(), data);

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
    auto f = makeFunctor([&](Sender, const nString& type, keg::Node value) {
        if (foundOne) return;

        // Parse based on type
        if (type == SS_GENTYPE_PLANET) {
            body->genType = SpaceBodyGenerationType::PLANET;
            PARSE(&body->planetProperties, PlanetProperties);

            // Use planet loader to load terrain and biomes
            //if (properties.generation.length()) {
            //    properties.planetGenData = m_planetLoader.loadPlanetGenData(properties.generation);
            //} else {
            //    properties.planetGenData = nullptr;
            //    // properties.planetGenData = pr.planetLoader->getRandomGenData(properties.density, pr.glrpc);
            //    properties.atmosphere = m_planetLoader.getRandomAtmosphere();
            //}

        } else if (type == SS_GENTYPE_STAR) {
            body->genType = SpaceBodyGenerationType::STAR;
            PARSE(&body->starProperties, StarProperties);
        } else if (type == SS_GENTYPE_GASGIANT) {
            body->genType = SpaceBodyGenerationType::GAS_GIANT;
            PARSE(&body->gasGiantProperties, GasGiantProperties);
            
            // Get full path for color map
            if (body->gasGiantProperties.colorMap.size()) {
                vio::Path colorPath;
                if (!m_iom->resolvePath(body->gasGiantProperties.colorMap, colorPath)) {
                    fprintf(stderr, "Failed to resolve %s\n", body->gasGiantProperties.colorMap.c_str());
                }
                body->gasGiantProperties.colorMap = colorPath.getString();
            }
            // Get full path for rings
            if (body->gasGiantProperties.rings.size()) {
                for (size_t i = 0; i < body->gasGiantProperties.rings.size(); i++) {
                    auto& r = body->gasGiantProperties.rings[i];
                    // Resolve the path
                    vio::Path ringPath;
                    if (!m_iom->resolvePath(r.colorLookup, ringPath)) {
                        fprintf(stderr, "Failed to resolve %s\n", r.colorLookup.c_str());
                    }
                    r.colorLookup = ringPath.getString();
                }
            }
          
        }

        //Only parse the first
        foundOne = true;
    });
    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();

    return goodParse;

#undef PARSE
}
