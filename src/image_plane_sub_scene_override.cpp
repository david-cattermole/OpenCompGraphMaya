/*
 * Copyright (C) 2020 David Cattermole.
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

MString ImagePlaneSubSceneOverride::m_texture_name = "";
MString ImagePlaneSubSceneOverride::m_shader_color_parameter_name = "gSolidColor";
MString ImagePlaneSubSceneOverride::m_shader_texture_parameter_name = "gTexture";
MString ImagePlaneSubSceneOverride::m_shader_texture_sampler_parameter_name = "gTextureSampler";
MString ImagePlaneSubSceneOverride::m_wireframe_render_item_name = "ocgImagePlaneWireframe";
MString ImagePlaneSubSceneOverride::m_shaded_render_item_name = "ocgImagePlaneShadedTriangles";

ImagePlaneSubSceneOverride::ImagePlaneSubSceneOverride(const MObject &obj)
        : MHWRender::MPxSubSceneOverride(obj),
          m_locator_node(obj),
          m_card_size_x(0.0f),
          m_card_res_x(16),
          m_time(0.0f),
          m_is_instance_mode(false),
          m_are_ui_drawables_dirty(true),
          m_position_buffer(nullptr),
          m_uv_buffer(nullptr),
          m_shaded_index_buffer(nullptr),
          m_instance_added_cb_id(0),
          m_instance_removed_cb_id(0),
          m_shader(nullptr),
          m_texture(nullptr),
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

ImagePlaneSubSceneOverride::~ImagePlaneSubSceneOverride() {
    ImagePlaneSubSceneOverride::release_shaders();
    ImagePlaneSubSceneOverride::delete_geometry_buffers();

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
void ImagePlaneSubSceneOverride::InstanceChangedCallback(
    MDagPath &/*child*/,
    MDagPath &/*parent*/,
    void *clientData) {
    ImagePlaneSubSceneOverride *ovr = static_cast<ImagePlaneSubSceneOverride *>(clientData);
    if (ovr) {
        ovr->m_instance_dag_paths.clear();
    }
}

