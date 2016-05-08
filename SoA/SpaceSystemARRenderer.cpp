#include "stdafx.h"
#include "SpaceSystemARRenderer.h"

#include "Camera.h"
#include "MainMenuSystemViewer.h"
#include "ModPathResolver.h"
#include "ShaderLoader.h"
#include "SpaceBodyComponentUpdater.h"
#include "SpaceSystem.h"
#include "Errors.h"
#include "SoaThreads.h"
#include "soaUtils.h"

#include <Vorb/colors.h>
#include <Vorb/graphics/DepthState.h>
#include <Vorb/graphics/GpuMemory.h>
#include <Vorb/graphics/SamplerState.h>
#include <Vorb/graphics/SpriteBatch.h>
#include <Vorb/graphics/SpriteFont.h>
#include <Vorb/io/IOManager.h>
#include <Vorb/utils.h>

namespace {
    const cString VERT_SRC = R"(
uniform mat4 unWVP;
in vec4 vPosition;
in float vAngle;
out float fAngle;
#include "Shaders/Utils/logz.glsl"
void main() {
    fAngle = vAngle;
    gl_Position = unWVP * vPosition;
    applyLogZ();
}
)";

    const cString FRAG_SRC = R"(
uniform vec4 unColor;
uniform float currentAngle;
in float fAngle;
out vec4 pColor;
void main() {
    pColor = unColor * vec4(1.0, 1.0, 1.0, 1.0 - mod(fAngle + currentAngle, 1.0));
}
)";
}

#define PATH_ORBITPATH_COLORS "StarSystems/path_colors.yml"

struct PathColorKegProps {
    color4 base = color4(0, 0, 0, 0);
    color4 hover = color4(0, 0, 0, 0);
};
KEG_TYPE_DEF_SAME_NAME(PathColorKegProps, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, PathColorKegProps, base, UI8_V4);
    KEG_TYPE_INIT_ADD_MEMBER(kt, PathColorKegProps, hover, UI8_V4);
}

SpaceSystemARRenderer::SpaceSystemARRenderer() {
    // Empty
}

SpaceSystemARRenderer::~SpaceSystemARRenderer() {
    dispose();
}

void SpaceSystemARRenderer::init(const ModPathResolver* textureResolver) {
    m_textureResolver = textureResolver;

    // Set up lookup table
    std::map<nString, SpaceBodyType> m_bodyTypeLookup;
    for (int i = 1; i < (int)SpaceBodyType::SIZE; i++) {
        m_bodyTypeLookup[SpaceBodyTypeStrings[i]] = (SpaceBodyType)i;
    }

    { // Load Path Colors
        nString data;
        if (!vio::IOManager().readFileToString(PATH_ORBITPATH_COLORS, data)) {
            fprintf(stderr, "Failed to load %s\n", PATH_ORBITPATH_COLORS);
            return;
        }

        // TODO(Ben): Keg helper function for maps with functor iteration

        keg::ReadContext context;
        context.env = keg::getGlobalEnvironment();
        context.reader.init(data.c_str());
        keg::Node node = context.reader.getFirst();
        if (keg::getType(node) != keg::NodeType::MAP) {
            fprintf(stderr, "Failed to load %s\n", PATH_ORBITPATH_COLORS);
            context.reader.dispose();
            return;
        }

        bool goodParse = true;
        auto f = makeFunctor([&](Sender, const nString& name, keg::Node value) {
            PathColorKegProps props;
            keg::Error err = keg::parse((ui8*)&props, value, context, &KEG_GLOBAL_TYPE(PathColorKegProps));
            if (err != keg::Error::NONE) {
                fprintf(stderr, "Failed to parse node %s in PathColors.yml\n", name.c_str());
                goodParse = false;
                return;
            }

            auto& it = m_bodyTypeLookup.find(name);
            if (it == m_bodyTypeLookup.end()) {
                fprintf(stderr, "Failed to parse node %s in PathColors.yml\n", name.c_str());
                goodParse = false;
                return;
            }
            m_pathColorMap[(int)it->second] = std::make_pair(props.base, props.hover);
        });

        context.reader.forAllInMap(node, f);
        delete f;
        context.reader.dispose();
        if (!goodParse) {
            fprintf(stderr, "Parse failure for %s\n", PATH_ORBITPATH_COLORS);
            return;
        }
    }

}

