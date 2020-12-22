// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>
#include <maya/MSelectionContext.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>

// STL
#include <unordered_map>

// OCG Maya
#include "shader.h"

namespace open_comp_graph_maya {

// Maintain a mini cache for 3d solid shaders in order to reuse the
// shader instance whenever possible. This can allow Viewport 2.0
// optimization e.g.  the GPU instancing system and the consolidation
// system to be leveraged.
std::size_t MColorHash::operator()(const MColor &color) const {
    std::size_t seed = 0;
    CombineHashCode(seed, color.r);
    CombineHashCode(seed, color.g);
    CombineHashCode(seed, color.b);
    CombineHashCode(seed, color.a);
    return seed;
}

void MColorHash::CombineHashCode(std::size_t &seed, float v) const {
    std::hash<float> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

MHWRender::MShaderInstance *get3dSolidShader(const MColor &color) {
    // Return the shader instance if exists.
    auto it = the3dSolidShaders.find(color);
    if (it != the3dSolidShaders.end()) {
        return it->second;
    }

    MHWRender::MShaderInstance *shader = NULL;

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
        if (shaderMgr) {
            shader = shaderMgr->getStockShader(
                MHWRender::MShaderManager::k3dSolidShader);
        }
    }

    if (shader) {
        float solidColor[] = {color.r, color.g, color.b, 1.0f};
        shader->setParameter(colorParameterName_, solidColor);

        the3dSolidShaders[color] = shader;
    }

    return shader;
}


MStatus releaseShaders() {
    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
        if (shaderMgr) {
            for (auto it = the3dSolidShaders.begin();
                 it != the3dSolidShaders.end(); it++) {
                shaderMgr->releaseShader(it->second);
            }

            the3dSolidShaders.clear();
            return MS::kSuccess;
        }
    }

    return MS::kFailure;
}
    
} // namespace open_comp_graph_maya 