void ImagePlaneSubSceneOverride::update(
    MHWRender::MSubSceneContainer &container,
    const MHWRender::MFrameContext &/*frame_context*/) {
    auto log = log::get_logger();
    MStatus status;

    std::uint32_t num_instances = m_instance_dag_paths.length();
    if (num_instances == 0) {
        if (!MDagPath::getAllPathsTo(m_locator_node, m_instance_dag_paths)) {
            log->error("ImagePlaneSubSceneOverride: Failed to get all DAG paths.");
            return;
        }
        num_instances = m_instance_dag_paths.length();
    }

    if (num_instances == 0) {
        return;
    }

    uint32_t attr_values_changed = 0;
    uint32_t shader_values_changed = 0;
    uint32_t geometry_values_changed = 0;

    // Get card_size_x attribute value.
    MPlug card_size_x_plug(ImagePlaneSubSceneOverride::m_locator_node,
                           ImagePlaneShape::m_card_size_x_attr);
    if (!card_size_x_plug.isNull()) {
        float new_card_size_x = 0.0f;
        MDistance card_size_x_value;
        if (card_size_x_plug.getValue(card_size_x_value)) {
            new_card_size_x = static_cast<float>(card_size_x_value.asCentimeters());
        }
        bool card_size_x_has_changed = m_card_size_x != new_card_size_x;
        attr_values_changed += static_cast<uint32_t>(card_size_x_has_changed);
        geometry_values_changed += static_cast<uint32_t>(card_size_x_has_changed);
        if (card_size_x_has_changed) {
            m_card_size_x = new_card_size_x;
        }
    }

    // Get Card Resolution X attribute value.
    MPlug card_res_x_plug(ImagePlaneSubSceneOverride::m_locator_node,
                          ImagePlaneShape::m_card_res_x_attr);
    if (!card_res_x_plug.isNull()) {
        uint32_t new_card_res_x = card_res_x_plug.asInt(&status);
        CHECK_MSTATUS(status);
        bool card_res_x_has_changed = m_card_res_x != new_card_res_x;
        attr_values_changed += static_cast<uint32_t>(card_res_x_has_changed);
        geometry_values_changed += static_cast<uint32_t>(card_res_x_has_changed);
        if (card_res_x_has_changed) {
            m_card_res_x = new_card_res_x;
        }
    }

    // Get time attribute value.
    MPlug time_plug(ImagePlaneSubSceneOverride::m_locator_node,
                    ImagePlaneShape::m_time_attr);
    if (!time_plug.isNull()) {
        float new_time = time_plug.asFloat(&status);
        CHECK_MSTATUS(status);
        bool time_has_changed = m_time != new_time;
        attr_values_changed += static_cast<uint32_t>(time_has_changed);
        shader_values_changed += static_cast<uint32_t>(time_has_changed);
        if (time_has_changed) {
            m_time = new_time;
        }
    }

    // Get input OCG Stream data.
    std::shared_ptr<ocg::Graph> shared_graph;
    MPlug in_stream_plug(ImagePlaneSubSceneOverride::m_locator_node,
                         ImagePlaneShape::m_in_stream_attr);
    if (!in_stream_plug.isNull()) {
        MObject new_in_stream = in_stream_plug.asMObject(&status);
        CHECK_MSTATUS(status);
        if (new_in_stream.isNull()) {
            log->error("ImagePlaneSubSceneOverride: Input stream is not valid.");
            return;
        }

        // Convert Maya controlled data into the OCG custom MPxData class.
        // We are ensured this is valid from Maya. The MObject is a smart
        // pointer and we check the object is valid before-hand too.
        MFnPluginData fn_plugin_data(new_in_stream);
        GraphMayaData* input_stream_data =
            static_cast<GraphMayaData*>(fn_plugin_data.data(&status));
        CHECK_MSTATUS(status);
        if (input_stream_data == nullptr) {
            log->debug("ImagePlaneSubSceneOverride: Input stream data is not valid.");
            return;
        }
        log->debug("ImagePlaneSubSceneOverride: input graph is valid.");
        shared_graph = input_stream_data->get_graph();
        ocg::Node new_in_stream_node = input_stream_data->get_node();
        log->debug(
            "ImagePlaneSubSceneOverride: input node id: {}",
            new_in_stream_node.get_id());

        bool in_stream_has_changed = shared_graph->state() != ocg::GraphState::kClean;
        attr_values_changed += static_cast<uint32_t>(in_stream_has_changed);
        shader_values_changed += static_cast<uint32_t>(in_stream_has_changed);
        if (in_stream_has_changed) {
            m_in_stream = new_in_stream;
            m_in_stream_node = new_in_stream_node;
        }
    }

    // Have the attribute values changed?
    bool update_geometry = container.count() == 0;
    bool update_shader = container.count() == 0;
    if (geometry_values_changed > 0) {
        update_geometry = true;
    }
    if (shader_values_changed > 0) {
        update_shader = true;
    }
    if (update_geometry) {
        // TODO: Split out the difference between topology and vertex
        // data changing. We can update the vertex buffer without
        // needing to change the index buffers.
        ImagePlaneSubSceneOverride::rebuild_geometry_buffers(
            m_card_res_x, m_card_res_x);
    }

    // Compile and update shader.
    status = ImagePlaneSubSceneOverride::compile_shaders();
    if (!m_shader) {
        log->error("ImagePlaneSubSceneOverride: Failed to get a shader.");
        return;
    }
    if (update_shader) {
        log->debug("ImagePlaneSubSceneOverride: Update shader parameters...");
        // MColor my_color = MHWRender::MGeometryUtilities::wireframeColor(m_instance_dag_paths[0]);
        const float color_values[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        ImagePlaneSubSceneOverride::set_shader_color(m_shader, color_values);
        // TODO: Set transform matrix.
        // TODO: Set color matrix.
        ImagePlaneSubSceneOverride::set_shader_texture(
            m_shader, m_texture, shared_graph, m_in_stream_node);
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
        wire_item->setShader(m_shader);
        shaded_item->setShader(m_shader);
    }

    if (items_changed || update_geometry) {
        MFnDagNode node(m_locator_node, &status);

        ImagePlaneShape *fp = status ? dynamic_cast<ImagePlaneShape *>(node.userNode()) : nullptr;
        MBoundingBox *bounds = fp ? new MBoundingBox(fp->boundingBox()) : nullptr;

        MHWRender::MVertexBufferArray vertex_buffers;
        vertex_buffers.addBuffer("positions", m_position_buffer);
        vertex_buffers.addBuffer("uvs", m_uv_buffer);
        setGeometryForRenderItem(
                *wire_item, vertex_buffers, *m_shaded_index_buffer, bounds);
        setGeometryForRenderItem(
                *shaded_item, vertex_buffers, *m_shaded_index_buffer, bounds);

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

void ImagePlaneSubSceneOverride::addUIDrawables(
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
bool ImagePlaneSubSceneOverride::getSelectionPath(
    const MHWRender::MRenderItem &/*render_item*/,
    MDagPath &dag_path) const {
    if (m_instance_dag_paths.length() == 0) return false;

    // Return the first DAG path because there is no instancing in this case.
    return MDagPath::getAPathTo(m_instance_dag_paths[0].transform(), dag_path);
}

bool ImagePlaneSubSceneOverride::getInstancedSelectionPath(
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

void ImagePlaneSubSceneOverride::rebuild_geometry_buffers(
    const size_t divisions_x,
    const size_t divisions_y) {
    auto log = log::get_logger();
    ImagePlaneSubSceneOverride::delete_geometry_buffers();

    log->debug("ocgImagePlane: rebuild geometry buffers");
    log->debug("ocgImagePlane: divisions: {}x{}",
               divisions_x, divisions_y);

    const auto per_vertex_pos_count = 3;
    const auto per_vertex_uv_count = 2;

    // VertexBuffer for positions. The index buffers will decide which
    // positions will be selected for each render items.
    const MHWRender::MVertexBufferDescriptor vb_desc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        per_vertex_pos_count);
    m_position_buffer = new MHWRender::MVertexBuffer(vb_desc);
    if (m_position_buffer) {
        auto pos_buffer_size = ocg::internal::calc_buffer_size_vertex_positions(
            divisions_x, divisions_y);
        auto pos_count = ocg::internal::calc_count_vertex_positions(
            divisions_x, divisions_y);
        bool write_only = true;  // We don't need the current buffer values
        float *buffer = static_cast<float *>(
            m_position_buffer->acquire(pos_count, write_only));
        if (buffer) {
            rust::Slice<float> slice{buffer, pos_buffer_size};
            ocg::internal::fill_buffer_vertex_positions(
                divisions_x, divisions_y, slice);
            // for (int i = 0; i < pos_count; ++i) {
            //     int index = i * per_vertex_pos_count;
            //     log->debug(
            //         "ocgImagePlane: positions: {}={} {}={}",
            //         index + 0, buffer[index + 0],
            //         index + 1, buffer[index + 1],
            //         index + 2, buffer[index + 2]
            //     );
            // }
            m_position_buffer->commit(buffer);
        }
    }

    // UV Vertex Buffer
    const MHWRender::MVertexBufferDescriptor uv_desc(
        "",
        MHWRender::MGeometry::kTexture,
        MHWRender::MGeometry::kFloat,
        per_vertex_uv_count);
    m_uv_buffer = new MHWRender::MVertexBuffer(uv_desc);
    if (m_uv_buffer) {
        auto uv_buffer_size = ocg::internal::calc_buffer_size_vertex_uvs(
            divisions_x, divisions_y);
        auto uv_count = ocg::internal::calc_count_vertex_uvs(
            divisions_x, divisions_y);
        bool write_only = true;  // We don't need the current buffer values
        float *buffer = static_cast<float *>(
            m_uv_buffer->acquire(uv_count, write_only));
        if (buffer) {
            rust::Slice<float> slice{buffer, uv_buffer_size};
            ocg::internal::fill_buffer_vertex_uvs(
                divisions_x, divisions_y, slice);
            // for (int i = 0; i < uv_count; ++i) {
            //     int index = i * per_vertex_uv_count;
            //     log->debug(
            //         "ocgImagePlane: uvs: {}={} {}={}",
            //         index + 0, buffer[index + 0],
            //         index + 1, buffer[index + 1]
            //     );
            // }
            m_uv_buffer->commit(buffer);
        }
    }

    // IndexBuffer for the shaded item
    m_shaded_index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (m_shaded_index_buffer) {
        auto tri_count = ocg::internal::calc_buffer_size_index_tris(
            divisions_x, divisions_y);
        bool write_only = true;  // We don't need the current buffer values
        uint32_t *buffer = static_cast<uint32_t *>(
            m_shaded_index_buffer->acquire(tri_count, write_only));
        if (buffer) {
            rust::Slice<uint32_t> slice{buffer, tri_count};
            ocg::internal::fill_buffer_index_tris(
                divisions_x, divisions_y, slice);
            // for (int i = 0; i < tri_count; ++i) {
            //     log->debug("ocgImagePlane: indices {}={}",
            //                i, buffer[i]);
            // }
            m_shaded_index_buffer->commit(buffer);
        }
    }
}

void ImagePlaneSubSceneOverride::delete_geometry_buffers() {
    auto log = log::get_logger();
    log->debug("ocgImagePlane: delete geometry buffers");

    delete m_position_buffer;
    m_position_buffer = nullptr;

    delete m_uv_buffer;
    m_uv_buffer = nullptr;

    delete m_shaded_index_buffer;
    m_shaded_index_buffer = nullptr;
}

MStatus
ImagePlaneSubSceneOverride::compile_shaders() {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;
    if (m_shader != nullptr) {
        // log->debug("ocgImagePlane, found shader!");
        return status;
    } else {
        log->debug("ocgImagePlane compiling shader...");
    }

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        log->error("ocgImagePlane failed to get renderer.");
        return MS::kFailure;
    }

    // If not core profile: ogsfx is not available save effect name
    // and leave.
    if (renderer->drawAPI() != MHWRender::kOpenGLCoreProfile) {
        log->warn("ocgImagePlane is only supported with OpenGL Core Profile!");
        return MS::kFailure;
    }

    const MHWRender::MShaderManager *shader_manager = renderer->getShaderManager();
    if (!shader_manager) {
        log->error("ocgImagePlane failed get shader manager.");
        return MS::kFailure;
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
    M3dView view = M3dView::active3dView(&status);
    if (status != MStatus::kSuccess) {
        return status;
    }
    view.makeSharedContextCurrent();

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
    const MString effects_file_name("ocgImagePlane");
    MShaderCompileMacro *macros = nullptr;
    unsigned int number_of_macros = 0;
    bool use_effect_cache = true;

    // Get Techniques.
    log->debug("ocgImagePlane: Get techniques...");
    MStringArray technique_names;
    shader_manager->getEffectsTechniques(
        effects_file_name,
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
        effects_file_name, technique_name,
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

/// Set a color parameter.
MStatus
ImagePlaneSubSceneOverride::set_shader_color(
        MHWRender::MShaderInstance* shader,
        const float color_values[4]) {
    MStatus status = shader->setParameter(
        m_shader_color_parameter_name,
        color_values);
    if (status != MStatus::kSuccess) {
        auto log = log::get_logger();
        log->error("ocgImagePlane: Failed to set color parameter!");
    }
    return status;
}

MStatus
ImagePlaneSubSceneOverride::set_shader_texture(
        MHWRender::MShaderInstance* shader,
        MHWRender::MTexture *texture,
        std::shared_ptr<ocg::Graph> shared_graph,
        ocg::Node input_stream_ocg_node) {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        log->error("ocgImagePlane: Failed to get renderer.");
        return MS::kFailure
    }

    MHWRender::MTextureManager* texture_manager =
        renderer->getTextureManager();
    if (!texture_manager) {
        log->error("ocgImagePlane: Failed to get texture manager.");
        return MS::kFailure;
    }

    // Upload Texture data to the GPU using Maya's API.
    //
    // NOTE: We do not use Maya's MTextureManager class for any caching,
    // we want to use our own custom caching.
    if (!texture) {
        bool exists = shared_graph->node_exists(input_stream_ocg_node);
        log->debug(
            "ocgImagePlane: input node id={} node type={} exists={}",
            input_stream_ocg_node.get_id(),
            static_cast<uint64_t>(input_stream_ocg_node.get_node_type()),
            exists);

        auto exec_status = shared_graph->execute(input_stream_ocg_node, m_ocg_cache);
        log->debug(
            "ocgImagePlane: execute status={}",
            static_cast<uint64_t>(exec_status));
        auto input_node_status = shared_graph->node_status(input_stream_ocg_node);
        log->debug(
            "ocgImagePlane: input node status={}",
            static_cast<uint64_t>(input_node_status));
        log->debug(
            "ColorGraphNode: Graph as string:\n{}",
            shared_graph->data_debug_string());
        if (exec_status != ocg::ExecuteStatus::kSuccess) {
            log->error("ocgImagePlane: Failed to execute OCG node network!");
            return MS::kFailure;
        }

        // Get the computed data from the ocg::Graph.
        auto stream_data = shared_graph->output_stream();
        auto display_window = stream_data.display_window();
        auto data_window = stream_data.data_window();
        auto pixel_buffer = stream_data.pixel_buffer();
        auto pixel_width = stream_data.pixel_width();
        auto pixel_height = stream_data.pixel_height();
        auto pixel_num_channels = stream_data.pixel_num_channels();
        log->debug("pixels: {}x{} c={}",
                   pixel_width,  pixel_height,
                   static_cast<uint32_t>(pixel_num_channels));
        auto buffer = static_cast<const void*>(pixel_buffer.data());

        // Upload Texture via Maya.
        //
        // See for details of values:
        // http://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_class_m_h_w_render_1_1_texture_description_html
        MHWRender::MTextureDescription texture_description;
        texture_description.setToDefault2DTexture();
        texture_description.fWidth = pixel_width;
        texture_description.fHeight = pixel_height;
        if (pixel_num_channels == 3) {
            texture_description.fFormat = MHWRender::kR32G32B32_FLOAT;
        } else {
            texture_description.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        }
        texture_description.fMipmaps = 1;
        texture = texture_manager->acquireTexture(
            m_texture_name,
            texture_description,
            buffer,
            false);
    }

    if (!texture) {
        log->error("ocgImagePlane: Failed to acquire texture.");
        return MS::kFailure;
    }

    // Set the shader's texture parameter to use our uploaded texture.
    log->debug("ocgImagePlane: Setting texture parameter...");
    MHWRender::MTextureAssignment texture_resource;
    texture_resource.texture = texture;
    shader->setParameter(
        m_shader_texture_parameter_name,
        texture_resource);
    // Release our reference now that it is set on the shader
    texture_manager->releaseTexture(texture);

    // Acquire and bind the default texture sampler.
    MHWRender::MSamplerStateDesc sampler_desc;
    sampler_desc.filter = MHWRender::MSamplerState::kMinMagMipPoint;
    sampler_desc.addressU = MHWRender::MSamplerState::kTexClamp;
    sampler_desc.addressV = MHWRender::MSamplerState::kTexClamp;
    const MHWRender::MSamplerState* sampler =
        MHWRender::MStateManager::acquireSamplerState(sampler_desc);
    if (sampler) {
        log->debug("ocgImagePlane: Setting texture sampler parameter...");
        shader->setParameter(
            m_shader_texture_sampler_parameter_name,
            *sampler);
    } else {
        log->error("ocgImagePlane: Failed to get texture sampler.");
        return MS::kFailure;
    }
    return status;
}

MStatus
ImagePlaneSubSceneOverride::release_shaders() {
    auto log = log::get_logger();
    MStatus status = MS::kSuccess;
    log->debug("ocgImagePlane: Releasing shader...");

    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        log->error("ocgImagePlane: Failed to get renderer.");
        return MS::kFailure;
    }

    const MHWRender::MShaderManager *shader_manager = renderer->getShaderManager();
    if (!shader_manager) {
        log->error("ocgImagePlane: Failed to get shader manager.");
        return MS::kFailure;
    }

    if (!m_shader) {
        log->error("ocgImagePlane: Failed to release shader.");
        return MS::kFailure;
    }

    shader_manager->releaseShader(m_shader);
    return status;
}


} // namespace open_comp_graph_maya
