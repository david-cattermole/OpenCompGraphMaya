/*
 * Copyright (C) 2020, 2021 David Cattermole.
 *
 * This file is part of OpenCompGraphMaya.
 *
 * OpenCompGraphMaya is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OpenCompGraphMaya is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenCompGraphMaya.  If not, see <https://www.gnu.org/licenses/>.
 * ====================================================================
 *
 * Viewport 2.0 MShaderInstance wrapper class.
 */

// Maya
#include <maya/MString.h>
#include <maya/MColor.h>
#include <maya/MFloatMatrix.h>
#include <maya/M3dView.h>
#include <maya/MGlobal.h>

// Maya Viewport 2.0
#include <maya/MShaderManager.h>
#include <maya/MStateManager.h>

// STL
#include <memory>
#include <cstdlib>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "constant_texture_data.h"
#include "image_plane_shader.h"
#include "logger.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

Shader::Shader() : m_shader(nullptr) {}

Shader::~Shader() {
    auto log = log::get_logger();
    log->debug("ocgImagePlane: Releasing shader...");
    if (m_shader == nullptr) {
        return;
    }

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        log->error("ocgImagePlane: Failed to get renderer.");
        return;
    }

    const MHWRender::MShaderManager *shader_manager = renderer->getShaderManager();
    if (!shader_manager) {
        log->error("ocgImagePlane: Failed to get shader manager.");
        return;
    }

    if (!m_shader) {
        log->error("ocgImagePlane: Failed to release shader.");
        return;
    }

    shader_manager->releaseShader(m_shader);
    return;
}

MShaderInstance* Shader::instance() const noexcept {
    return this->m_shader;
};


const MHWRender::MShaderManager*
Shader::get_shader_manager() {
    auto log = log::get_logger();

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        log->error("ocgImagePlane: failed to get renderer.");
        return nullptr;
    }

    // If not core profile: ogsfx is not available save effect name
    // and leave.
    if (renderer->drawAPI() != MHWRender::kOpenGLCoreProfile) {
        log->warn("ocgImagePlane is only supported with OpenGL Core Profile!");
        return nullptr;
    }

    const MHWRender::MShaderManager *shader_manager = renderer->getShaderManager();
    if (!shader_manager) {
        log->error("ocgImagePlane failed get shader manager.");
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
    MStatus status = MS::kSuccess;
    M3dView view = M3dView::active3dView(&status);
    if (status != MStatus::kSuccess) {
        return nullptr;
    }
    view.makeSharedContextCurrent();
    return shader_manager;
}


MStatus
Shader::compile_stock_3d_shader() {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;
    if (m_shader != nullptr) {
        return status;
    } else {
        log->debug("ocgImagePlane compiling stock 3d shader...");
    }

    auto shader_manager = get_shader_manager();
    if (shader_manager == nullptr) {
        return MS::kFailure;
    }

    m_shader = shader_manager->getStockShader(
        MHWRender::MShaderManager::k3dSolidShader);
    return MS::kSuccess;
}


