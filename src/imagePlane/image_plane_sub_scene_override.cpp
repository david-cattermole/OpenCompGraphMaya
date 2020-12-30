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

// OCG Maya
#include "constant_texture_data.h"
#include "image_plane_sub_scene_override.h"
#include "image_plane_shape.h"

// OCG
#include "opencompgraph.h"


namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MString ImagePlaneSubSceneOverride::m_texture_name = "MyColorBarsTexture";
MString ImagePlaneSubSceneOverride::m_shader_color_parameter_name = "gSolidColor";
MString ImagePlaneSubSceneOverride::m_shader_texture_parameter_name = "gTexture";
MString ImagePlaneSubSceneOverride::m_shader_texture_sampler_parameter_name = "gTextureSampler";
MString ImagePlaneSubSceneOverride::m_wireframe_render_item_name = "ocgImagePlaneWireframe";
MString ImagePlaneSubSceneOverride::m_shaded_render_item_name = "ocgImagePlaneShadedTriangles";

ImagePlaneSubSceneOverride::ImagePlaneSubSceneOverride(const MObject &obj)
        : MHWRender::MPxSubSceneOverride(obj),
          m_locator_node(obj),
          m_multiplier(0.0f),
          m_is_instance_mode(false),
          m_are_ui_drawables_dirty(true),
          m_position_buffer(nullptr),
          m_uv_buffer(nullptr),
          m_wire_index_buffer(nullptr),
          m_shaded_index_buffer(nullptr),
          m_instance_added_cb_id(0),
          m_instance_removed_cb_id(0),
          m_shader(nullptr),
          m_texture(nullptr) {
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
    ImagePlaneSubSceneOverride::deleteGeometryBuffers();

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
    std::uint32_t num_instances = m_instance_dag_paths.length();
    if (num_instances == 0) {
        if (!MDagPath::getAllPathsTo(m_locator_node, m_instance_dag_paths)) {
            MStreamUtils::stdErrorStream() << "ImagePlaneSubSceneOverride: Failed to get all DAG paths.\n";
            return;
        }
        num_instances = m_instance_dag_paths.length();
    }

    if (num_instances == 0) {
        return;
    }

    // MColor = MHWRender::MGeometryUtilities::wireframeColor(m_instance_dag_paths[0]);
    MHWRender::MShaderInstance *shader =
        ImagePlaneSubSceneOverride::compile_shaders();
    if (!shader) {
        MStreamUtils::stdErrorStream()
            << "ImagePlaneSubSceneOverride: Failed to get a shader.\n";
        return;
    }

    MPlug plug(m_locator_node, ImagePlaneShape::m_size_attr);
    float new_multiplier = 1.0f;
    if (!plug.isNull()) {
        MDistance size_val;
        if (plug.getValue(size_val)) {
            new_multiplier = static_cast<float>(size_val.asCentimeters());
        }
    }

    bool update_geometry = (container.count() == 0);
    if (m_multiplier != new_multiplier) {
        m_multiplier = new_multiplier;
        update_geometry = true;
    }
    if (update_geometry) {
        rebuildGeometryBuffers();
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
                MHWRender::MGeometry::kLines);
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
                MGeometry::kTriangles);
        shaded_item->setDrawMode(
            static_cast<MGeometry::DrawMode>(MGeometry::kShaded | MGeometry::kTextured));
        shaded_item->setExcludedFromPostEffects(false);
        shaded_item->castsShadows(true);
        shaded_item->receivesShadows(true);
        shaded_item->depthPriority(MRenderItem::sDormantFilledDepthPriority);
        container.add(shaded_item);
        items_changed = true;
    }

    if (items_changed || any_instance_changed) {
        wire_item->setShader(shader);
        shaded_item->setShader(shader);
    }

    if (items_changed || update_geometry) {
        MStatus status;
        MFnDagNode node(m_locator_node, &status);

        ImagePlaneShape *fp = status ? dynamic_cast<ImagePlaneShape *>(node.userNode()) : nullptr;
        MBoundingBox *bounds = fp ? new MBoundingBox(fp->boundingBox()) : nullptr;

        MHWRender::MVertexBufferArray vertex_buffers;
        vertex_buffers.addBuffer("positions", m_position_buffer);
        vertex_buffers.addBuffer("uvs", m_uv_buffer);
        setGeometryForRenderItem(
                *wire_item, vertex_buffers, *m_wire_index_buffer, bounds);
        setGeometryForRenderItem(
                *shaded_item, vertex_buffers, *m_shaded_index_buffer, bounds);

        if (bounds) {
            delete bounds;
        }
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
            setExtraInstanceData(*wire_item, m_shader_color_parameter_name, instance_color_array);
            setExtraInstanceData(*shaded_item, m_shader_color_parameter_name, instance_color_array);

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

    // MUIDrawManager assumes the object space of the original instance. If there
    // are multiple instances, each text needs to be drawn in the origin of each
    // instance, so we need to transform the coordinates from each instance's
    // object space to the original instance's object space.
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

void ImagePlaneSubSceneOverride::rebuildGeometryBuffers() {
    ImagePlaneSubSceneOverride::deleteGeometryBuffers();

    // VertexBuffer for positions. We concatenate the shape_vertices_b and
    // shape_vertices_a positions into a single vertex buffer.  The index
    // buffers will decide which positions will be selected for each
    // render items.
    const MHWRender::MVertexBufferDescriptor vb_desc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        3);
    m_position_buffer = new MHWRender::MVertexBuffer(vb_desc);
    if (m_position_buffer) {
        float *positions = (float *) m_position_buffer->acquire(shape_vertices_count_a + shape_vertices_count_b, true);
        if (positions) {
            int verticesPointerOffset = 0;
            for (int current_vertex = 0;
                 current_vertex < shape_vertices_count_a + shape_vertices_count_b;
                 ++current_vertex) {
                if (current_vertex < shape_vertices_count_b) {
                    int shapeBVtx = current_vertex;
                    float x = shape_vertices_b[shapeBVtx][0] * m_multiplier;
                    float y = shape_vertices_b[shapeBVtx][1] * m_multiplier;
                    float z = shape_vertices_b[shapeBVtx][2] * m_multiplier;
                    positions[verticesPointerOffset++] = x;
                    positions[verticesPointerOffset++] = y;
                    positions[verticesPointerOffset++] = z;
                } else {
                    int shapeAVtx = current_vertex - shape_vertices_count_b;
                    float x = shape_vertices_a[shapeAVtx][0] * m_multiplier;
                    float y = shape_vertices_a[shapeAVtx][1] * m_multiplier;
                    float z = shape_vertices_a[shapeAVtx][2] * m_multiplier;
                    positions[verticesPointerOffset++] = x;
                    positions[verticesPointerOffset++] = y;
                    positions[verticesPointerOffset++] = z;
                }
            }

            m_position_buffer->commit(positions);
        }
    }

    // UV Vertex Buffer
    const MHWRender::MVertexBufferDescriptor uv_desc(
        "",
        MHWRender::MGeometry::kTexture,
        MHWRender::MGeometry::kFloat,
        2);
    m_uv_buffer = new MHWRender::MVertexBuffer(uv_desc);
    if (m_uv_buffer) {
        bool write_only = true;  // We don't need the current buffer values
        float *uvs = (float *) m_uv_buffer->acquire(
                shape_uvs_count_a + shape_uvs_count_b,
                write_only);
        if (uvs) {
            int uvsPointerOffset = 0;
            for (int current_vertex = 0;
                 current_vertex < (shape_uvs_count_a + shape_uvs_count_b);
                 ++current_vertex) {
                if (current_vertex < shape_uvs_count_b) {
                    int shapeBUv = current_vertex;
                    float x = shape_uvs_b[shapeBUv][0];
                    float y = shape_uvs_b[shapeBUv][1];
                    uvs[uvsPointerOffset++] = x;
                    uvs[uvsPointerOffset++] = y;
                } else {
                    int shapeAUv = current_vertex - shape_uvs_count_b;
                    float x = shape_uvs_a[shapeAUv][0];
                    float y = shape_uvs_a[shapeAUv][1];
                    uvs[uvsPointerOffset++] = x;
                    uvs[uvsPointerOffset++] = y;
                }
            }
            m_uv_buffer->commit(uvs);
        }
    }

    // IndexBuffer for the wireframe item
    m_wire_index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (m_wire_index_buffer) {
        int primitive_index = 0;
        int start_index = 0;
        int num_primitive = (shape_vertices_count_b + shape_vertices_count_a) - 2;
        int num_index = num_primitive * 2;

        unsigned int *indices = static_cast<unsigned int *>(m_wire_index_buffer->acquire(
                num_index, true));
        if (indices) {
            for (int i = 0; i < num_index;) {
                if (i < (shape_vertices_count_b - 1) * 2) {
                    start_index = 0;
                    primitive_index = i / 2;
                } else {
                    start_index = shape_vertices_count_b;
                    primitive_index = i / 2 - shape_vertices_count_b + 1;
                }
                indices[i++] = start_index + primitive_index;
                indices[i++] = start_index + primitive_index + 1;
            }

            m_wire_index_buffer->commit(indices);
        }
    }

    // IndexBuffer for the shaded item
    m_shaded_index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (m_shaded_index_buffer) {
        int primitive_index = 0;
        int start_index = 0;
        int num_primitive = shape_vertices_count_b + shape_vertices_count_a - 4;
        int num_index = num_primitive * 3;

        unsigned int *indices = (unsigned int *) m_shaded_index_buffer->acquire(
                num_index, true);
        if (indices) {
            for (int i = 0; i < num_index;) {
                if (i < (shape_vertices_count_b - 2) * 3) {
                    start_index = 0;
                    primitive_index = i / 3;
                } else {
                    start_index = shape_vertices_count_b;
                    primitive_index = i / 3 - shape_vertices_count_b + 2;
                }
                indices[i++] = start_index;
                indices[i++] = start_index + primitive_index + 1;
                indices[i++] = start_index + primitive_index + 2;
            }

            m_shaded_index_buffer->commit(indices);
        }
    }
}

