// Maya
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>
#include <maya/MSelectionContext.h>
#include <maya/MStreamUtils.h>
#include <Maya/M3dView.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MStateManager.h>

// STL
#include <unordered_map>
#include <cstdlib>

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
        // MStreamUtils::stdErrorStream()
        //     << "ocgImagePlane, found shader!\n";
        return imagePlaneShader;
    } else {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane compiling shader...\n";
    }

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        MString errorMsg = MString(
            "ocgImagePlane failed to get renderer.");
        MGlobal::displayError(errorMsg);
        return nullptr;
    }

    // If not core profile: ogsfx is not available save effect name
    // and leave.
    if (renderer->drawAPI() != MHWRender::kOpenGLCoreProfile) {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane is only supported with OpenGL Core Profile!\n";
        return nullptr;
    }

    const MHWRender::MShaderManager *shaderMgr = renderer->getShaderManager();
    if (!shaderMgr) {
        MString errorMsg = MString(
            "ocgImagePlane failed to get shader manager.");
        MGlobal::displayError(errorMsg);
        return nullptr;
    }

    // In core profile, there used to be the problem where the shader
    // fails to load sometimes.  The problem occurs when the OpenGL
    // Device Context is switched before calling the
    // GLSLShaderNode::loadEffect() function(this switch is performed
    // by Tmodel::selectManip).  When that occurs, the shader is
    // loaded in the wrong context instead of the viewport
    // context... so that in the draw phase, after switching to the
    // viewport context, the drawing is erroneous.  In order to solve
    // that problem, make the view context current
    MStatus status;
    M3dView view = M3dView::active3dView(&status);
    if (status != MStatus::kSuccess) {
        return nullptr;
    }
    view.makeSharedContextCurrent();

    MString shaderLocation;
    MString cmd = MString("getModulePath -moduleName \"OpenCompGraphMaya\";");
    if( !MGlobal::executeCommand(cmd, shaderLocation, false) ) {
        MString warnMsg = MString(
            "ocgImagePlane: Could not get module path, looking up env var.");
        MGlobal::displayWarning(warnMsg);
        shaderLocation = MString(std::getenv("OPENCOMPGRAPHMAYA_LOCATION"));
    }
    shaderLocation += MString("/shader");
    MString shaderPathMsg = MString(
            "ocgImagePlane: Shader path is ") + shaderLocation;
    MGlobal::displayWarning(shaderPathMsg);
    shaderMgr->addShaderPath(shaderLocation);

    // Shader compiling options.
    const MString effectsFileName("ocgImagePlane");
    MShaderCompileMacro *macros = nullptr;
    unsigned int numberOfMacros = 0;
    bool useEffectCache = true;

    // Get Techniques.
    MStreamUtils::stdErrorStream() << "ocgImagePlane: Get techniques...\n";
    MStringArray techniqueNames;
    shaderMgr->getEffectsTechniques(
        effectsFileName,
        techniqueNames,
        macros, numberOfMacros,
        useEffectCache);
    for (uint32_t i = 0; i < techniqueNames.length(); ++i) {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: technique"
            << i << ": " << techniqueNames[i].asChar() << '\n';
    }
    if (techniqueNames.length() == 0) {
        MString errorMsg = MString(
            "ocgImagePlane shader contains no techniques!");
        MGlobal::displayError(errorMsg);
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: shader contains no techniques..\n";
        return nullptr;
    }

    // Compile shader.
    MStreamUtils::stdErrorStream() << "ocgImagePlane: Compiling shader...\n";
    const MString techniqueName(techniqueNames[0]);  // pick first technique.
    imagePlaneShader = shaderMgr->getEffectsFileShader(
        effectsFileName, techniqueName,
        macros, numberOfMacros,
        useEffectCache);
    if (!imagePlaneShader) {
        MString errorMsg = MString(
            "ocgImagePlane failed to compile shader.");
        bool displayLineNumber = true;
        bool filterSource = true;
        uint32_t numLines = 2;
        MGlobal::displayError(errorMsg);
        MGlobal::displayError(shaderMgr->getLastError());
        MGlobal::displayError(shaderMgr->getLastErrorSource(
                                  displayLineNumber, filterSource, numLines));
        return nullptr;
    }
    MStringArray paramlist;
    imagePlaneShader->parameterList(paramlist);
    for (uint32_t i = 0; i < paramlist.length(); ++i) {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: param" << i << ": " << paramlist[i].asChar() << '\n';
    }

    // Set a color parameter.
    const float colorValues[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    status = imagePlaneShader->setParameter(
        colorParameterName_,
        colorValues);
    if (status != MStatus::kSuccess) {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Failed to set color parameter!" << '\n';
        MString errorMsg = MString(
            "ocgImagePlane: Failed to set color parameter.");
        MGlobal::displayError(errorMsg);
        return nullptr;
    }

    MHWRender::MTextureManager* textureManager =
        renderer->getTextureManager();
    if (!textureManager) {
        MString errorMsg = MString(
            "ocgImagePlane failed to get texture manager.");
        MGlobal::displayError(errorMsg);
        return nullptr;
    }
    MString textureLocation("C:/Users/user/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data");
    textureManager->addImagePath(textureLocation);

    // Load texture onto shader.
    int mipmapLevels = 1;  // 1 = Only one mip-map level, don't create more.
    bool useExposureControl = true;
    MString textureName("checker_8bit_rgba_8x8.png");
    const MString contextNodeFullName("ocgImagePlane1");
    MHWRender::MTexture* texture =
        textureManager->acquireTexture(
            textureName,
            contextNodeFullName,
            mipmapLevels,
            useExposureControl);
    if (texture) {
        MHWRender::MTextureAssignment texResource;
        texResource.texture = texture;
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Setting texture parameter...\n";
        imagePlaneShader->setParameter(
            textureParameterName_,
            texResource);

        // Release our reference now that it is set on the shader
        textureManager->releaseTexture(texture);
    } else {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Failed to acquire texture from "
            << textureName.asChar() << '\n';
        MString errorMsg = MString(
            "ocgImagePlane: Failed to acquire texture from ");
        MGlobal::displayError(errorMsg + textureName);
    }

    // Acquire and bind the default texture sampler.
    MHWRender::MSamplerStateDesc samplerDesc;
    samplerDesc.filter = MHWRender::MSamplerState::kMinMagMipPoint;
    samplerDesc.addressU = MHWRender::MSamplerState::kTexClamp;
    samplerDesc.addressV = MHWRender::MSamplerState::kTexClamp;
    const MHWRender::MSamplerState* sampler =
        MHWRender::MStateManager::acquireSamplerState(samplerDesc);
    if (sampler) {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Setting texture sampler parameter...\n";
        imagePlaneShader->setParameter(
            textureSamplerParameterName_,
            *sampler);
    } else {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Failed to get texture sampler.\n";
        MString errorMsg = MString(
            "ocgImagePlane: Failed to get texture sampler.");
        MGlobal::displayError(errorMsg);
        return nullptr;
    }

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
