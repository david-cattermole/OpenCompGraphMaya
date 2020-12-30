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
#include <memory>
#include <cstdlib>

// OCG
#include "opencompgraph.h"


// OCG Maya
#include "shader.h"

namespace ocg = open_comp_graph;

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
    const float colorValues[4] = {1.0f, 1.0f, 1.0f, 1.0f};
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

    // MTextureManager Caching Behaviour
    //
    // When using the MTextureManager::acquire* methods a cache is
    // used to look up existing textures.
    //
    // If the texure name provided is an empty string then the texture
    // will not be cached as part of the internal texture caching
    // system. Thus each such call to this method will create a new
    // texture.
    //
    // If a non-empty texture name is specified then the caching
    // system will attempt to return any previously cached texture
    // with that name.
    //
    // The renderer will add 1 reference to this texture on
    // creation. If the texture has already been acquired then no new
    // texture will be created, and a new reference will be added. To
    // release the reference, call releaseTexture().
    //
    // If no pre-existing cached texture exists, then a new texture is
    // created by tiling a set of images on disk. The images are
    // specified by a set of file names and their tile position. The
    // input images must be 2D textures.
    //

    // MString textureLocation("C:/Users/user/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data");
    // textureManager->addImagePath(textureLocation);
    // // Load texture onto shader, using Maya's image loading libraries.
    // int mipmapLevels = 1;  // 1 = Only one mip-map level, don't create more.
    // bool useExposureControl = true;
    // MString textureName("checker_8bit_rgba_8x8.png");
    // const MString contextNodeFullName("ocgImagePlane1");
    // MHWRender::MTexture* texture =
    //     textureManager->acquireTexture(
    //         textureName,
    //         contextNodeFullName,
    //         mipmapLevels,
    //         useExposureControl);

    // Upload Texture data to the GPU using Maya's API.
    if (!imagePlaneTexture_)
    {
        // First, search the texture cache to see if another instance
        // of this override has already generated the texture. We can
        // reuse it to save GPU memory since the noise data is
        // constant.
        imagePlaneTexture_ =
            textureManager->findTexture(imagePlaneTextureName_);
        // Not in cache, so we need to actually build the texture.
        if (!imagePlaneTexture_)
        {
            // // Create a 2D texture with the hard-coded data.
            // //
            // // See:
            // // http://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_class_m_h_w_render_1_1_m_texture_description_html
            // MHWRender::MTextureDescription desc;
            // desc.setToDefault2DTexture();
            // desc.fWidth = 4;
            // desc.fHeight = 4;
            // desc.fFormat = MHWRender::kR32G32B32_FLOAT;
            // desc.fMipmaps = 1;
            // imagePlaneTexture_ = textureManager->acquireTexture(
            //     imagePlaneTextureName_,
            //     desc,
            //     (const void*)&(colorBars_f32_8x8_[0]),
            //     false);

            auto graph = ocg::Graph();
            auto read_node = ocg::Node(ocg::NodeType::kReadImage, "read1");
            auto grade_node = ocg::Node(ocg::NodeType::kGrade, "grade1");
            // read_node.set_attr_str("file_path", "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/checker_8bit_rgba_3840x2160.png");
            read_node.set_attr_str("file_path", "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/oiio-images/tahoe-gps.jpg");
            grade_node.set_attr_f32("multiply", 1.0f);
            auto read_node_id = graph.add_node(read_node);
            auto grade_node_id = graph.add_node(grade_node);
            graph.connect(read_node_id, grade_node_id, 0);

            auto cache = std::make_shared<ocg::Cache>();
            auto exec_status = graph.execute(grade_node_id, cache);
            if (exec_status == ocg::ExecuteStatus::kSuccess) {
                auto stream_data = graph.output_stream();
                auto pixel_buffer = stream_data.pixel_buffer();
                auto pixel_width = stream_data.pixel_width();
                auto pixel_height = stream_data.pixel_height();
                auto pixel_num_channels = stream_data.pixel_num_channels();
                MStreamUtils::stdErrorStream()
                        << "pixels: "
                        << pixel_width << "x"
                        << pixel_height << "x"
                        << static_cast<uint32_t>(pixel_num_channels)
                        << " | data=" << &pixel_buffer << '\n';
                // auto buffer = static_cast<const void*>(colorBars_f32_8x8_[0]),
                auto buffer = static_cast<const void*>(pixel_buffer.data());

                // Upload Texture via Maya.
                MHWRender::MTextureDescription desc;
                desc.setToDefault2DTexture();
                desc.fWidth = pixel_width;
                desc.fHeight = pixel_height;
                if (pixel_num_channels == 4) {
                    desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
                } else {
                    desc.fFormat = MHWRender::kR32G32B32_FLOAT;
                }
                desc.fMipmaps = 1;
                imagePlaneTexture_ = textureManager->acquireTexture(
                    imagePlaneTextureName_,
                    desc,
                    buffer,
                    false);
            }
        }
    }

    // Set the shader's texture parameter to use our uploaded texture.
    if (imagePlaneTexture_) {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Setting texture parameter...\n";
        MHWRender::MTextureAssignment texResource;
        texResource.texture = imagePlaneTexture_;
        imagePlaneShader->setParameter(
            textureParameterName_,
            texResource);
        // Release our reference now that it is set on the shader
        textureManager->releaseTexture(imagePlaneTexture_);
    } else {
        MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Failed to acquire texture." << '\n';
        MString errorMsg = MString(
            "ocgImagePlane: Failed to acquire texture!");
        MGlobal::displayError(errorMsg);
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
