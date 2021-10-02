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
 * Image Plane Viewport 2.0 MPxGeometryOverride implementation.
 */

#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_OVERRIDE_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_OVERRIDE_H

// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MPxGeometryOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// STL
#include <map>
#include <memory>
#include <string>

// OCG
#include <opencompgraph.h>

// OCG Maya
#include "image_plane_geometry_canvas.h"
#include "image_plane_geometry_window.h"
#include "image_plane_shader.h"


namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

class GeometryOverride : public MHWRender::MPxGeometryOverride {
public:

    static MHWRender::MPxGeometryOverride *Creator(const MObject &obj) {
        return new GeometryOverride(obj);
    }

    ~GeometryOverride() override;

    MHWRender::DrawAPI supportedDrawAPIs() const override {
        return MHWRender::kAllDevices;
    }

    void updateDG() override;

    bool isIndexingDirty(const MHWRender::MRenderItem &item) override {
        return true;
    }

    bool
    isStreamDirty(const MHWRender::MVertexBufferDescriptor &desc) override {
        return true;
    }

    void updateRenderItems(
        const MDagPath &path,
        MHWRender::MRenderItemList &list) override;

    void populateGeometry(
        const MHWRender::MGeometryRequirements &requirements,
        const MHWRender::MRenderItemList &renderItems,
        MHWRender::MGeometry &data) override;

    // Delete any cached data generated in the earlier phases that is
    // no longer needed.
    void cleanUp() override {};

    bool hasUIDrawables() const override {
        return true;
    }

    void addUIDrawables(
        const MDagPath &path,
        MHWRender::MUIDrawManager &draw_manager,
        const MHWRender::MFrameContext &frameContext) override;

private:

    GeometryOverride(const MObject &obj);

    MStatus updateWithStream(
        std::shared_ptr<ocg::Graph> &shared_graph,
        ocg::StreamData &stream_data);

    GeometryCanvas m_geometry_canvas;
    GeometryWindow m_geometry_window_display;
    GeometryWindow m_geometry_window_data;

    // Shaders
    Shader m_shader_wire;
    Shader m_shader_border;
    Shader m_shader;
    Shader m_shader_display_window;
    Shader m_shader_data_window;

    // Shader Constants
    static MString m_shader_color_parameter_name;
    static MString m_shader_geometry_transform_parameter_name;
    static MString m_shader_rescale_transform_parameter_name;
    static MString m_shader_display_mode_parameter_name;
    static MString m_shader_image_transform_parameter_name;
    static MString m_shader_image_color_matrix_parameter_name;
    static MString m_shader_image_texture_parameter_name;
    static MString m_shader_image_texture_sampler_parameter_name;
    static MString m_shader_3d_lut_enable_parameter_name;
    static MString m_shader_3d_lut_edge_size_parameter_name;
    static MString m_shader_3d_lut_texture_parameter_name;
    static MString m_shader_3d_lut_texture_sampler_parameter_name;
    static MString m_shader_color_ops_lut_enable_parameter_name;
    static MString m_shader_color_ops_lut_edge_size_parameter_name;
    static MString m_shader_color_ops_1d_lut_texture_parameter_name;
    static MString m_shader_color_ops_1d_lut_texture_sampler_parameter_name;
    static MString m_shader_color_ops_3d_lut_texture_parameter_name;
    static MString m_shader_color_ops_3d_lut_texture_sampler_parameter_name;

    // Internal state.
    MObject m_locator_node;
    bool m_update_vertices;
    bool m_update_topology;
    bool m_update_shader;
    bool m_update_shader_border;
    ocg::ExecuteStatus m_exec_status;

    // Cached attribute values
    float m_focal_length;
    uint8_t m_display_mode;
    float m_card_depth;
    float m_card_size_x;
    float m_card_size_y;
    uint32_t m_card_res_x;
    uint32_t m_card_res_y;
    float m_time;
    uint32_t m_lut_edge_size;
    std::string m_from_color_space_name;
    MString m_color_space_name;
    uint8_t m_cache_option;
    bool m_cache_crop_on_format;
    bool m_disk_cache_enable;
    MString m_disk_cache_file_path;
    ocg::Node m_in_stream_node;
    ocg::Node m_viewer_node;
    ocg::Node m_read_cache_node;
    int m_display_window_width;
    int m_display_window_height;
    int m_data_window_min_x;
    int m_data_window_min_y;
    int m_data_window_max_x;
    int m_data_window_max_y;

    // Viewport 2.0 render item names
    static MString m_data_window_render_item_name;
    static MString m_display_window_render_item_name;
    static MString m_border_render_item_name;
    static MString m_wireframe_render_item_name;
    static MString m_shaded_render_item_name;

    // Viewport 2.0 geometry stream names
    static MString m_canvas_stream_name;
    static MString m_display_window_stream_name;
    static MString m_data_window_stream_name;
};

} // namespace image_plane
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_OVERRIDE_H
