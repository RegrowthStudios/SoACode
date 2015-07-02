#include "stdafx.h"
#include "PlanetLoader.h"
#include "PlanetData.h"

#include <random>

#include <Vorb/graphics/ImageIO.h>
#include <Vorb/graphics/GpuMemory.h>
#include <Vorb/io/IOManager.h>
#include <Vorb/Events.hpp>
#include <Vorb/io/YAML.h>
#include <Vorb/RPC.h>

#include "Errors.h"

PlanetLoader::PlanetLoader(vio::IOManager* ioManager) :
    m_iom(ioManager),
    m_textureCache(m_iom) {
    // Empty
}

PlanetLoader::~PlanetLoader() {
}

PlanetGenData* PlanetLoader::loadPlanet(const nString& filePath, vcore::RPCManager* glrpc /* = nullptr */) {
    m_glRpc = glrpc;
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

    PlanetGenData* genData = new PlanetGenData;

    nString biomePath = "";

    auto f = makeFunctor<Sender, const nString&, keg::Node>([&](Sender, const nString& type, keg::Node value) {
        // Parse based on type
        if (type == "biomes") {
            loadBiomes(keg::convert<nString>(value), genData);
        } else if (type == "terrainColor") {
            parseTerrainColor(context, value, genData);
        } else if (type == "liquidColor") {
            parseLiquidColor(context, value, genData);
        } else if (type == "tempLatitudeFalloff") {
            genData->tempLatitudeFalloff = keg::convert<f32>(value);
        } else if (type == "humLatitudeFalloff") {
            genData->humLatitudeFalloff = keg::convert<f32>(value);
        } else if (type == "tempHeightFalloff") {
            genData->tempHeightFalloff = keg::convert<f32>(value);
        } else if (type == "humHeightFalloff") {
            genData->humHeightFalloff = keg::convert<f32>(value);
        } else if (type == "baseHeight") {
            parseTerrainFuncs(&genData->baseTerrainFuncs, context, value);
        } else if (type == "temperature") {
            parseTerrainFuncs(&genData->tempTerrainFuncs, context, value);
        } else if (type == "humidity") {
            parseTerrainFuncs(&genData->humTerrainFuncs, context, value);
        } else if (type == "blockLayers") {
            parseBlockLayers(context, value, genData);
        } else if (type == "liquidBlock") {
            genData->blockInfo.liquidBlockName = keg::convert<nString>(value);
        } else if (type == "surfaceBlock") {
            genData->blockInfo.surfaceBlockName = keg::convert<nString>(value);
        }
    });
    context.reader.forAllInMap(node, f);
    context.reader.dispose();

    return genData;
}

PlanetGenData* PlanetLoader::getDefaultGenData(vcore::RPCManager* glrpc /* = nullptr */) {
    // Lazily construct default data
    if (!m_defaultGenData) {
        // Allocate data
        m_defaultGenData = new PlanetGenData;
    }
    return m_defaultGenData;
}