void ImagePlaneSubSceneOverride::deleteGeometryBuffers() {
    if (m_position_buffer) {
        delete m_position_buffer;
        m_position_buffer = nullptr;
    }

    if (m_uv_buffer) {
        delete m_uv_buffer;
        m_uv_buffer = nullptr;
    }

    if (m_wire_index_buffer) {
        delete m_wire_index_buffer;
        m_wire_index_buffer = nullptr;
    }

    if (m_shaded_index_buffer) {
        delete m_shaded_index_buffer;
        m_shaded_index_buffer = nullptr;
    }
}

MHWRender::MShaderInstance *
ImagePlaneSubSceneOverride::compile_shaders() {
    if (m_shader != nullptr) {
        // MStreamUtils::stdErrorStream()
        //     << "ocgImagePlane, found shader!\n";
        return m_shader;
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
    m_shader = shaderMgr->getEffectsFileShader(
            effectsFileName, techniqueName,
            macros, numberOfMacros,
            useEffectCache);
    if (!m_shader) {
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
    m_shader->parameterList(paramlist);
    for (uint32_t i = 0; i < paramlist.length(); ++i) {
        MStreamUtils::stdErrorStream()
                << "ocgImagePlane: param" << i << ": " << paramlist[i].asChar() << '\n';
    }

    // Set a color parameter.
    const float colorValues[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    status = m_shader->setParameter(
            m_shader_color_parameter_name,
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
    if (!m_texture)
    {
        // First, search the texture cache to see if another instance
        // of this override has already generated the texture. We can
        // reuse it to save GPU memory since the noise data is
        // constant.
        m_texture = textureManager->findTexture(m_texture_name);
        // Not in cache, so we need to actually build the texture.
        if (!m_texture)
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
            // m_texture = textureManager->acquireTexture(
            //     m_texture_name,
            //     desc,
            //     (const void*)&(color_bars_f32_8x8_[0]),
            //     false);

            auto graph = ocg::Graph();
            auto read_node = ocg::Node(ocg::NodeType::kReadImage, "read1");
            auto grade_node = ocg::Node(ocg::NodeType::kGrade, "grade1");
            // read_node.set_attr_str("file_path", "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/checker_8bit_rgba_3840x2160.png");
            read_node.set_attr_str("file_path", "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/oiio-images/tahoe-gps.jpg");
            grade_node.set_attr_f32("multiply", 2.0f);
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
                // auto buffer = static_cast<const void*>(color_bars_f32_8x8_[0]),
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
                m_texture = textureManager->acquireTexture(
                        m_texture_name,
                        desc,
                        buffer,
                        false);
            }
        }
    }

    // Set the shader's texture parameter to use our uploaded texture.
    if (m_texture) {
        MStreamUtils::stdErrorStream()
                << "ocgImagePlane: Setting texture parameter...\n";
        MHWRender::MTextureAssignment texResource;
        texResource.texture = m_texture;
        m_shader->setParameter(
                m_shader_texture_parameter_name,
                texResource);
        // Release our reference now that it is set on the shader
        textureManager->releaseTexture(m_texture);
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
        m_shader->setParameter(
                m_shader_texture_sampler_parameter_name,
                *sampler);
    } else {
        MStreamUtils::stdErrorStream()
                << "ocgImagePlane: Failed to get texture sampler.\n";
        MString errorMsg = MString(
                "ocgImagePlane: Failed to get texture sampler.");
        MGlobal::displayError(errorMsg);
        return nullptr;
    }

    return m_shader;
}

MStatus
ImagePlaneSubSceneOverride::release_shaders() {
    MStreamUtils::stdErrorStream()
            << "ocgImagePlane: Releasing shaders....\n";
    MHWRender::MRenderer *renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const MHWRender::MShaderManager *shader_mgr = renderer->getShaderManager();
        if (shader_mgr && m_shader) {
            shader_mgr->releaseShader(m_shader);
            return MS::kSuccess;
        }
    }
    return MS::kFailure;
}


} // namespace open_comp_graph_maya
