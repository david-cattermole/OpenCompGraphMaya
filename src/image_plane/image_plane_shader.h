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

#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H

// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MFloatMatrix.h>
#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// STL
#include <map>
#include <memory>

// OCG
#include <opencompgraph.h>


namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

class Shader {
public:

    Shader();
    ~Shader();

    MHWRender::MShaderInstance* instance() const noexcept;

    MStatus compile_stock_3d_shader();
    MStatus compile_file(const MString shader_file_name);

    // Is the shader treated as transparent?
    bool is_transparent() const;
    MStatus set_is_transparent(const bool value);

    // Set Parameters
    MStatus set_bool_param(
        const MString parameter_name,
        const bool value);

    MStatus set_int_param(
        const MString parameter_name,
        const int32_t value);

    MStatus set_color_param(
        const MString parameter_name,
        const float color_values[4]);

    MStatus set_float_matrix4x4_param(
        const MString parameter_name,
        const MFloatMatrix matrix);

    MStatus set_texture_sampler_param(
        const MString parameter_name,
        MHWRender::MSamplerStateDesc sampler_description);

    MStatus set_texture_param_with_stream_data(
        const MString parameter_name,
        ocg::StreamData stream_data);

private:
    const MHWRender::MShaderManager* get_shader_manager();

    MHWRender::MShaderInstance *m_shader;
};

} // namespace image_plane
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHADER_H