PlanetGenData* PlanetLoader::getRandomGenData(f32 radius, vcore::RPCManager* glrpc /* = nullptr */) {
    // Lazily construct default data

    // Allocate data
    PlanetGenData* genData = m_planetGenerator.generateRandomPlanet(SpaceObjectType::PLANET, glrpc);
    // TODO(Ben): Radius is temporary hacky fix for small planet darkness!
    if (radius < 15.0) {
        genData->baseTerrainFuncs.funcs.setData();
    }

    // Load textures
    if (glrpc) {
        vcore::RPC rpc;
        rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
            genData->grassTexture = m_textureCache.addTexture("_shared/terrain_b.png", vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
            genData->rockTexture = m_textureCache.addTexture("_shared/terrain_a.png", vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
            genData->liquidTexture = m_textureCache.addTexture("_shared/water_a.png", vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
        });
        glrpc->invoke(&rpc, true);
    } else {
        genData->grassTexture = m_textureCache.addTexture("_shared/terrain_b.png", vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
        genData->rockTexture = m_textureCache.addTexture("_shared/terrain_a.png", vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
        genData->liquidTexture = m_textureCache.addTexture("_shared/water_a.png", vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
    }

    return genData;
}

AtmosphereKegProperties PlanetLoader::getRandomAtmosphere() {
    static std::mt19937 generator(2636);
    static std::uniform_real_distribution<f32> randomWav(0.4f, 0.8f);
    AtmosphereKegProperties props;
    props.waveLength.r = randomWav(generator);
    props.waveLength.g = randomWav(generator);
    props.waveLength.b = randomWav(generator);
    return props;
}

void PlanetLoader::loadBiomes(const nString& filePath, PlanetGenData* genData) {
    nString data;
    m_iom->readFileToString(filePath.c_str(), data);

    keg::ReadContext context;
    context.env = keg::getGlobalEnvironment();
    context.reader.init(data.c_str());
    keg::Node node = context.reader.getFirst();
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to load " + filePath;
        context.reader.dispose();
        return;
    }

    m_baseBiomeLookupTextureData.resize(LOOKUP_TEXTURE_SIZE, 0);

    keg::Error error;

    // Load yaml data
    int i = 0;
    auto f = makeFunctor<Sender, const nString&, keg::Node>([&] (Sender, const nString& type, keg::Node value) {
        // Parse based on type
        if (type == "baseLookupMap") {
            vpath texPath;
            m_iom->resolvePath(keg::convert<nString>(value), texPath);
            vg::BitmapResource bitmap = vg::ImageIO().load(texPath.getString());
            if (!bitmap.data) {
                pError("Failed to load " + keg::convert<nString>(value));
            }
            if (bitmap.width != 256 || bitmap.height != 256) {
                pError("loadBiomes() error: width and height of " + keg::convert<nString>(value) + " must be 256");
            }

            for (int i = 0; i < LOOKUP_TEXTURE_SIZE; i++) {
                int index = i * 4;
                ui32 colorCode = ((ui32)bitmap.bytesUI8[index] << 16) | ((ui32)bitmap.bytesUI8[index + 1] << 8) | (ui32)bitmap.bytesUI8[index + 2];
                addBiomePixel(colorCode, i);
            }
        } else {
            // It is a biome
            genData->biomes.emplace_back();
            
            error = keg::parse((ui8*)&genData->biomes.back(), value, context, &KEG_GLOBAL_TYPE(Biome));
            if (error != keg::Error::NONE) {
                fprintf(stderr, "Keg error %d in loadBiomes()\n", (int)error);
                return;
            }
        }
    });
    context.reader.forAllInMap(node, f);
    delete f;
    context.reader.dispose();

    // Code for uploading biome texture
#define BIOME_TEX_CODE \
    genData->baseBiomeLookupTexture = vg::GpuMemory::uploadTexture(m_baseBiomeLookupTextureData.data(),\
                                                                   LOOKUP_TEXTURE_WIDTH, LOOKUP_TEXTURE_WIDTH,\
                                                                   vg::TexturePixelType::UNSIGNED_BYTE,\
                                                                   vg::TextureTarget::TEXTURE_2D,\
                                                                   &vg::SamplerState::POINT_CLAMP,\
                                                                   vg::TextureInternalFormat::R8,\
                                                                   vg::TextureFormat::RED, 0);\
    glGenTextures(1, &genData->biomeArrayTexture); \
    glBindTexture(GL_TEXTURE_2D_ARRAY, genData->biomeArrayTexture);\
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, LOOKUP_TEXTURE_WIDTH, LOOKUP_TEXTURE_WIDTH, m_biomeLookupMap.size(), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);\
    for (auto& it : m_biomeLookupMap) {\
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, it.second.index, LOOKUP_TEXTURE_WIDTH, LOOKUP_TEXTURE_WIDTH,\
                        1, GL_RED, GL_UNSIGNED_BYTE, it.second.data.data());\
    }\
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);\
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);\
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);\
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_biomeLookupMap.size()) {
        // Handle RPC for texture upload
        if (m_glRpc) {
            vcore::RPC rpc;
            rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
                BIOME_TEX_CODE
            });
            m_glRpc->invoke(&rpc, true);
        } else {
            BIOME_TEX_CODE
        }     
    }
    // Free memory
    std::map<ui32, BiomeLookupTexture>().swap(m_biomeLookupMap);
    std::vector<ui8>().swap(m_baseBiomeLookupTextureData);
    m_biomeCount = 0;
}

