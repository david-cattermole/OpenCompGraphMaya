#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H

// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// STL
#include <map>
#include <unordered_map>

namespace open_comp_graph_maya{

// Viewport 2.0 specific data
const MString colorParameterName_ = "gSolidColor";
const MString textureParameterName_ = "gTexture";
const MString textureSamplerParameterName_ = "gTextureSampler";
const MString wireframeItemName_ = "ocgImagePlaneWireframe";
const MString shadedItemName_ = "ocgImagePlaneShadedTriangles";

/*
 * Color Bars Texture, for debug.
 *
 * https://en.wikipedia.org/wiki/SMPTE_color_bars
 *
 * ------------------------------
 *  R  -  G  -  B   - COLOR NAME
 * ------------------------------
 * 235 - 235 - 235  - 100% White
 * 180 - 180 - 180  - 75% White
 * 235 - 235 - 16   - Yellow
 * 16  - 235 - 235  - Cyan
 * 16  - 235 - 16   - Green
 * 235 - 16  - 235  - Magenta
 * 235 - 16  - 16   - Red
 * 16  - 16  - 235  - Blue
 * 16  - 16  - 16   - Black
 * ------------------------------
 *
 * The texture block (below) starts at the lower-left (zeroth index)
 * and continues the upper-right (last index).
 *
 * Note to make things even (only 8 entries), we skip the 75% white.
 */
static const float colorBars_f32_8x8_[] = {
    // Row 0
    //
    // 235, 16, 235  - Magenta
    0.9215f, 0.0627f, 0.9215f,

    // 235, 16, 16   - Red
    0.9215f, 0.0627f, 0.0627f,

    // 16, 16, 235   - Blue
    0.0627f, 0.0627f, 0.9215f,

    // 16, 16, 16    - Black
    0.0627f, 0.0627f, 0.0627f,

    // Row 1
    //
    // 235, 16, 235  - Magenta
    0.9215f, 0.0627f, 0.9215f,

    // 235, 16, 16   - Red
    0.9215f, 0.0627f, 0.0627f,

    // 16, 16, 235   - Blue
    0.0627f, 0.0627f, 0.9215f,

    // 16, 16, 16    - Black
    0.0627f, 0.0627f, 0.0627f,

    // Row 2
    //
    // 235, 235, 235 - 100% White
    0.9215f, 0.9215f, 0.9215f,

    // 235, 235, 16  - Yellow
    0.9215f, 0.9215f, 0.0627f,

    // 16, 235, 235  - Cyan
    0.0627f, 0.9215f, 0.9215f,

    // 16, 235, 16   - Green
    0.0627f, 0.9215f, 0.0627f,

    // Row 3
    //
    // 235, 235, 235 - 100% White
    0.9215f, 0.9215f, 0.9215f,

    // 235, 235, 16  - Yellow
    0.9215f, 0.9215f, 0.0627f,

    // 16, 235, 235  - Cyan
    0.0627f, 0.9215f, 0.9215f,

    // 16, 235, 16   - Green
    0.0627f, 0.9215f, 0.0627f
};
static const int colorBars_f32_8x8_count_ = 16;

// struct MColorHash {
//     std::size_t operator()(const MColor &color) const;
//     void CombineHashCode(std::size_t &seed, float v) const;
// };
// MHWRender::MShaderInstance *get3dSolidShader(const MColor &color);
// MStatus releaseShaders();
// static std::unordered_map<MColor, MHWRender::MShaderInstance *, MColorHash> the3dSolidShaders;

static MHWRender::MShaderInstance *imagePlaneShader = nullptr;
static MHWRender::MTexture *imagePlaneTexture_ = nullptr;
const MString imagePlaneTextureName_ = "MyColorBarsTexture";
MHWRender::MShaderInstance *getImagePlaneShader();
MStatus releaseImagePlaneShader();

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H
