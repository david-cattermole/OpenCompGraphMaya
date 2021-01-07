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
 * Image Plane Viewport 2.0 MPxSubSceneOverride implementation.
 */

// Maya
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MFloatMatrix.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MDagMessage.h>
#include <maya/MSelectionContext.h>
#include <maya/M3dView.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MStateManager.h>

// STL
#include <memory>
#include <tuple>
#include <cstdlib>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "constant_texture_data.h"
#include "image_plane_sub_scene_override.h"
#include "image_plane_shape.h"
#include "graph_maya_data.h"
#include "logger.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

namespace {

// Get distance attribute value.
std::tuple<float, bool> get_plug_value_distance_float(MPlug plug, float old_value) {
    bool has_changed = false;
    float value = old_value;
    if (!plug.isNull()) {
        float new_value = 0.0;
        MDistance temp_value;
        if (plug.getValue(temp_value)) {
            new_value = static_cast<float>(temp_value.asCentimeters());
        }
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get unsigned integer attribute value.
std::tuple<uint32_t, bool> get_plug_value_uint32(MPlug plug, uint32_t old_value) {
    MStatus status;
    bool has_changed = false;
    uint32_t value = old_value;
    if (!plug.isNull()) {
        uint32_t new_value = plug.asInt(&status);
        CHECK_MSTATUS(status);
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get floating point attribute value.
std::tuple<float, bool> get_plug_value_float(MPlug plug, float old_value) {
    MStatus status;
    bool has_changed = false;
    float value = old_value;
    if (!plug.isNull()) {
        uint32_t new_value = plug.asFloat(&status);
        CHECK_MSTATUS(status);
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get the ocgStreamData type from the given plug.
std::tuple<std::shared_ptr<ocg::Graph>, ocg::Node, bool>
get_plug_value_stream(MPlug plug, ocg::Node old_value) {
    MStatus status;
    auto log = log::get_logger();

    std::shared_ptr<ocg::Graph> shared_graph;
    bool has_changed = false;
    ocg::Node value = old_value;
    if (!plug.isNull()) {
        MObject new_object = plug.asMObject(&status);
        CHECK_MSTATUS(status);
        if (new_object.isNull()) {
            log->error("Input stream is not valid.");
            return std::make_tuple(shared_graph, value, has_changed);
        }

        // Convert Maya controlled data into the OCG custom MPxData class.
        // We are ensured this is valid from Maya. The MObject is a smart
        // pointer and we check the object is valid before-hand too.
        MFnPluginData fn_plugin_data(new_object);
        GraphMayaData* input_stream_data =
            static_cast<GraphMayaData*>(fn_plugin_data.data(&status));
        CHECK_MSTATUS(status);
        if (input_stream_data == nullptr) {
            log->error("Input stream data is not valid.");
            return std::make_tuple(shared_graph, value, has_changed);
        }
        shared_graph = input_stream_data->get_graph();
        ocg::Node new_value = input_stream_data->get_node();
        log->debug("input node id: {}", new_value.get_id());

        has_changed =
            (shared_graph->state() != ocg::GraphState::kClean)
            || (old_value.get_id() != new_value.get_id());
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(shared_graph, value, has_changed);
}

} // namespace anonymous

MString SubSceneOverride::m_shader_color_parameter_name = "gSolidColor";
MString SubSceneOverride::m_shader_geometry_transform_parameter_name = "gGeometryTransform";
MString SubSceneOverride::m_shader_image_transform_parameter_name = "gImageTransform";
MString SubSceneOverride::m_shader_image_color_matrix_parameter_name = "gImageColorMatrix";
MString SubSceneOverride::m_shader_image_texture_parameter_name = "gImageTexture";
MString SubSceneOverride::m_shader_image_texture_sampler_parameter_name = "gImageTextureSampler";
MString SubSceneOverride::m_wireframe_render_item_name = "ocgImagePlaneWireframe";
MString SubSceneOverride::m_shaded_render_item_name = "ocgImagePlaneShadedTriangles";

SubSceneOverride::SubSceneOverride(const MObject &obj)
        : MHWRender::MPxSubSceneOverride(obj),
          m_locator_node(obj),
          m_card_size_x(0.0f),
          m_card_res_x(16),
          m_time(0.0f),
          m_is_instance_mode(false),
          m_are_ui_drawables_dirty(true),
          m_instance_added_cb_id(0),
          m_instance_removed_cb_id(0),
          m_ocg_cache(std::make_shared<ocg::Cache>()),
          m_in_stream_node(ocg::Node(ocg::NodeType::kNull, 0)) {
    MDagPath dag_path;
    if (MDagPath::getAPathTo(obj, dag_path)) {
        m_instance_added_cb_id = MDagMessage::addInstanceAddedDagPathCallback(
                dag_path, InstanceChangedCallback, this);

        m_instance_removed_cb_id = MDagMessage::addInstanceRemovedDagPathCallback(
                dag_path, InstanceChangedCallback, this);
    }
}

SubSceneOverride::~SubSceneOverride() {
    SubSceneOverride::m_geometry.clear_all();

    // Remove callbacks related to instances.
    if (m_instance_added_cb_id != 0) {
        MMessage::removeCallback(m_instance_added_cb_id);
        m_instance_added_cb_id = 0;
    }

    if (m_instance_removed_cb_id != 0) {
        MMessage::removeCallback(m_instance_removed_cb_id);
        m_instance_removed_cb_id = 0;
    }
}

/* static */
void SubSceneOverride::InstanceChangedCallback(
    MDagPath &/*child*/,
    MDagPath &/*parent*/,
    void *clientData) {
    SubSceneOverride *ovr = static_cast<SubSceneOverride *>(clientData);
    if (ovr) {
        ovr->m_instance_dag_paths.clear();
    }
}

void SubSceneOverride::update(
    MHWRender::MSubSceneContainer &container,
    const MHWRender::MFrameContext &/*frame_context*/) {
    auto log = log::get_logger();
    MStatus status;

    std::uint32_t num_instances = m_instance_dag_paths.length();
    if (num_instances == 0) {
        if (!MDagPath::getAllPathsTo(m_locator_node, m_instance_dag_paths)) {
            log->error("SubSceneOverride: Failed to get all DAG paths.");
            return;
        }
        num_instances = m_instance_dag_paths.length();
    }

    if (num_instances == 0) {
        return;
    }

    ocg::Node new_stream_node = m_in_stream_node;
    bool in_stream_has_changed = false;
    std::shared_ptr<ocg::Graph> shared_graph;
    MPlug in_stream_plug(m_locator_node, ShapeNode::m_in_stream_attr);
    std::tie(shared_graph, new_stream_node, in_stream_has_changed) =
        get_plug_value_stream(in_stream_plug, m_in_stream_node);
    if (!shared_graph) {
        log->warn("OCG Graph is not valid.");
        return;
    }
    // Only update the internal class variable once we are sure the
    // input data is valid..
    m_in_stream_node = new_stream_node;

    bool card_size_x_has_changed = false;
    bool card_size_y_has_changed = false;
    MPlug card_size_x_plug(m_locator_node, ShapeNode::m_card_size_x_attr);
    MPlug card_size_y_plug(m_locator_node, ShapeNode::m_card_size_y_attr);
    std::tie(m_card_size_x, card_size_x_has_changed) =
        get_plug_value_distance_float(card_size_x_plug, m_card_size_x);
    std::tie(m_card_size_y, card_size_y_has_changed) =
        get_plug_value_distance_float(card_size_y_plug, m_card_size_y);

    bool card_res_x_has_changed = false;
    bool card_res_y_has_changed = false;
    MPlug card_res_x_plug(m_locator_node, ShapeNode::m_card_res_x_attr);
    MPlug card_res_y_plug(m_locator_node, ShapeNode::m_card_res_x_attr);
    std::tie(m_card_res_x, card_res_x_has_changed) =
        get_plug_value_uint32(card_res_x_plug, m_card_res_x);
    std::tie(m_card_res_y, card_res_y_has_changed) =
        get_plug_value_uint32(card_res_y_plug, m_card_res_y);

    bool time_has_changed = false;
    MPlug time_plug(m_locator_node, ShapeNode::m_time_attr);
    std::tie(m_time, time_has_changed) =
        get_plug_value_float(time_plug, m_time);

    uint32_t stream_values_changed = 0;
    uint32_t shader_values_changed = 0;
    uint32_t topology_values_changed = 0;
    uint32_t vertex_values_changed = 0;
    shader_values_changed += static_cast<uint32_t>(card_size_x_has_changed);
    shader_values_changed += static_cast<uint32_t>(card_size_y_has_changed);
    topology_values_changed += static_cast<uint32_t>(card_res_x_has_changed);
    topology_values_changed += static_cast<uint32_t>(card_res_y_has_changed);
    shader_values_changed += static_cast<uint32_t>(time_has_changed);
    shader_values_changed += static_cast<uint32_t>(in_stream_has_changed);
    stream_values_changed += static_cast<uint32_t>(in_stream_has_changed);
    log->debug("shader_values_changed: {}", shader_values_changed);
    log->debug("topology_values_changed: {}", topology_values_changed);
    log->debug("stream_values_changed: {}", stream_values_changed);

    // Evaluate the OCG Graph.
    auto exec_status = ocg::ExecuteStatus::kUninitialized;
    if (stream_values_changed > 0) {
        exec_status = evalutate_ocg_graph(
            m_in_stream_node,
            shared_graph,
            m_ocg_cache);
        // Get the graph output stream to deform the vertices.
        auto stream_data = shared_graph->output_stream();

        auto num_deformers = stream_data.deformers_len();
        log->info("num_deformers: {}", num_deformers);

        // TODO: Get and check if the deformer has changed.
        vertex_values_changed += 1;
    }
    log->debug("vertex_values_changed: {}", vertex_values_changed);
    log->debug("exec_status: {}", exec_status);

    // Have the attribute values changed?
    bool update_vertices = container.count() == 0;
    bool update_topology = container.count() == 0;
    bool update_shader = container.count() == 0;
    if (vertex_values_changed > 0) {
        update_vertices = true;
    }
    if (topology_values_changed > 0) {
        update_topology = true;
    }
    if (shader_values_changed > 0) {
        update_shader = true;
    }
    log->debug("update_shader={}", update_shader);
    log->debug("update_topology={}", update_topology);
    log->debug("update_vertices={}", update_vertices);

    if (update_vertices) {
        // if (exec_status == ocg::ExecuteStatus::kSuccess) {
        auto stream_data = shared_graph->output_stream();
        // auto display_window = stream_data.display_window();
        // auto data_window = stream_data.data_window();
        // auto transform_matrix = stream_data.transform_matrix();
        // auto color_matrix = stream_data.color_matrix();
        auto num_deformers = stream_data.deformers_len();
        // if (num_deformers > 0) {
        //     // SubSceneOverride::m_geometry.set_deformer_func(stream_data);
        // }
        log->debug("Updating vertex position... num_deformers={}", num_deformers);
        SubSceneOverride::m_geometry.rebuild_vertex_positions(std::move(stream_data));
        // } else {
        //     auto stream_data = ocg::StreamData();
        //     SubSceneOverride::m_geometry.rebuild_vertex_positions(std::move(stream_data));
        // }
    }

    // Update Geometry.
    if (update_topology) {
        // TODO: Split out the difference between topology and vertex
        // data changing. We can update the vertex buffer without
        // needing to change the index buffers.
        auto stream_data = shared_graph->output_stream();
        SubSceneOverride::m_geometry.set_divisions_x(m_card_res_x);
        SubSceneOverride::m_geometry.set_divisions_y(m_card_res_y);
        SubSceneOverride::m_geometry.rebuild_all(std::move(stream_data));
        // The vertices have been updated already, so there's no need
        // to do it again.
        update_vertices = false;
    }

    // Compile and update shader.
    status = SubSceneOverride::m_shader.compile("ocgImagePlane");
    if (!m_shader.instance()) {
        log->error("SubSceneOverride: Failed to get a shader.");
        return;
    }
    if (update_shader) {
        log->debug("SubSceneOverride: Update shader parameters...");
        // MColor my_color = MHWRender::MGeometryUtilities::wireframeColor(m_instance_dag_paths[0]);
        const float color_values[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        status = m_shader.set_color_param(m_shader_color_parameter_name, color_values);
        CHECK_MSTATUS(status);

        // TODO: Replace with proper values.
        const float identity_matrix_values[4][4] = {
            {1.0, 0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
            {0.0, 0.0, 0.0, 1.0},
        };

        // Set the transform matrix parameter expected to move the
        // geometry buffer into the correct place
        MFloatMatrix geom_matrix(identity_matrix_values);
        status = m_shader.set_float_matrix4x4_param(
            m_shader_geometry_transform_parameter_name, geom_matrix);
        CHECK_MSTATUS(status);

        if (exec_status == ocg::ExecuteStatus::kSuccess) {
            auto stream_data = shared_graph->output_stream();
            // auto display_window = stream_data.display_window();
            // auto data_window = stream_data.data_window();
            // auto transform_matrix = stream_data.transform_matrix();

            // Set the transform matrix parameter expected to move the
            // geometry buffer into the correct place.
            MFloatMatrix image_transform(identity_matrix_values);
            status = m_shader.set_float_matrix4x4_param(
                m_shader_image_transform_parameter_name,
                image_transform);
            CHECK_MSTATUS(status);

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
    }

    bool any_instance_changed = false;
    std::uint32_t num_visible_instances = 0;
    const std::uint32_t components_per_color = 4; // RGBA color

    MMatrixArray instance_matrix_array(num_instances);
    MFloatArray instance_color_array(
        static_cast<std::uint32_t>(num_instances * components_per_color));

    // If expecting large numbers of instances, walking through all
    // the instances every frame to look for changes is not efficient
    // enough. Monitoring change events and changing only the required
    // instances should be done instead.
    for (uint32_t i = 0; i < num_instances; i++) {
        const MDagPath &instance = m_instance_dag_paths[i];
        if (instance.isValid() && instance.isVisible()) {
            InstanceInfo instance_info(
                instance.inclusiveMatrix(),
                MHWRender::MGeometryUtilities::wireframeColor(instance));

            InstanceInfoMap::iterator iter = m_instance_info_cache.find(i);
            if (iter == m_instance_info_cache.end() ||
                iter->second.m_color != instance_info.m_color ||
                !iter->second.m_matrix.isEquivalent(instance_info.m_matrix)) {
                if (!m_are_ui_drawables_dirty &&
                    (iter == m_instance_info_cache.end() ||
                     !iter->second.m_matrix.isEquivalent(
                         instance_info.m_matrix))) {
                    m_are_ui_drawables_dirty = true;
                }
                any_instance_changed = true;
                m_instance_info_cache[i] = instance_info;
            }

            instance_matrix_array[num_visible_instances] = instance_info.m_matrix;

            uint32_t idx = num_visible_instances * components_per_color;
            instance_color_array[idx + 0] = instance_info.m_color.r;
            instance_color_array[idx + 1] = instance_info.m_color.g;
            instance_color_array[idx + 2] = instance_info.m_color.b;
            instance_color_array[idx + 3] = instance_info.m_color.a;

            num_visible_instances++;
        } else {
            InstanceInfoMap::iterator iter = m_instance_info_cache.find(i);
            if (iter != m_instance_info_cache.end()) {
                m_instance_info_cache.erase(i);

                any_instance_changed = true;
                m_are_ui_drawables_dirty = true;
            }
        }
    }

    // Shrink to fit
    instance_matrix_array.setLength(num_visible_instances);
    instance_color_array.setLength(num_visible_instances * components_per_color);

    std::uint32_t num_instance_info = static_cast<uint32_t>(m_instance_info_cache.size());
    if (num_instance_info != num_visible_instances) {
        for (unsigned int i = num_visible_instances; i < num_instance_info; i++) {
            m_instance_info_cache.erase(i);
        }
        any_instance_changed = true;
        m_are_ui_drawables_dirty = true;
    }

    bool items_changed = false;

    MHWRender::MRenderItem *wire_item = container.find(m_wireframe_render_item_name);
    if (!wire_item) {
        wire_item = MHWRender::MRenderItem::Create(
                m_wireframe_render_item_name,
                MHWRender::MRenderItem::DecorationItem,
                MHWRender::MGeometry::kLineStrip
            );
        wire_item->setDrawMode(MHWRender::MGeometry::kWireframe);
        wire_item->depthPriority(MRenderItem::sDormantWireDepthPriority);
        container.add(wire_item);
        items_changed = true;
    }

    MHWRender::MRenderItem* shaded_item = container.find(m_shaded_render_item_name);
    if (!shaded_item)
    {
        shaded_item = MHWRender::MRenderItem::Create(
                m_shaded_render_item_name,
                MRenderItem::MaterialSceneItem,
                MHWRender::MGeometry::kTriangles
            );
        MGeometry::DrawMode draw_mode =
            static_cast<MGeometry::DrawMode>(
                MHWRender::MGeometry::kShaded
                | MHWRender::MGeometry::kTextured);
        // MGeometry::DrawMode draw_mode = MHWRender::MGeometry::kAll;
        shaded_item->setDrawMode(draw_mode);
        shaded_item->setExcludedFromPostEffects(true);
        shaded_item->castsShadows(true);
        shaded_item->receivesShadows(true);
        shaded_item->depthPriority(MRenderItem::sDormantFilledDepthPriority);
        container.add(shaded_item);
        items_changed = true;
    }

    if (items_changed || any_instance_changed) {
        // TODO: Create custom solid-color shader for wireframes.
        wire_item->setShader(m_shader.instance());
        shaded_item->setShader(m_shader.instance());
    }

    if (items_changed || update_topology || update_vertices) {
        MFnDagNode node(m_locator_node, &status);

        ShapeNode *fp = status ? dynamic_cast<ShapeNode *>(node.userNode()) : nullptr;
        MBoundingBox *bounds = fp ? new MBoundingBox(fp->boundingBox()) : nullptr;

        auto position_buffer = this->m_geometry.vertex_positions();
        auto uv_buffer = this->m_geometry.vertex_uvs();
        auto shaded_index_buffer = this->m_geometry.index_triangles();

        MHWRender::MVertexBufferArray vertex_buffers;
        vertex_buffers.addBuffer("positions", position_buffer);
        vertex_buffers.addBuffer("uvs", uv_buffer);
        setGeometryForRenderItem(
                *wire_item, vertex_buffers, *shaded_index_buffer, bounds);
        setGeometryForRenderItem(
                *shaded_item, vertex_buffers, *shaded_index_buffer, bounds);

        delete bounds;
    }

    if (items_changed || any_instance_changed) {
        if (!m_is_instance_mode && num_instances == 1) {
            // For multiple copies (not multiple instances), subscene
            // consolidation is enabled for static scenario, mainly to
            // improve tumbling performance.
            wire_item->setWantSubSceneConsolidation(true);
            shaded_item->setWantSubSceneConsolidation(true);

            // When not dealing with multiple instances, don't convert
            // the render items into instanced mode.  Set the matrices
            // on them directly.
            MMatrix &obj_to_world = instance_matrix_array[0];
            wire_item->setMatrix(&obj_to_world);
            shaded_item->setMatrix(&obj_to_world);
        } else {
            // For multiple instances, subscene conslidation should be
            // turned off so that the GPU instancing can kick in.
            wire_item->setWantSubSceneConsolidation(false);
            shaded_item->setWantSubSceneConsolidation(false);

            // If we have DAG instances of this shape then use the
            // MPxSubSceneOverride instance transform API to set up
            // instance copies of the render items.  This will be
            // faster to render than creating render items for each
            // instance, especially for large numbers of instances.
            // Note this has to happen after the geometry and shaders
            // are set, otherwise it will fail.
            setInstanceTransformArray(*wire_item, instance_matrix_array);
            setInstanceTransformArray(*shaded_item, instance_matrix_array);
            setExtraInstanceData(
                *wire_item, m_shader_color_parameter_name, instance_color_array);
            setExtraInstanceData(
                *shaded_item, m_shader_color_parameter_name, instance_color_array);

            // Once we change the render items into instance rendering
            // they can't be changed back without being deleted and
            // re-created.  So if instances are deleted to leave only
            // one remaining, just keep treating them the instance
            // way.
            m_is_instance_mode = true;
        }
    }
}

void SubSceneOverride::addUIDrawables(
    MHWRender::MUIDrawManager &draw_manager,
    const MHWRender::MFrameContext &/*frame_context*/) {
    MPoint pos(0.0, 0.0, 0.0);
    MColor text_color(0.1f, 0.8f, 0.8f, 1.0f);
    MString text("Open Comp Graph Maya");

    draw_manager.beginDrawable();

    draw_manager.setColor(text_color);
    draw_manager.setFontSize(MHWRender::MUIDrawManager::kSmallFontSize);

    // MUIDrawManager assumes the object space of the original
    // instance. If there are multiple instances, each text needs to
    // be drawn in the origin of each instance, so we need to
    // transform the coordinates from each instance's object space to
    // the original instance's object space.
    MMatrix world_inverse0 = m_instance_info_cache[0].m_matrix.inverse();
    for (auto it = m_instance_info_cache.begin();
         it != m_instance_info_cache.end(); it++) {

        draw_manager.text((pos * it->second.m_matrix) * world_inverse0,
                          text, MHWRender::MUIDrawManager::kCenter);
    }

    draw_manager.endDrawable();

    m_are_ui_drawables_dirty = false;
}

// NOTE: This will be unneeded in Maya 2019+.
bool SubSceneOverride::getSelectionPath(
    const MHWRender::MRenderItem &/*render_item*/,
    MDagPath &dag_path) const {
    if (m_instance_dag_paths.length() == 0) return false;

    // Return the first DAG path because there is no instancing in this case.
    return MDagPath::getAPathTo(m_instance_dag_paths[0].transform(), dag_path);
}

bool SubSceneOverride::getInstancedSelectionPath(
    const MHWRender::MRenderItem &/*render_item*/,
    const MHWRender::MIntersection &intersection,
    MDagPath &dag_path) const {
    unsigned int num_instances = m_instance_dag_paths.length();
    if (num_instances == 0) return false;

    // The instance ID starts from 1 for the first DAG path. We use instanceID-1
    // as the index to DAG path array returned by MFnDagNode::getAllPaths().
    int instance_id = intersection.instanceID();
    if (instance_id > (int) num_instances) return false;

    // Return the first DAG path in case of no instancing.
    if (num_instances == 1 || instance_id == -1) {
        instance_id = 1;
    }

    return MDagPath::getAPathTo(
        m_instance_dag_paths[instance_id - 1].transform(),
        dag_path);
}

// Trigger a Graph evaluation and return the computed data.
ocg::ExecuteStatus
SubSceneOverride::evalutate_ocg_graph(
        ocg::Node stream_ocg_node,
        std::shared_ptr<ocg::Graph> shared_graph,
        std::shared_ptr<ocg::Cache> shared_cache) {
    auto log = log::get_logger();

    // bool exists = shared_graph->node_exists(stream_ocg_node);
    // log->debug(
    //     "ocgImagePlane: input node id={} node type={} exists={}",
    //     stream_ocg_node.get_id(),
    //     static_cast<uint64_t>(stream_ocg_node.get_node_type()),
    //     exists);

    auto exec_status = shared_graph->execute(stream_ocg_node, shared_cache);
    // log->debug(
    //     "ocgImagePlane: execute status={}",
    //     static_cast<uint64_t>(exec_status));

    // auto input_node_status = shared_graph->node_status(stream_ocg_node);
    // log->debug(
    //     "ocgImagePlane: input node status={}",
    //     static_cast<uint64_t>(input_node_status));
    // log->debug(
    //     "ColorGraphNode: Graph as string:\n{}",
    //     shared_graph->data_debug_string());
    if (exec_status != ocg::ExecuteStatus::kSuccess) {
        log->error("ocgImagePlane: Failed to execute OCG node network!");
    }
    return exec_status;
}

} // namespace image_plane
} // namespace open_comp_graph_maya