void PlanetLoader::addBiomePixel(ui32 colorCode, int index) {
    auto& it = m_biomeLookupMap.find(colorCode);
    if (it == m_biomeLookupMap.end()) {
        BiomeLookupTexture& tex = m_biomeLookupMap[colorCode];
        tex.index = m_biomeCount;
        m_baseBiomeLookupTextureData[index] = m_biomeCount;
        tex.data[index] = 255;
        m_biomeCount++;
    } else {
        m_baseBiomeLookupTextureData[index] = m_biomeCount - 1;
        it->second.data[index] = 255;
    }
}

void PlanetLoader::parseTerrainFuncs(NoiseBase* terrainFuncs, keg::ReadContext& context, keg::Node node) {
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to parse node";
        return;
    }

    keg::Error error = keg::parse((ui8*)terrainFuncs, node, context, &KEG_GLOBAL_TYPE(NoiseBase));
    if (error != keg::Error::NONE) {
        fprintf(stderr, "Keg error %d in parseTerrainFuncs()\n", (int)error);
        return;
    }
}

void PlanetLoader::parseLiquidColor(keg::ReadContext& context, keg::Node node, PlanetGenData* genData) {
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to parse node";
        return;
    }

    LiquidColorKegProperties kegProps;

    keg::Error error;
    error = keg::parse((ui8*)&kegProps, node, context, &KEG_GLOBAL_TYPE(LiquidColorKegProperties));
    if (error != keg::Error::NONE) {
        fprintf(stderr, "Keg error %d in parseLiquidColor()\n", (int)error);
        return;
    }

    if (kegProps.colorPath.size()) {
        vg::BitmapResource pixelData;
        // Handle RPC for texture upload
        if (m_glRpc) {
            vcore::RPC rpc;
            rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
                genData->liquidColorMap = m_textureCache.addTexture(kegProps.colorPath,
                                                                    pixelData,
                                                                    vg::ImageIOFormat::RGB_UI8,
                                                                    vg::TextureTarget::TEXTURE_2D,
                                                                    &vg::SamplerState::LINEAR_CLAMP,
                                                                    vg::TextureInternalFormat::RGB8,
                                                                    vg::TextureFormat::RGB);
            });
            m_glRpc->invoke(&rpc, true);
        } else {
            genData->liquidColorMap = m_textureCache.addTexture(kegProps.colorPath,
                                                                pixelData,
                                                                vg::ImageIOFormat::RGB_UI8,
                                                                vg::TextureTarget::TEXTURE_2D,
                                                                &vg::SamplerState::LINEAR_CLAMP,
                                                                vg::TextureInternalFormat::RGB8,
                                                                vg::TextureFormat::RGB);
        }
        // Turn into a color map
        if (genData->liquidColorMap.id) {
            if (genData->liquidColorMap.width != 256 ||
                genData->liquidColorMap.height != 256) {
                std::cerr << "Liquid color map needs to be 256x256";
            } else {
                genData->colorMaps.colorMaps.emplace_back(new vg::BitmapResource);
                *genData->colorMaps.colorMaps.back() = pixelData;
                genData->colorMaps.colorMapTable["liquid"] = genData->colorMaps.colorMaps.back();
            }
        } else {
            vg::ImageIO::free(pixelData);
        }
    }
    if (kegProps.texturePath.size()) {
        // Handle RPC for texture upload
        if (m_glRpc) {
            vcore::RPC rpc;
            rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
                genData->liquidTexture = m_textureCache.addTexture(kegProps.texturePath, vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
            });
            m_glRpc->invoke(&rpc, true);
        } else {
            genData->liquidTexture = m_textureCache.addTexture(kegProps.texturePath, vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
        }
    }
    genData->liquidFreezeTemp = kegProps.freezeTemp;
    genData->liquidDepthScale = kegProps.depthScale;
    genData->liquidTint = kegProps.tint;
}