void SpaceSystemARRenderer::initGL() {
    if (!m_colorProgram.isCreated()) m_colorProgram = ShaderLoader::createProgram("SystemAR", VERT_SRC, FRAG_SRC);
    if (m_selectorTexture == 0) loadTextures();
}

void SpaceSystemARRenderer::draw(SpaceSystem* spaceSystem, const Camera* camera,
                            OPT const MainMenuSystemViewer* systemViewer,
                            const f32v2& viewport) {
    // Get handles so we don't have huge parameter lists
    m_spaceSystem = spaceSystem;
    m_camera = camera;
    m_systemViewer = systemViewer;
    m_viewport = viewport;
    m_zCoef = computeZCoef(camera->getFarClip());
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    drawPaths();
    if (m_systemViewer) {
    //    drawHUD();
    }
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SpaceSystemARRenderer::dispose() {
    if (m_colorProgram.isCreated()) {
        m_colorProgram.dispose();
    }
  
    if (m_spriteBatch) {
        m_spriteBatch->dispose();
        m_spriteFont->dispose();
        delete m_spriteBatch;
        m_spriteBatch = nullptr;
        delete m_spriteFont;
        m_spriteFont = nullptr;
    }
}

void SpaceSystemARRenderer::loadTextures() {
    { // Selector
        vio::Path path;
        m_textureResolver->resolvePath("GUI/selector.png", path);
        vg::ScopedBitmapResource res = vg::ImageIO().load(path);
        if (!res.data) {
            fprintf(stderr, "ERROR: Failed to load GUI/selector.png\n");
        }
        m_selectorTexture = vg::GpuMemory::uploadTexture(&res, vg::TexturePixelType::UNSIGNED_BYTE,
                                                         vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_CLAMP_MIPMAP);
    }
    { // Barycenter
        vio::Path path;
        m_textureResolver->resolvePath("GUI/barycenter.png", path);
        vg::ScopedBitmapResource res = vg::ImageIO().load(path);
        if (!res.data) {
            fprintf(stderr, "ERROR: Failed to load GUI/barycenter.png\n");
        }
        m_baryTexture = vg::GpuMemory::uploadTexture(&res, vg::TexturePixelType::UNSIGNED_BYTE,
                                                     vg::TextureTarget::TEXTURE_2D, &vg::SamplerState::LINEAR_CLAMP_MIPMAP);
    }
}

void SpaceSystemARRenderer::drawPaths() {

    AssertIsGraphics();

    m_colorProgram.use();
    m_colorProgram.enableVertexAttribArrays();

    // For logarithmic Z buffer
    glUniform1f(m_colorProgram.getUniform("unZCoef"), m_zCoef);
    glLineWidth(3.0f);

    color4 color;

    f32m4 wvp = m_camera->getViewProjectionMatrix();
    for (auto& it : m_spaceSystem->spaceBody) {
        SpaceBodyComponent& cmp = it.second;

        if (m_systemViewer) {
            // Get the augmented reality data
            const MainMenuSystemViewer::BodyArData* bodyArData = m_systemViewer->findBodyAr(it.first);
            if (bodyArData == nullptr) continue;

            // If its selected we force a different color
            if (m_systemViewer->getTargetBody() == it.first) {
                color = m_selectedColor;
            } else {
                // Hermite interpolated color
                const std::pair<color4, color4>& pathColorRange = getPathColorRange(cmp.type);
                color.lerp(pathColorRange.first, pathColorRange.second, hermite(bodyArData->hoverTime));
            }
        }

        if (cmp.parentBodyComponent) {
            // TODO(Ben): We can do this without parent by using the base component position,
            // as it already accounts parent position.

            const SpaceBodyComponent& pBodyCmp = m_spaceSystem->spaceBody.get(cmp.parentBodyComponent);
            m_orbitComponentRenderer.drawPath(cmp, getOrbitPathPathRenderData(cmp, it.first),
                                              m_colorProgram, wvp, m_camera->getPosition(), color,
                                              &pBodyCmp);


        } else {

            m_orbitComponentRenderer.drawPath(cmp, getOrbitPathPathRenderData(cmp, it.first),
                                              m_colorProgram, wvp, m_camera->getPosition(), color,
                                              nullptr);

        }

    }
    m_colorProgram.disableVertexAttribArrays();
    m_colorProgram.unuse();
}

void SpaceSystemARRenderer::drawHUD() {
    const f32 ROTATION_FACTOR = (f32)(M_PI + M_PI / 4.0);
    static f32 dt = 0.0;
    dt += 0.01f;

    // Lazily load spritebatch
    if (!m_spriteBatch) {
        m_spriteBatch = new vg::SpriteBatch(true, true);
        m_spriteFont = new vg::SpriteFont();
        m_spriteFont->init("Fonts/orbitron_bold-webfont.ttf", 32);
    }

    m_spriteBatch->begin();

    // Render all bodies
    for (auto& it : m_spaceSystem->spaceBody) {

        auto& cmp = it.second;

        // Get the augmented reality data
        const MainMenuSystemViewer::BodyArData* bodyArData = m_systemViewer->findBodyAr(it.first);
        if (bodyArData == nullptr) continue;

        if (bodyArData->inFrustum) {

            f64v3 position = cmp.position;
            f64v3 relativePos = position - m_camera->getPosition();
            f64 distance = vmath::length(relativePos);
            color4 textColor;

            f32 hoverTime = bodyArData->hoverTime;

            // Get screen position 
            f32v3 screenCoords = m_camera->worldToScreenPointLogZ(relativePos, (f64)m_camera->getFarClip());
            f32v2 xyScreenCoords(screenCoords.x * m_viewport.x, screenCoords.y * m_viewport.y);

            // Get a smooth interpolator with hermite
            f32 interpolator = hermite(hoverTime);

            // Calculate colors
            color4 color;
            // If its selected we use a different color
            bool isSelected = false;
            if (m_systemViewer->getTargetBody() == it.first) {
                isSelected = true;
             //   ui8Color = ui8v3(m_spaceSystem->pathColorMap["Selected"].second * 255.0f);
            } else {
                // Hermite interpolated color
                const std::pair<color4, color4>& pathColorRange = getPathColorRange(cmp.type);
                color.lerp(pathColorRange.first, pathColorRange.second, interpolator);
            }
            textColor.lerp(color::LightGray, color::White, interpolator);

            f32 selectorSize = bodyArData->selectorSize;

            // Only render if it isn't too big
            if (selectorSize < MainMenuSystemViewer::MAX_SELECTOR_SIZE) {

                // Alpha interpolation from size so they fade out
                f32 low = MainMenuSystemViewer::MAX_SELECTOR_SIZE * 0.7f;
                if (selectorSize > low) {
                    // Fade out when close
                    color.a = (ui8)((1.0f - (selectorSize - low) /
                        (MainMenuSystemViewer::MAX_SELECTOR_SIZE - low)) * 255);
                    textColor.a = color.a;
                } else {
                    f64 d = distance - (f64)low;
                    // Fade name based on distance
                    switch (cmp.type) {
                        case SpaceBodyType::STAR:
                            textColor.a = color.a = (ui8)(vmath::max(0.0, (f64)textColor.a - d * 0.00000000001));
                            break;
                        case SpaceBodyType::BARYCENTER:
                        case SpaceBodyType::PLANET:
                        case SpaceBodyType::DWARF_PLANET:
                            textColor.a = color.a = (ui8)(vmath::max(0.0, (f64)textColor.a - d * 0.000000001));
                            break;
                        default:
                            textColor.a = color.a = (ui8)(vmath::max(0.0, (f64)textColor.a - d * 0.000001));
                            break;
                    }
                }

                // Pick texture
                VGTexture tx;
                if (cmp.type == SpaceBodyType::BARYCENTER) {
                    tx = m_baryTexture;
                    selectorSize = MainMenuSystemViewer::MIN_SELECTOR_SIZE * 2.5f - (f32)(distance * 0.00000000001);
                    if (selectorSize < 0.0) continue;        
                    interpolator = 0.0f; // Don't rotate barycenters
                } else {
                    tx = m_selectorTexture;
                }

                // Draw Indicator
                m_spriteBatch->draw(tx, nullptr, nullptr,
                                    xyScreenCoords,
                                    f32v2(0.5f, 0.5f),
                                    f32v2(selectorSize),
                                    interpolator * ROTATION_FACTOR,
                                    color, screenCoords.z);

                // Text offset and scaling
                const f32v2 textOffset(selectorSize / 2.0f, -selectorSize / 2.0f);
                const f32v2 textScale((((selectorSize - MainMenuSystemViewer::MIN_SELECTOR_SIZE) /
                    (MainMenuSystemViewer::MAX_SELECTOR_SIZE - MainMenuSystemViewer::MIN_SELECTOR_SIZE)) * 0.5f + 0.5f) * 0.6f);

                // Draw Text
                if (textColor.a > 0) {
                    m_spriteBatch->drawString(m_spriteFont,
                                              cmp.name.c_str(),
                                              xyScreenCoords + textOffset,
                                              textScale,
                                              textColor,
                                              vg::TextAlign::TOP_LEFT,
                                              screenCoords.z);
                }

            }
            // Land selector
            if (isSelected && bodyArData->isLandSelected) {
                f32v3 selectedPos = bodyArData->selectedPos;
   
                f64q rot = cmp.currentOrientation;
                selectedPos = f32v3(rot * f64v3(selectedPos));

                relativePos = (position + f64v3(selectedPos)) - m_camera->getPosition();
                // Bring it close to the camera so it doesn't get occluded by anything
                relativePos = vmath::normalize(relativePos) * ((f64)m_camera->getNearClip() + 0.0001);
                screenCoords = m_camera->worldToScreenPointLogZ(relativePos, (f64)m_camera->getFarClip());
                xyScreenCoords = f32v2(screenCoords.x * m_viewport.x, screenCoords.y * m_viewport.y);

                color4 sColor = color::Red;
                sColor.a = 155;
                m_spriteBatch->draw(m_selectorTexture, nullptr, nullptr,
                                    xyScreenCoords,
                                    f32v2(0.5f, 0.5f),
                                    f32v2(22.0f) + cos(dt * 8.0f) * 4.0f,
                                    dt * ROTATION_FACTOR,
                                    sColor, screenCoords.z);
            }
        }
    }

    m_spriteBatch->end();
    m_spriteBatch->render(m_viewport, nullptr, &vg::DepthState::READ, nullptr);
}

const std::pair<color4, color4>& SpaceSystemARRenderer::getPathColorRange(SpaceBodyType type) {
    auto& it = m_pathColorMap.find((int)type);
    if (it != m_pathColorMap.end()) {
        return it->second;
    } else {
        return m_defaultPathColor;
    }
}

OrbitPathRenderData& SpaceSystemARRenderer::getOrbitPathPathRenderData(SpaceBodyComponent& cmp, vecs::ComponentID cmpID) {
    auto& it = m_renderDatas.find(cmpID);
    if (it != m_renderDatas.end()) {
        return it->second;
    } else {
        return m_renderDatas.insert(std::make_pair(cmpID, OrbitPathRenderData())).first->second;
    }
}