MStatus
Shader::compile_file(const MString shader_file_name) {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;
    if (m_shader != nullptr) {
        return status;
    } else {
        log->debug("ocgImagePlane compiling shader file...");
    }

    auto shader_manager = get_shader_manager();
    if (shader_manager == nullptr) {
        return MS::kFailure;
    }

    MString shader_location;
    MString cmd = MString("getModulePath -moduleName \"OpenCompGraphMaya\";");
    if (!MGlobal::executeCommand(cmd, shader_location, false)) {
        MString warning_message = MString(
            "ocgImagePlane: Could not get module path, looking up env var.");
        MGlobal::displayWarning(warning_message);
        shader_location = MString(std::getenv("OPENCOMPGRAPHMAYA_LOCATION"));
    }
    shader_location += MString("/shader");
    MString shader_path_message = MString(
        "ocgImagePlane: Shader path is ") + shader_location;
    MGlobal::displayWarning(shader_path_message);
    shader_manager->addShaderPath(shader_location);

    // Shader compiling options.
    MShaderCompileMacro *macros = nullptr;
    unsigned int number_of_macros = 0;
    bool use_effect_cache = true;

    // Get Techniques.
    log->debug("ocgImagePlane: Get techniques...");
    MStringArray technique_names;
    shader_manager->getEffectsTechniques(
        shader_file_name,
        technique_names,
        macros, number_of_macros,
        use_effect_cache);
    for (uint32_t i = 0; i < technique_names.length(); ++i) {
        log->debug("ocgImagePlane: technique{}: {}", i, technique_names[i].asChar());
    }
    if (technique_names.length() == 0) {
        log->error("ocgImagePlane: shader contains no techniques.");
        return MS::kFailure;
    }

    // Compile shader.
    log->debug("ocgImagePlane: Compiling shader...");
    const MString technique_name(technique_names[0]);  // pick first technique.
    m_shader = shader_manager->getEffectsFileShader(
        shader_file_name, technique_name,
        macros, number_of_macros,
        use_effect_cache);
    if (!m_shader) {
        MString error_message = MString(
            "ocgImagePlane failed to compile shader.");
        bool display_line_number = true;
        bool filter_source = true;
        uint32_t num_lines = 3;
        MGlobal::displayError(error_message);
        MGlobal::displayError(shader_manager->getLastError());
        MGlobal::displayError(shader_manager->getLastErrorSource(
                                  display_line_number, filter_source, num_lines));
        log->error("ocgImagePlane failed to compile shader.");
        log->error(shader_manager->getLastError().asChar());
        log->error(shader_manager->getLastErrorSource(
                       display_line_number, filter_source, num_lines).asChar());
        return MS::kFailure;
    }

    MStringArray parameter_list;
    m_shader->parameterList(parameter_list);
    for (uint32_t i = 0; i < parameter_list.length(); ++i) {
        log->debug(
            "ocgImagePlane: param {}: {}", i, parameter_list[i].asChar());
    }

    return status;
}


bool Shader::is_transparent() const {
    return m_shader->isTransparent();
}


MStatus Shader::set_is_transparent(const bool value) {
    return m_shader->setIsTransparent(value);
}


// Set a boolean parameter.
MStatus
Shader::set_bool_param(
        const MString parameter_name,
        const bool value) {
    MStatus status = m_shader->setParameter(
        parameter_name,
        value);
    if (status != MStatus::kSuccess) {
        auto log = log::get_logger();
        log->error("ocgImagePlane: Failed to set boolean parameter!");
    }
    return status;
}

// Set a signed integer 32-bit parameter.
MStatus
Shader::set_int_param(
        const MString parameter_name,
        const int32_t value) {
    MStatus status = m_shader->setParameter(
        parameter_name,
        value);
    if (status != MStatus::kSuccess) {
        auto log = log::get_logger();
        log->error("ocgImagePlane: Failed to set integer parameter!");
    }
    return status;
}

// Set a color parameter.
MStatus
Shader::set_color_param(
        const MString parameter_name,
        const float color_values[4]) {
    MStatus status = m_shader->setParameter(
        parameter_name,
        color_values);
    if (status != MStatus::kSuccess) {
        auto log = log::get_logger();
        log->error("ocgImagePlane: Failed to set color parameter!");
    }
    return status;
}

// Set the named matrix parameter on the shader.
MStatus
Shader::set_float_matrix4x4_param(
        const MString parameter_name,
        const MFloatMatrix matrix) {
    MStatus status = m_shader->setParameter(
            parameter_name,
            matrix);
    if (status != MStatus::kSuccess) {
        auto log = log::get_logger();
        log->error("ocgImagePlane: Failed to set Matrix 4x4 parameter!");
    }
    return status;
}