void PlanetLoader::parseTerrainColor(keg::ReadContext& context, keg::Node node, PlanetGenData* genData) {
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to parse node";
        return;
    }

    TerrainColorKegProperties kegProps;

    keg::Error error;
    error = keg::parse((ui8*)&kegProps, node, context, &KEG_GLOBAL_TYPE(TerrainColorKegProperties));
    if (error != keg::Error::NONE) {
        fprintf(stderr, "Keg error %d in parseTerrainColor()\n", (int)error);
        return;
    }

    if (kegProps.colorPath.size()) {
        // TODO(Ben): "biome" color map will conflict with other planets
        vg::BitmapResource pixelData;
        // Handle RPC for texture upload
        if (m_glRpc) {
            vcore::RPC rpc;
            rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
                genData->terrainColorMap = m_textureCache.addTexture(kegProps.colorPath,
                                                                     pixelData,
                                                                     vg::ImageIOFormat::RGB_UI8,
                                                                     vg::TextureTarget::TEXTURE_2D,
                                                                     &vg::SamplerState::LINEAR_CLAMP,
                                                                     vg::TextureInternalFormat::RGB8,
                                                                     vg::TextureFormat::RGB);
            });
            m_glRpc->invoke(&rpc, true);
        } else {
            genData->terrainColorMap = m_textureCache.addTexture(kegProps.colorPath,
                                                                 pixelData,
                                                                 vg::ImageIOFormat::RGB_UI8,
                                                                 vg::TextureTarget::TEXTURE_2D,
                                                                 &vg::SamplerState::LINEAR_CLAMP,
                                                                 vg::TextureInternalFormat::RGB8,
                                                                 vg::TextureFormat::RGB);
        }
        // Turn into a color map
        if (genData->terrainColorMap.id) {
            if (genData->terrainColorMap.width != 256 ||
                genData->terrainColorMap.height != 256) {
                std::cerr << "Terrain color map needs to be 256x256";
            } else {
                genData->colorMaps.colorMaps.emplace_back(new vg::BitmapResource);
                *genData->colorMaps.colorMaps.back() = pixelData;
                genData->colorMaps.colorMapTable["biome"] = genData->colorMaps.colorMaps.back();
            }
        } else {
            vg::ImageIO::free(pixelData);
        }
    }
    // TODO(Ben): stop being lazy and copy pasting
    if (kegProps.grassTexturePath.size()) {
        // Handle RPC for texture upload
        if (m_glRpc) {
            vcore::RPC rpc;
            rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
                genData->grassTexture = m_textureCache.addTexture(kegProps.grassTexturePath, vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
            });
            m_glRpc->invoke(&rpc, true);
        } else {
            genData->grassTexture = m_textureCache.addTexture(kegProps.grassTexturePath, vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
        }
    }
    if (kegProps.rockTexturePath.size()) {
        // Handle RPC for texture upload
        if (m_glRpc) {
            vcore::RPC rpc;
            rpc.data.f = makeFunctor<Sender, void*>([&](Sender s, void* userData) {
                genData->rockTexture = m_textureCache.addTexture(kegProps.rockTexturePath, vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
            });
            m_glRpc->invoke(&rpc, true);
        } else {
            genData->rockTexture = m_textureCache.addTexture(kegProps.rockTexturePath, vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_WRAP_MIPMAP);
        }
    }
    genData->terrainTint = kegProps.tint;
}

void PlanetLoader::parseBlockLayers(keg::ReadContext& context, keg::Node node, PlanetGenData* genData) {
    if (keg::getType(node) != keg::NodeType::MAP) {
        std::cout << "Failed to parse node in parseBlockLayers. Should be MAP";
        return;
    }

    auto f = makeFunctor<Sender, const nString&, keg::Node>([&](Sender, const nString& name, keg::Node value) {
        // Add a block
        genData->blockInfo.blockLayerNames.emplace_back(name);
        genData->blockLayers.emplace_back();

        BlockLayer& l = genData->blockLayers.back();
        // Load data
        keg::parse((ui8*)&l, value, context, &KEG_GLOBAL_TYPE(BlockLayer));
    });
    context.reader.forAllInMap(node, f);
    delete f;

    // Set starts for binary search application
    int start = 0;
    for (auto& l : genData->blockLayers) {
        l.start = start;
        start += l.width;
    }
}
