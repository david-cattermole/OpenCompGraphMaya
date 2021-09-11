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

// Maya
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MTime.h>
#include <maya/MMatrix.h>
#include <maya/MFloatMatrix.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnCamera.h>
#include <maya/MFnPluginData.h>
#include <maya/MDagMessage.h>
#include <maya/MSelectionContext.h>
#include <maya/M3dView.h>

// Maya Viewport 2.0
#include <maya/MPxGeometryOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MStateManager.h>

// STL
#include <cmath>
#include <memory>
#include <tuple>
#include <cstdlib>
#include <vector>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "constant_texture_data.h"
#include "image_plane_utils.h"
#include "image_plane_geometry_override.h"
#include "image_plane_shape.h"
#include "graph_data.h"
#include "graph_execute.h"
#include "global_cache.h"
#include "logger.h"

namespace ocg = open_comp_graph;
namespace ocgm_cache = open_comp_graph_maya::cache;
namespace ocgm_graph = open_comp_graph_maya::graph;

namespace open_comp_graph_maya {
namespace image_plane {

// Parameter Names
MString GeometryOverride::m_shader_color_parameter_name = "gSolidColor";
MString GeometryOverride::m_shader_geometry_transform_parameter_name = "gGeometryTransform";
MString GeometryOverride::m_shader_rescale_transform_parameter_name = "gRescaleTransform";
MString GeometryOverride::m_shader_image_color_matrix_parameter_name = "gImageColorMatrix";
MString GeometryOverride::m_shader_image_texture_parameter_name = "gImageTexture";
MString GeometryOverride::m_shader_image_texture_sampler_parameter_name = "gImageTextureSampler";
MString GeometryOverride::m_shader_3d_lut_enable_parameter_name = "g3dLutEnable";
MString GeometryOverride::m_shader_3d_lut_edge_size_parameter_name = "g3dLutEdgeSize";
MString GeometryOverride::m_shader_3d_lut_texture_parameter_name = "g3dLutTexture";
MString GeometryOverride::m_shader_3d_lut_texture_sampler_parameter_name = "g3dLutTextureSampler";

// Item Names
MString GeometryOverride::m_data_window_render_item_name = "ocgImagePlaneDataWindow";
MString GeometryOverride::m_display_window_render_item_name = "ocgImagePlaneDisplayWindow";
MString GeometryOverride::m_border_render_item_name = "ocgImagePlaneBorder";
MString GeometryOverride::m_wireframe_render_item_name = "ocgImagePlaneWireframe";
MString GeometryOverride::m_shaded_render_item_name = "ocgImagePlaneShadedTriangles";

// Stream Names
MString GeometryOverride::m_canvas_stream_name = "ocgImagePlaneCanvasStream";
MString GeometryOverride::m_display_window_stream_name = "ocgImagePlaneDisplayWindowStream";
MString GeometryOverride::m_data_window_stream_name = "ocgImagePlaneDataWindowStream";


GeometryOverride::GeometryOverride(const MObject &obj)
        : MHWRender::MPxGeometryOverride(obj)
        , m_locator_node(obj)
        , m_update_vertices(true)
        , m_update_topology(true)
        , m_update_shader(true)
        , m_update_shader_border(true)
        , m_exec_status(ocg::ExecuteStatus::kUninitialized)
        , m_focal_length(35.0f)
        , m_card_depth(1.0f)
        , m_card_size_x(1.0f)
        , m_card_size_y(1.0f)
        , m_card_res_x(16)
        , m_card_res_y(16)
        , m_time(0.0f)
        , m_display_window_width(0)
        , m_display_window_height(0)
        , m_data_window_min_x(0)
        , m_data_window_min_y(0)
        , m_data_window_max_x(0)
        , m_data_window_max_y(0)
        , m_lut_edge_size(0)
        , m_from_color_space_name()
        , m_color_space_name()
        , m_disk_cache_enable(false)
        , m_disk_cache_image_type(0)
        , m_disk_cache_dir()
        , m_in_stream_node(ocg::Node(ocg::NodeType::kNull, 0))
        , m_viewer_node(ocg::Node(ocg::NodeType::kNull, 0)) {
}

GeometryOverride::~GeometryOverride() {
}


// Cache values on the DG node.
//
// In the updateDG() call, all data needed to compute the indexing and
// geometry data must be pulled from Maya and cached. It is invalid to
// query attribute values from Maya nodes in any later stage and doing
// so may result in instability.
void GeometryOverride::updateDG() {
    auto log = log::get_logger();
    MStatus status;
    log->debug("GeometryOverride::updateDG: start.");

    ocg::Node new_stream_node = m_in_stream_node;
    bool in_stream_has_changed = false;
    MPlug in_stream_plug(m_locator_node, ShapeNode::m_in_stream_attr);
    std::tie(new_stream_node, in_stream_has_changed) =
        utils::get_plug_value_stream(in_stream_plug, m_in_stream_node);
    auto shared_graph = get_shared_graph();
    if (!shared_graph) {
        log->error("OCG Graph is not valid.");
        // Reset to empty node.
        m_in_stream_node = ocg::Node(ocg::NodeType::kNull, 0);
        return;
    }
    // Only update the internal class variable once we are sure the
    // input data is valid..
    m_in_stream_node = new_stream_node;

    // Create Viewer node.
    bool viewer_exists = shared_graph->node_exists(m_viewer_node);
    if (!viewer_exists) {
        MFnDagNode node(m_locator_node, &status);
        ShapeNode *fp = status ? dynamic_cast<ShapeNode *>(node.userNode()) : nullptr;

        auto viewer_node_hash = fp->m_viewer_node_hash;
        m_viewer_node = shared_graph->create_node(
            ocg::NodeType::kViewer,
            viewer_node_hash);
    }
    if (in_stream_has_changed) {
        bool stream_node_exists = shared_graph->node_exists(m_in_stream_node);
        if (stream_node_exists) {
            shared_graph->connect(m_in_stream_node, m_viewer_node, 0);
        } else {
            shared_graph->disconnect_input(m_viewer_node, 0);
        }
    }

    // Set attributes on Viewer
    bool cache_option_has_changed = false;
    bool cache_crop_on_format_has_changed = false;
    bool disk_cache_enable_has_changed = false;
    bool disk_cache_image_type_has_changed = false;
    bool disk_cache_dir_has_changed = false;
    MPlug cache_option_plug(
        m_locator_node, ShapeNode::m_cache_option_attr);
    MPlug cache_crop_on_format_plug(
        m_locator_node, ShapeNode::m_cache_crop_on_format_attr);
    MPlug disk_cache_enable_plug(
        m_locator_node, ShapeNode::m_disk_cache_enable_attr);
    MPlug disk_cache_image_type_plug(
        m_locator_node, ShapeNode::m_disk_cache_image_type_attr);
    MPlug disk_cache_dir_plug(
        m_locator_node, ShapeNode::m_disk_cache_dir_attr);

    std::tie(m_cache_option, cache_option_has_changed) =
        utils::get_plug_value_uint32(cache_option_plug, m_cache_option);

    std::tie(m_cache_crop_on_format, cache_crop_on_format_has_changed) =
        utils::get_plug_value_bool(cache_crop_on_format_plug, m_cache_crop_on_format);
    std::tie(m_disk_cache_enable, disk_cache_enable_has_changed) =
        utils::get_plug_value_bool(disk_cache_enable_plug, m_disk_cache_enable);

    std::tie(m_disk_cache_image_type, disk_cache_image_type_has_changed) =
        utils::get_plug_value_uint32(disk_cache_image_type_plug, m_disk_cache_image_type);

    std::tie(m_disk_cache_dir, disk_cache_dir_has_changed) =
        utils::get_plug_value_string(disk_cache_dir_plug, m_disk_cache_dir);

    if (cache_option_has_changed) {
        shared_graph->set_node_attr_i32(
            m_viewer_node, "bake_option", m_cache_option);
    }
    if (cache_crop_on_format_has_changed) {
        shared_graph->set_node_attr_i32(
            m_viewer_node, "crop_to_format", m_cache_crop_on_format);
    }
    if (disk_cache_enable_has_changed) {
        shared_graph->set_node_attr_i32(
            m_viewer_node, "disk_cache", m_disk_cache_enable);
    }
    if (disk_cache_image_type_has_changed) {
        shared_graph->set_node_attr_i32(
            m_viewer_node, "disk_cache_image_type", m_disk_cache_image_type);
    }
    if (disk_cache_dir_has_changed) {
        auto dir_name = rust::String(m_disk_cache_dir.asChar());
        shared_graph->set_node_attr_str(
            m_viewer_node, "disk_cache_dir", dir_name);
    }

    // TODO: Detect when the camera matrix has changed.
    //
    // TODO: Find the camera by following the node's 'message'
    // attribute. This is the way Maya image planes normally work, so
    // we should mimic the same feature.
    //
    // TODO: Query other attributes, like film back size, and iflm
    // back offsets.
    bool camera_has_changed = true;
    bool focal_length_has_changed = true;
    MPlug camera_plug(m_locator_node, ShapeNode::m_camera_attr);
    if (!camera_plug.isNull()) {
        MPlug src_plug = camera_plug.source(&status);
        if (!src_plug.isNull()) {
            MObject camera_object = src_plug.node(&status);
            CHECK_MSTATUS(status);
            MFnCamera camera_fn(camera_object, &status);
            CHECK_MSTATUS(status);

            m_focal_length = camera_fn.focalLength(&status);
            CHECK_MSTATUS(status);
        }
    }

    bool card_depth_has_changed = false;
    MPlug card_depth_plug(m_locator_node, ShapeNode::m_card_depth_attr);
    std::tie(m_card_depth, card_depth_has_changed) =
        utils::get_plug_value_distance_float(card_depth_plug, m_card_depth);

    bool card_size_x_has_changed = false;
    bool card_size_y_has_changed = false;
    MPlug card_size_x_plug(m_locator_node, ShapeNode::m_card_size_x_attr);
    MPlug card_size_y_plug(m_locator_node, ShapeNode::m_card_size_y_attr);
    std::tie(m_card_size_x, card_size_x_has_changed) =
        utils::get_plug_value_distance_float(card_size_x_plug, m_card_size_x);
    std::tie(m_card_size_y, card_size_y_has_changed) =
        utils::get_plug_value_distance_float(card_size_y_plug, m_card_size_y);

    bool card_res_x_has_changed = false;
    bool card_res_y_has_changed = false;
    MPlug card_res_x_plug(m_locator_node, ShapeNode::m_card_res_x_attr);
    MPlug card_res_y_plug(m_locator_node, ShapeNode::m_card_res_y_attr);
    std::tie(m_card_res_x, card_res_x_has_changed) =
        utils::get_plug_value_uint32(card_res_x_plug, m_card_res_x);
    std::tie(m_card_res_y, card_res_y_has_changed) =
        utils::get_plug_value_uint32(card_res_y_plug, m_card_res_y);

    bool time_has_changed = false;
    MPlug time_plug(m_locator_node, ShapeNode::m_time_attr);
    std::tie(m_time, time_has_changed) =
        utils::get_plug_value_frame_float(time_plug, m_time);

    uint32_t stream_values_changed = 0;
    uint32_t shader_values_changed = 0;
    uint32_t shader_border_values_changed = 0;
    uint32_t topology_values_changed = 0;
    uint32_t vertex_values_changed = 0;
    shader_values_changed += static_cast<uint32_t>(camera_has_changed);
    shader_values_changed += static_cast<uint32_t>(focal_length_has_changed);
    shader_values_changed += static_cast<uint32_t>(card_depth_has_changed);
    shader_values_changed += static_cast<uint32_t>(card_size_x_has_changed);
    shader_values_changed += static_cast<uint32_t>(card_size_y_has_changed);

    shader_border_values_changed += static_cast<uint32_t>(focal_length_has_changed);
    shader_border_values_changed += static_cast<uint32_t>(card_depth_has_changed);
    shader_border_values_changed += static_cast<uint32_t>(card_size_x_has_changed);
    shader_border_values_changed += static_cast<uint32_t>(card_size_y_has_changed);

    topology_values_changed += static_cast<uint32_t>(card_res_x_has_changed);
    topology_values_changed += static_cast<uint32_t>(card_res_y_has_changed);

    shader_values_changed += static_cast<uint32_t>(time_has_changed);
    shader_values_changed += static_cast<uint32_t>(in_stream_has_changed);

    stream_values_changed += static_cast<uint32_t>(time_has_changed);
    stream_values_changed += static_cast<uint32_t>(in_stream_has_changed);

    vertex_values_changed += static_cast<uint32_t>(focal_length_has_changed);
    vertex_values_changed += static_cast<uint32_t>(card_depth_has_changed);
    vertex_values_changed += static_cast<uint32_t>(card_size_x_has_changed);
    vertex_values_changed += static_cast<uint32_t>(card_size_y_has_changed);

    log->debug("shader_values_changed: {}", shader_values_changed);
    log->debug("shader_border_values_changed: {}", shader_border_values_changed);
    log->debug("topology_values_changed: {}", topology_values_changed);
    log->debug("stream_values_changed: {}", stream_values_changed);

    // Evaluate the OCG Graph.
    m_exec_status = ocg::ExecuteStatus::kUninitialized;
    if (stream_values_changed > 0) {
        log->debug("ocgImagePlane: m_time={}", m_time);
        double execute_frame = std::lround(m_time);
        log->debug("ocgImagePlane: execute_frame={}", execute_frame);
        auto shared_cache = ocgm_cache::get_shared_cache();
        m_exec_status = ocgm_graph::execute_ocg_graph(
            m_viewer_node,
            execute_frame,
            shared_graph,
            shared_cache);

        // TODO: Get and check if the deformer has changed.
        vertex_values_changed += 1;
    }
    log->debug("vertex_values_changed: {}", vertex_values_changed);
    log->debug("exec_status: {}", m_exec_status);

    // Have the attribute values changed?
    if (vertex_values_changed > 0) {
        m_update_vertices = true;
    }
    if (topology_values_changed > 0) {
        m_update_topology = true;
    }
    if (shader_values_changed > 0) {
        m_update_shader = true;
    }
    if (shader_border_values_changed > 0) {
        m_update_shader_border = true;
    }
    log->debug("update_shader_border={}", m_update_shader_border);
    log->debug("update_shader={}", m_update_shader);
    log->debug("update_topology={}", m_update_topology);
    log->debug("update_vertices={}", m_update_vertices);

    // // TODO: Query the bounding box and store it for later use.
    //
    // MFnDagNode node(m_locator_node, &status);
    // ShapeNode *fp = status ? dynamic_cast<ShapeNode *>(node.userNode())
    //                        : nullptr;
    // MBoundingBox *bounds = fp ? new MBoundingBox(fp->boundingBox())
    //                           : nullptr;

    log->debug("GeometryOverride::updateDG: end.");
}

// Create and update the list of items to render in Viewport 2.0.
//
// For each shader assigned to the instance of the object Maya will
// assign a render item. A render item is a single atomic renderable
// entity containing a shader and some geometry.
//
// In updateRenderItems(), implementations of this class may enable or
// disable the automatic shader-based render items and they may add or
// remove custom user defined render items in order to cause
// additional things to be drawn. Look at the MRenderItem interface
// for more details.
void GeometryOverride::updateRenderItems(const MDagPath &path,
                                         MHWRender::MRenderItemList &list) {
    auto log = log::get_logger();
    MStatus status;

    log->debug("GeometryOverride::updateRenderItems: start.");

    // Compile and update shader.
    CHECK_MSTATUS(m_shader_wire.compile_file("ocgImagePlaneSolid"));
    CHECK_MSTATUS(m_shader_border.compile_file("ocgImagePlaneSolid"));
    CHECK_MSTATUS(m_shader_display_window.compile_file("ocgImagePlaneSolid"));
    CHECK_MSTATUS(m_shader_data_window.compile_file("ocgImagePlaneSolid"));
    CHECK_MSTATUS(m_shader.compile_file("ocgImagePlaneTextured"));
    if (!m_shader.instance()
        || !m_shader_border.instance()
        || !m_shader_wire.instance()
        || !m_shader_display_window.instance()
        || !m_shader_data_window.instance()) {
        log->error("GeometryOverride: Failed to compile shader.");
        return;
    }

    if (m_update_shader || m_update_shader_border) {
        log->debug("GeometryOverride: Update shader parameters...");

        // Allow transparency in the shader.
        m_shader.set_is_transparent(true);

        auto filmBackWidth = 36.0f;  // 35mm film width is 36 x 24 millimetres.
        auto plane_scale = utils::getCameraPlaneScale(filmBackWidth, m_focal_length);

        const float depth_scale = m_card_depth * plane_scale;
        const float inv_card_depth = -1.0f * m_card_depth;
        const float depth_matrix_values[4][4] = {
            // Column 0
            {depth_scale, 0.0,         0.0,            0.0},
            // Column 1
            {0.0,         depth_scale, 0.0,            0.0},
            // Column 2
            {0.0,         0.0,         m_card_depth,   0.0},
            // Column 3
            {0.0,         0.0,         inv_card_depth, 1.0},
        };
        MFloatMatrix depth_matrix(depth_matrix_values);
        MFloatMatrix geom_matrix(depth_matrix);

        const float display_window_color_values[4] = {1.0f, 1.0f, 0.0f, 1.0f};
        status = m_shader_display_window.set_color_param(
            m_shader_color_parameter_name, display_window_color_values);
        CHECK_MSTATUS(status);

        const float data_window_color_values[4] = {0.0f, 1.0f, 1.0f, 1.0f};
        status = m_shader_data_window.set_color_param(
            m_shader_color_parameter_name, data_window_color_values);
        CHECK_MSTATUS(status);

        const float wire_color_values[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        status = m_shader_wire.set_color_param(
            m_shader_color_parameter_name, wire_color_values);
        CHECK_MSTATUS(status);

        const float border_color_values[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        status = m_shader_border.set_color_param(
            m_shader_color_parameter_name, border_color_values);
        CHECK_MSTATUS(status);

        // Set the transform matrix parameter expected to move the
        // geometry buffer into the correct place
        status = m_shader_display_window.set_float_matrix4x4_param(
            m_shader_geometry_transform_parameter_name, geom_matrix);
        CHECK_MSTATUS(status);

        status = m_shader_data_window.set_float_matrix4x4_param(
            m_shader_geometry_transform_parameter_name, geom_matrix);
        CHECK_MSTATUS(status);

        status = m_shader_wire.set_float_matrix4x4_param(
            m_shader_geometry_transform_parameter_name, geom_matrix);
        CHECK_MSTATUS(status);

        status = m_shader_border.set_float_matrix4x4_param(
            m_shader_geometry_transform_parameter_name, geom_matrix);
        CHECK_MSTATUS(status);

        status = m_shader.set_float_matrix4x4_param(
            m_shader_geometry_transform_parameter_name, geom_matrix);
        CHECK_MSTATUS(status);

        if (m_exec_status == ocg::ExecuteStatus::kSuccess) {
            auto shared_graph = get_shared_graph();
            auto stream_data = shared_graph->output_stream();

            auto display_window = stream_data.display_window();
            auto data_window = stream_data.data_window();

            m_display_window_width = display_window.max_x - display_window.min_x;
            m_display_window_height = display_window.max_y - display_window.min_y;
            m_data_window_min_x = data_window.min_x;
            m_data_window_min_y = data_window.min_y;
            m_data_window_max_x = data_window.max_x;
            m_data_window_max_y = data_window.max_y;

            // Move display window to the image plane.
            auto display_width = static_cast<float>(display_window.max_x -
                                                    display_window.min_x);
            auto display_height = static_cast<float>(display_window.max_y -
                                                     display_window.min_y);
            auto display_half_width = display_width / 2.0f;
            auto display_half_height = display_height / 2.0f;
            // TODO: Create logic for "film fit" modes. Currently
            // we're using "horizontal" (aka "width").
            auto display_fit_scale_x = (display_width / 2.0f);
            auto display_fit_scale_y = (display_width / 2.0f);
            auto display_scale_x = 1.0f / display_fit_scale_x;
            auto display_scale_y = 1.0f / display_fit_scale_y;
            auto display_offset_x = (static_cast<float>(display_window.min_x) -
                                     display_half_width) / display_fit_scale_x;
            auto display_offset_y = (static_cast<float>(display_window.min_y) -
                                     display_half_height) / display_fit_scale_y;
            const float rescale_display_window_values[4][4] = {
                // X
                {display_scale_x,  0.0,              0.0, 0.0},
                // Y
                {0.0,              display_scale_y,  0.0, 0.0},
                // Z
                {0.0,              0.0,              1.0, 0.0},
                // W
                {display_offset_x, display_offset_y, 0.0, 1.0},
            };
            MFloatMatrix rescale_display_window_transform(
                rescale_display_window_values);

            // Move Canvas to the data window.
            auto data_scale_x = static_cast<float>(data_window.max_x -
                                                   data_window.min_x);
            auto data_scale_y = static_cast<float>(data_window.max_y -
                                                   data_window.min_y);
            auto data_offset_x = static_cast<float>(data_window.min_x);
            auto data_offset_y = static_cast<float>(data_window.min_y);
            const float move_data_window_values[4][4] = {
                // X
                {data_scale_x,  0.0,           0.0, 0.0},
                // Y
                {0.0,           data_scale_y,  0.0, 0.0},
                // Z
                {0.0,           0.0,           1.0, 0.0},
                // W
                {data_offset_x, data_offset_y, 0.0, 1.0},
            };
            MFloatMatrix move_data_window_transform(move_data_window_values);
            move_data_window_transform *= rescale_display_window_transform;

            status = m_shader_wire.set_float_matrix4x4_param(
                m_shader_rescale_transform_parameter_name,
                move_data_window_transform);
            CHECK_MSTATUS(status);

            status = m_shader_border.set_float_matrix4x4_param(
                m_shader_rescale_transform_parameter_name,
                move_data_window_transform);
            CHECK_MSTATUS(status);

            status = m_shader.set_float_matrix4x4_param(
                m_shader_rescale_transform_parameter_name,
                move_data_window_transform);
            CHECK_MSTATUS(status);

            status = m_shader_display_window.set_float_matrix4x4_param(
                m_shader_rescale_transform_parameter_name,
                rescale_display_window_transform);
            CHECK_MSTATUS(status);

            status = m_shader_data_window.set_float_matrix4x4_param(
                m_shader_rescale_transform_parameter_name,
                rescale_display_window_transform);
            CHECK_MSTATUS(status);

            // The image color space.
            auto from_color_space = stream_data.clone_image_spec().color_space;
            auto from_color_space_str = std::string(from_color_space);
            bool from_color_space_changed = m_from_color_space_name.compare(from_color_space_str) != 0;

            // Size of the 3D LUT
            bool lut_edge_size_has_changed = false;
            MPlug lut_edge_size_plug(m_locator_node, ShapeNode::m_lut_edge_size_attr);
            std::tie(m_lut_edge_size, lut_edge_size_has_changed) =
                utils::get_plug_value_uint32(lut_edge_size_plug, m_lut_edge_size);

            // Color Space Name
            bool color_space_name_has_changed = false;
            MPlug color_space_name_plug(
                m_locator_node, ShapeNode::m_color_space_name_attr);
            std::tie(m_color_space_name, color_space_name_has_changed) =
                utils::get_plug_value_string(color_space_name_plug, m_color_space_name);

            // Generate a 3D volume texture to be used to look
            // up colour space transforms.
            if (lut_edge_size_has_changed
                || color_space_name_has_changed
                || from_color_space_changed) {
                m_from_color_space_name = from_color_space_str;

                // Color Space Conersation values
                auto to_color_space = m_color_space_name.asChar();

                auto use_3dlut = from_color_space != to_color_space;
                log->warn("GeometryOverride:: use 3D LUT: {}", use_3dlut);
                log->warn("GeometryOverride:: 3D LUT Edge Size: {}", m_lut_edge_size);
                log->warn("GeometryOverride:: Color Space: {} to {}", from_color_space_str, to_color_space);

                if (use_3dlut && (m_lut_edge_size > 0)) {
                    auto shared_color_transform_cache = \
                        ocgm_cache::get_shared_color_transform_cache();
                    auto lut_image = ocg::get_color_transform_3dlut(
                        from_color_space, to_color_space,
                        m_lut_edge_size, shared_color_transform_cache);

                    // 3D LUT Texture Sampler.
                    MHWRender::MSamplerStateDesc sampler_description;
                    sampler_description.filter = MSamplerState::TextureFilter::kMinMagMipLinear;
                    sampler_description.addressU = MSamplerState::TextureAddress::kTexClamp;
                    sampler_description.addressV = MSamplerState::TextureAddress::kTexClamp;
                    sampler_description.addressW = MSamplerState::TextureAddress::kTexClamp;
                    sampler_description.minLOD = 0;
                    sampler_description.maxLOD = 0;
                    status = m_shader.set_texture_sampler_param(
                        m_shader_3d_lut_texture_sampler_parameter_name,
                        sampler_description
                    );
                    CHECK_MSTATUS(status);

                    // 3D LUT Texture.
                    //
                    // Texture is a cube and assumed to be lut_edge_size^3
                    // (eg, 20 * 20 * 20).
                    auto pixel_width = m_lut_edge_size;
                    auto pixel_height = m_lut_edge_size;
                    auto pixel_depth = m_lut_edge_size;
                    auto pixel_num_channels = lut_image.pixel_block->num_channels();
                    auto pixel_data_type = lut_image.pixel_block->pixel_data_type();
                    auto pixel_buffer = lut_image.pixel_block->as_slice();
                    auto buffer = static_cast<const void*>(pixel_buffer.data());

                    log->warn("GeometryOverride:: lut_edge_size: {}", m_lut_edge_size);
                    log->warn("GeometryOverride:: pixel_num_channels: {}", pixel_num_channels);
                    log->warn("GeometryOverride:: pixel count: {}", pixel_buffer.length());
                    // for (auto i = 0; i < pixel_buffer.length(); ++i) {
                    //     log->warn("num: {}={}", i, pixel_buffer[i]);
                    //     if (i > 100) break;
                    // }

                    // Upload 3D LUT to GPU.
                    status = m_shader.set_texture_param_with_image_data(
                        m_shader_3d_lut_texture_parameter_name,
                        MHWRender::kVolumeTexture,
                        pixel_width,
                        pixel_height,
                        pixel_depth,
                        pixel_num_channels,
                        pixel_data_type,
                        buffer);
                    CHECK_MSTATUS(status);


                    // 3D LUT Edge Size.
                    status = m_shader.set_int_param(
                        m_shader_3d_lut_edge_size_parameter_name,
                        m_lut_edge_size);
                    CHECK_MSTATUS(status);
                }

                // Should we use the 3D LUT texture?
                status = m_shader.set_bool_param(
                    m_shader_3d_lut_enable_parameter_name, use_3dlut);
                CHECK_MSTATUS(status);
            }


            // Set the matrix parameter expected to adjust the colors
            // of the image texture.
            auto color_matrix = stream_data.color_matrix();
            const float color_matrix_values[4][4] = {
                {color_matrix.m00, color_matrix.m01, color_matrix.m02, color_matrix.m03},
                {color_matrix.m10, color_matrix.m11, color_matrix.m12, color_matrix.m13},
                {color_matrix.m20, color_matrix.m21, color_matrix.m22, color_matrix.m23},
                {color_matrix.m30, color_matrix.m31, color_matrix.m32, color_matrix.m33},
            };
            MFloatMatrix image_color_matrix(color_matrix_values);
            status = m_shader.set_float_matrix4x4_param(
                m_shader_image_color_matrix_parameter_name,
                image_color_matrix);
            CHECK_MSTATUS(status);

            MHWRender::MSamplerStateDesc sampler_description;
            sampler_description.filter = MSamplerState::TextureFilter::kMinMagMipPoint;
            sampler_description.addressU = MSamplerState::TextureAddress::kTexClamp;
            sampler_description.addressV = MSamplerState::TextureAddress::kTexClamp;
            status = m_shader.set_texture_sampler_param(
                m_shader_image_texture_sampler_parameter_name,
                sampler_description
            );
            CHECK_MSTATUS(status);

            status = m_shader.set_texture_param_with_stream_data(
                m_shader_image_texture_parameter_name,
                std::move(stream_data));
            CHECK_MSTATUS(status);
        }
        m_update_shader = false;
        m_update_shader_border = false;
    }

    bool items_changed = false;
    int index = 0;

    MHWRender::MRenderItem *display_window_item = nullptr;
    index = list.indexOf(m_display_window_render_item_name);
    if (index < 0) {
        display_window_item = MHWRender::MRenderItem::Create(
            m_display_window_render_item_name,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines);
        display_window_item->setDrawMode(MHWRender::MGeometry::kAll);
        display_window_item->depthPriority(MRenderItem::sActiveLineDepthPriority);

        list.append(display_window_item);
        items_changed = true;
    } else {
        display_window_item = list.itemAt(index);
    }

    MHWRender::MRenderItem *data_window_item = nullptr;
    index = list.indexOf(m_data_window_render_item_name);
    if (index < 0) {
        data_window_item = MHWRender::MRenderItem::Create(
            m_data_window_render_item_name,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines
        );
        data_window_item->setDrawMode(MHWRender::MGeometry::kAll);
        data_window_item->depthPriority(MRenderItem::sActiveWireDepthPriority);

        list.append(data_window_item);
        items_changed = true;
    } else {
        data_window_item = list.itemAt(index);
    }

    MHWRender::MRenderItem *wireframe_item = nullptr;
    index = list.indexOf(m_wireframe_render_item_name);
    if (index < 0) {
        wireframe_item = MHWRender::MRenderItem::Create(
            m_wireframe_render_item_name,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines
        );
        wireframe_item->setDrawMode(MHWRender::MGeometry::kWireframe);
        wireframe_item->depthPriority(MRenderItem::sHiliteWireDepthPriority);

        list.append(wireframe_item);
        items_changed = true;
    } else {
        wireframe_item = list.itemAt(index);
    }

    MHWRender::MRenderItem *border_item = nullptr;
    index = list.indexOf(m_border_render_item_name);
    if (index < 0) {
        border_item = MHWRender::MRenderItem::Create(
            m_border_render_item_name,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines
        );
        MGeometry::DrawMode draw_mode =
            static_cast<MGeometry::DrawMode>(MHWRender::MGeometry::kShaded);
        border_item->setDrawMode(draw_mode);
        border_item->depthPriority(MRenderItem::sHiliteWireDepthPriority);

        list.append(border_item);
        items_changed = true;
    } else {
        border_item = list.itemAt(index);
    }

    MHWRender::MRenderItem *shaded_item = nullptr;
    index = list.indexOf(m_shaded_render_item_name);
    if (index < 0) {
        shaded_item = MHWRender::MRenderItem::Create(
            m_shaded_render_item_name,
            MRenderItem::MaterialSceneItem,
            MHWRender::MGeometry::kTriangles
        );
        MGeometry::DrawMode draw_mode =
            static_cast<MGeometry::DrawMode>(MHWRender::MGeometry::kTextured);
        shaded_item->setDrawMode(draw_mode);
        shaded_item->setExcludedFromPostEffects(true);
        shaded_item->castsShadows(true);
        shaded_item->receivesShadows(true);
        shaded_item->depthPriority(MRenderItem::sDormantFilledDepthPriority);

        list.append(shaded_item);
        items_changed = true;
    } else {
        shaded_item = list.itemAt(index);
    }

    if (items_changed) {
        // Canvas
        wireframe_item->setShader(m_shader_wire.instance(), &m_canvas_stream_name);
        border_item->setShader(m_shader_border.instance(), &m_canvas_stream_name);
        shaded_item->setShader(m_shader.instance(), &m_canvas_stream_name);

        // Display and Data windows.
        display_window_item->setShader(
            m_shader_display_window.instance(), &m_display_window_stream_name);
        data_window_item->setShader(
            m_shader_data_window.instance(), &m_data_window_stream_name);
    }

    log->debug("GeometryOverride::updateRenderItems: end.");
}

// Add text and simple UI elements.
//
// For each instance of the object, besides the render items updated
// in 'updateRenderItems()' for the geometry rendering, there is also
// a render item list there for render the simple UI elements.
//
// 'addUIDrawables()' happens just after normal geometry item
// updating, The original design for this stage is to allow user
// accessing 'MUIDrawManager' which helps drawing the simple geometry
// easily like line, circle, rectangle, text, etc.
void GeometryOverride::addUIDrawables(
        const MDagPath &path,
        MHWRender::MUIDrawManager &draw_manager,
        const MHWRender::MFrameContext &/*frame_context*/) {
    // TODO Calculate the correct positions for the image window.
    MPoint center_pos(0.0, 0.0, 0.0);
    MPoint upper_right(1.0, 1.0, 0.0);
    MPoint lower_left(-1.0, -1.0, 0.0);
    MPoint lower_right(1.0, -1.0, 0.0);
    MColor text_color(0.1f, 0.8f, 0.8f, 1.0f);

    MString text("Open Comp Graph Maya");

    // Draw Display Window
    //
    // TODO: draw the pixel aspect ratio.
    MString display_window_width = "";
    MString display_window_height = "";
    display_window_width.set(m_display_window_width, 0);
    display_window_height.set(m_display_window_height, 0);
    MString display_window =
        display_window_width + " x " + display_window_height;

    // Draw the data-window coordinate values for lower-left and
    // upper-right corners.
    MString data_window_min_x = "";
    MString data_window_min_y = "";
    MString data_window_max_x = "";
    MString data_window_max_y = "";
    data_window_min_x.set(m_data_window_min_x, 0);
    data_window_min_y.set(m_data_window_min_y, 0);
    data_window_max_x.set(m_data_window_max_x, 0);
    data_window_max_y.set(m_data_window_max_y, 0);
    MString data_window_min =
        data_window_min_x + " x " + data_window_min_y;
    MString data_window_max =
        data_window_max_x + " x " + data_window_max_y;

    draw_manager.beginDrawable();
    draw_manager.setColor(text_color);
    draw_manager.setFontSize(MHWRender::MUIDrawManager::kDefaultFontSize);
    draw_manager.text(center_pos, text, MHWRender::MUIDrawManager::kCenter);
    draw_manager.text(lower_left, data_window_min, MHWRender::MUIDrawManager::kRight);
    draw_manager.text(upper_right, data_window_max, MHWRender::MUIDrawManager::kLeft);
    draw_manager.text(lower_right, display_window, MHWRender::MUIDrawManager::kLeft);
    draw_manager.endDrawable();
}

// Create Geometry Buffers.
//
// In this method the implementation is expected to fill the MGeometry
// data structure with the vertex and index buffers required to draw
// the object as indicated by the data in the geometry requirements
// instance passed to this method. Failure to fulfill the geometry
// requirements may result in incorrect drawing or possibly complete
// failure to draw the object.
void GeometryOverride::populateGeometry(
        const MHWRender::MGeometryRequirements &requirements,
        const MHWRender::MRenderItemList &renderItems,
        MHWRender::MGeometry &data) {
    auto log = log::get_logger();
    MStatus status;

    log->debug("GeometryOverride::populateGeometry: start.");
    log->debug("GeometryOverride::populateGeometry: m_update_vertices: {}",
               m_update_vertices);
    log->debug("GeometryOverride::populateGeometry: m_update_topology: {}",
               m_update_topology);

    // Generate vertex buffer data (Position and UVs).
    auto shared_graph = get_shared_graph();
    auto stream_data = shared_graph->output_stream();
    auto num_deformers = stream_data.deformers_len();
    log->debug("Updating vertex position... num_deformers={}",
               num_deformers);

    auto display_window = stream_data.display_window();
    auto data_window = stream_data.data_window();
    m_geometry_window_display.set_bounding_box(display_window);
    m_geometry_window_data.set_bounding_box(data_window);

    m_geometry_canvas.set_divisions_x(m_card_res_x);
    m_geometry_canvas.set_divisions_y(m_card_res_y);

    MHWRender::MVertexBuffer *canvas_positions_buffer = nullptr;
    MHWRender::MVertexBuffer *canvas_uvs_buffer = nullptr;
    MHWRender::MVertexBuffer *window_display_positions_buffer = nullptr;
    MHWRender::MVertexBuffer *window_data_positions_buffer = nullptr;

    const MHWRender::MVertexBufferDescriptorList &
        desc_list = requirements.vertexRequirements();
    MHWRender::MVertexBufferDescriptor desc;
    const int number_of_vertex_requirements = desc_list.length();
    for (int i = 0; i < number_of_vertex_requirements; ++i) {
        if (!desc_list.getDescriptor(i, desc)) {
            continue;
        }

        if (desc.name() == m_canvas_stream_name) {
            // Canvas - Positions and UVs.
            if (desc.semantic() == MHWRender::MGeometry::kPosition) {
                if (!canvas_positions_buffer) {
                    canvas_positions_buffer = data.createVertexBuffer(desc);
                    if (canvas_positions_buffer) {
                        m_geometry_canvas.fill_vertex_buffer_positions(
                            canvas_positions_buffer,
                            stream_data);
                    }
                }
            } else if (desc.semantic() == MHWRender::MGeometry::kTexture) {
                if (!canvas_uvs_buffer) {
                    canvas_uvs_buffer = data.createVertexBuffer(desc);
                    if (canvas_uvs_buffer) {
                        m_geometry_canvas.fill_vertex_buffer_uvs(canvas_uvs_buffer);
                    }
                }
            }

        } else if (desc.name() == m_display_window_stream_name) {
            // Display Window - Positions.
            if (desc.semantic() == MHWRender::MGeometry::kPosition) {
                if (!window_display_positions_buffer) {
                    window_display_positions_buffer = data.createVertexBuffer(desc);
                    if (window_display_positions_buffer) {
                        m_geometry_window_display.fill_vertex_buffer_positions(
                            window_display_positions_buffer);
                    }
                }
            }

        } else if (desc.name() == m_data_window_stream_name) {
            // Data Window - Positions.
            if (desc.semantic() == MHWRender::MGeometry::kPosition) {
                if (!window_data_positions_buffer) {
                    window_data_positions_buffer = data.createVertexBuffer(desc);
                    if (window_data_positions_buffer) {
                        m_geometry_window_data.fill_vertex_buffer_positions(
                            window_data_positions_buffer);
                    }
                }
            }
        }
    }

    // Index Buffers
    for (int i = 0; i < renderItems.length(); ++i) {
        const MHWRender::MRenderItem *item = renderItems.itemAt(i);
        if (!item) {
            continue;
        }

        MHWRender::MIndexBuffer *index_buffer = data.createIndexBuffer(
            MHWRender::MGeometry::kUnsignedInt32);

        if (item->name() == m_data_window_render_item_name) {
            m_geometry_window_data.fill_index_buffer_border_lines(index_buffer);

        } else if (item->name() == m_display_window_render_item_name) {
            m_geometry_window_display.fill_index_buffer_border_lines(index_buffer);

        } else if (item->name() == m_shaded_render_item_name) {
            m_geometry_canvas.fill_index_buffer_triangles(index_buffer);

        } else if (item->name() == m_border_render_item_name) {
            m_geometry_canvas.fill_index_buffer_border_lines(index_buffer);

        } else if (item->name() == m_wireframe_render_item_name) {
            m_geometry_canvas.fill_index_buffer_wire_lines(index_buffer);
        }

        if (index_buffer) {
            item->associateWithIndexBuffer(index_buffer);
        }
    }

    log->debug("GeometryOverride::populateGeometry: end.");
}

} // namespace image_plane
} // namespace open_comp_graph_maya