MStatus
Shader::set_texture_param_with_image_data(
        const MString parameter_name,
        const MHWRender::MTextureType texture_type,
        const int32_t pixel_width,
        const int32_t pixel_height,
        const int32_t pixel_depth,
        const int32_t pixel_num_channels,
        const ocg::PixelDataType pixel_data_type,
        const void* buffer) {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        log->error("ocgImagePlane: Failed to get renderer.");
        return MS::kFailure;
    }

    MHWRender::MTextureManager* texture_manager =
        renderer->getTextureManager();
    if (!texture_manager) {
        log->error("ocgImagePlane: Failed to get texture manager.");
        return MS::kFailure;
    }

    // Upload Texture data to the GPU using Maya's API.
    //
    // See for details of values:
    // http://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_class_m_h_w_render_1_1_texture_description_html
    MHWRender::MTextureDescription texture_description;
    texture_description.setToDefault2DTexture();
    texture_description.fWidth = pixel_width;
    texture_description.fHeight = pixel_height;
    texture_description.fDepth = pixel_depth;
    texture_description.fTextureType = texture_type;
    texture_description.fMipmaps = 1;

    if (pixel_data_type == ocg::PixelDataType::kUInt8) {
        // log->warn("ocg::PixelDataType::kUInt8");
        // log->warn("MHWRender::kR8G8B8A8_UNORM");
        texture_description.fFormat = MHWRender::kR8G8B8A8_UNORM;

    } else if (pixel_data_type == ocg::PixelDataType::kHalf16) {
        // log->warn("ocg::PixelDataType::kHalf16");
        // log->warn("MHWRender::kR16G16B16A16_FLOAT");
        texture_description.fFormat = MHWRender::kR16G16B16A16_FLOAT;

    } else if (pixel_data_type == ocg::PixelDataType::kUInt16) {
        // log->warn("ocg::PixelDataType::kUInt16");
        // log->warn("MHWRender::kR16G16B16A16_UINT");
        texture_description.fFormat = MHWRender::kR16G16B16A16_UINT;

    } else if (pixel_data_type == ocg::PixelDataType::kFloat32) {
        // log->warn("ocg::PixelDataType::kFloat32");
        if (pixel_num_channels == 3) {
            texture_description.fFormat = MHWRender::kR32G32B32_FLOAT;
            // log->warn("MHWRender::kR32G32B32_FLOAT");
        } else if (pixel_num_channels == 4) {
            // log->warn("MHWRender::kR32G32B32A32_FLOAT");
            texture_description.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        } else {
            log->error("ocgImagePlane: Invalid number of channels in image.");
            return MS::kFailure;
        }

    } else {
        log->error(
            "ocgImagePlane: Invalid image pixel data type: {}",
            pixel_data_type);
        return MS::kFailure;
    }

    // Using an empty texture name by-passes the MTextureManager's
    // inbuilt caching system - or values are not remembered by
    // Maya, we must store a cache.
    MTexture *texture = texture_manager->acquireTexture(
        /*textureName=*/ "",
        texture_description,
        buffer,
        /*generateMipMaps=*/ false);

    if (!texture) {
        log->error("ocgImagePlane: Failed to acquire texture.");
        return MS::kFailure;
    }

    // Set the shader's texture parameter to use our uploaded texture.
    log->debug("ocgImagePlane: Setting texture parameter...");
    MHWRender::MTextureAssignment texture_resource;
    texture_resource.texture = texture;
    m_shader->setParameter(parameter_name, texture_resource);
    // Release our reference now that it is set on the shader
    texture_manager->releaseTexture(texture);
    return status;
}

MStatus
Shader::set_texture_param_with_stream_data(
        const MString parameter_name,
        ocg::StreamData stream_data) {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;

    auto pixel_buffer = stream_data.pixel_buffer();
    auto pixel_width = stream_data.pixel_width();
    auto pixel_height = stream_data.pixel_height();
    auto pixel_depth = 1;  // We do not support 3D textures.
    auto pixel_num_channels = stream_data.pixel_num_channels();
    auto pixel_data_type = stream_data.pixel_data_type();
    // auto channel_num_bytes = ocg::internal::channel_size_bytes(pixel_data_type);

    // log->warn("pixels: {}x{} c={}",
    //           pixel_width,  pixel_height,
    //           static_cast<uint32_t>(pixel_num_channels));
    auto buffer = static_cast<const void*>(pixel_buffer.data());

    return set_texture_param_with_image_data(
        parameter_name,
        MHWRender::kImage2D,
        pixel_width,
        pixel_height,
        pixel_depth,
        pixel_num_channels,
        pixel_data_type,
        buffer);
}

// Acquire and bind the default texture sampler.
MStatus
Shader::set_texture_sampler_param(
        const MString parameter_name,
        MHWRender::MSamplerStateDesc sampler_description) {
    auto log = log::get_logger();
    const MHWRender::MSamplerState* sampler =
        MHWRender::MStateManager::acquireSamplerState(sampler_description);
    if (sampler) {
        log->debug("ocgImagePlane: Setting texture sampler parameter...");
        m_shader->setParameter(parameter_name, *sampler);
    } else {
        log->error("ocgImagePlane: Failed to get texture sampler.");
        return MS::kFailure;
    }
    return MS::kSuccess;
}

} // namespace image_plane
} // namespace open_comp_graph_maya
