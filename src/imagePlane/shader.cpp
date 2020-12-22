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

// // Maintain a mini cache for 3d solid shaders in order to reuse the
// // shader instance whenever possible. This can allow Viewport 2.0
// // optimization e.g.  the GPU instancing system and the consolidation
// // system to be leveraged.
// std::size_t MColorHash::operator()(const MColor &color) const {
//     std::size_t seed = 0;
//     CombineHashCode(seed, color.r);
//     CombineHashCode(seed, color.g);
//     CombineHashCode(seed, color.b);
//     CombineHashCode(seed, color.a);
//     return seed;
// }

// void MColorHash::CombineHashCode(std::size_t &seed, float v) const {
//     std::hash<float> hasher;
//     seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
// }

// MHWRender::MShaderInstance *get3dSolidShader(const MColor &color) {
//     // Return the shader instance if exists.
//     auto it = the3dSolidShaders.find(color);
//     if (it != the3dSolidShaders.end()) {
//         return it->second;
//     }
//     MHWRender::MShaderInstance *shader = NULL;
//     MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
//     if (renderer) {
//         const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
//         if (shaderMgr) {
//             shader = shaderMgr->getStockShader(
//                 MHWRender::MShaderManager::k3dSolidShader);
//         }
//     }
//     if (shader) {
//         float solidColor[] = {color.r, color.g, color.b, 1.0f};
//         shader->setParameter(colorParameterName_, solidColor);
//         the3dSolidShaders[color] = shader;
//     }
//     return shader;
// }

// MStatus releaseShaders() {
//     MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
//     if (renderer) {
//         const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
//         if (shaderMgr) {
//             for (auto it = the3dSolidShaders.begin();
//                  it != the3dSolidShaders.end(); it++) {
//                 shaderMgr->releaseShader(it->second);
//             }
//             the3dSolidShaders.clear();
//             return MS::kSuccess;
//         }
//     }
//     return MS::kFailure;
// }


MHWRender::MShaderInstance *getImagePlaneShader() {
    if (imagePlaneShader != nullptr) {
        return imagePlaneShader;
    }

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
        if (shaderMgr) {
            const MString searchPath("C:\\Users\\user\\dev\\OpenCompGraphMaya\\shaders");
            shaderMgr->addShaderPath(searchPath);

            MShaderCompileMacro *macros = nullptr;
            unsigned int numberOfMacros = 0;
            bool useEffectCache = true;
            MShaderInstance::DrawCallback preCb = nullptr;
            MShaderInstance::DrawCallback postCb = nullptr;
            const MString effectsFileName("imagePlane");
            const MString techniqueName("T0");
            imagePlaneShader = shaderMgr->getEffectsFileShader(
                effectsFileName, techniqueName,
                macros, numberOfMacros, useEffectCache,
                preCb, postCb
            );
            if (!imagePlaneShader) {
                MGlobal::displayError(shaderMgr->getLastError());
                return nullptr;
            }
        }
    }

    // path += MString("/plug-ins/customSpriteShader/");
    // MHWRender::MTexture* texture =
    //     renderer->getTextureManager()->acquireTexture(path + MString("snow.png"), 1);
    // if (texture)
    // {
    //     MHWRender::MTextureAssignment texResource;
    //     texResource.texture = texture;
    //     fShaderInstance->setParameter("map", texResource);
    // }
    // else
    // {
    //     MString errorMsg = MString("customSpriteShader failed to acquire texture from ") + path + MString("snow.png");
    //     MGlobal::displayError(errorMsg);
    // }

    // // Acquire and bind the default texture sampler.
    // //
    // MHWRender::MSamplerStateDesc samplerDesc;
    // const MHWRender::MSamplerState* sampler =
    //     MHWRender::MStateManager::acquireSamplerState(samplerDesc);
    // if (sampler)
    // {
    //     fShaderInstance->setParameter("textureSampler", *sampler);
    // }

    // if (shader) {
    //     // float solidColor[] = {color.r, color.g, color.b, 1.0f};
    //     shader->setParameter("checker_8bit_rgba_8x8png", solidColor);
    //     the3dSolidShaders[color] = shader;
    // }

    return imagePlaneShader;
}


MStatus releaseImagePlaneShader() {
    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
        if (shaderMgr && imagePlaneShader) {
            shaderMgr->releaseShader(imagePlaneShader);
            return MS::kSuccess;
        }
    }
    return MS::kFailure;
}

} // namespace open_comp_graph_maya
