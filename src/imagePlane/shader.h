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

// OCG Maya
#include "utils/unused.h"

namespace open_comp_graph_maya{

// Viewport 2.0 specific data
const MString colorParameterName_ = "solidColor";
const MString wireframeItemName_ = "ocgImagePlaneLocatorWires";
const MString shadedItemName_ = "ocgImagePlaneLocatorTriangles";

struct MColorHash {
    std::size_t operator()(const MColor &color) const;
    void CombineHashCode(std::size_t &seed, float v) const;
};

static std::unordered_map<MColor, MHWRender::MShaderInstance *, MColorHash> the3dSolidShaders;

MHWRender::MShaderInstance *get3dSolidShader(const MColor &color);

MStatus releaseShaders();

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H
