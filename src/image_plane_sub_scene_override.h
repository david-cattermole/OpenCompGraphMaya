#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_SUB_SCENE_OVERRIDE_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_SUB_SCENE_OVERRIDE_H

// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// STL
#include <map>

namespace open_comp_graph_maya{

class ImagePlaneSubSceneOverride : public MHWRender::MPxSubSceneOverride {
public:

    static MHWRender::MPxSubSceneOverride *Creator(const MObject &obj) {
        return new ImagePlaneSubSceneOverride(obj);
    }

    ~ImagePlaneSubSceneOverride() override;

    MHWRender::DrawAPI supportedDrawAPIs() const override {
        return MHWRender::kAllDevices;
    }

    bool requiresUpdate(
        const MHWRender::MSubSceneContainer &/*container*/,
        const MHWRender::MFrameContext &/*frameContext*/) const override {
        return true;
    }

    void update(
            MHWRender::MSubSceneContainer &container,
            const MHWRender::MFrameContext &frameContext) override;

    bool hasUIDrawables() const override {
        return true;
    }

    bool areUIDrawablesDirty() const override {
        return m_are_ui_drawables_dirty;
    }

    void addUIDrawables(
            MHWRender::MUIDrawManager &draw_manager,
            const MHWRender::MFrameContext &frameContext) override;

    bool getSelectionPath(
            const MHWRender::MRenderItem &renderItem,
            MDagPath &dag_path) const override;

    bool getInstancedSelectionPath(
            const MHWRender::MRenderItem &renderItem,
            const MHWRender::MIntersection &intersection,
            MDagPath &dag_path) const override;

private:

    ImagePlaneSubSceneOverride(const MObject &obj);

    // Create and delete VertexBuffers and Index Buffers, etc.
    void rebuild_geometry_buffers();
    void delete_geometry_buffers();

    MHWRender::MVertexBuffer *m_position_buffer;
    MHWRender::MVertexBuffer *m_uv_buffer;
    MHWRender::MIndexBuffer *m_wire_index_buffer;
    MHWRender::MIndexBuffer *m_shaded_index_buffer;

    // Shader compile and release.
    MStatus compile_shaders();
    MStatus release_shaders();
    MStatus set_shader_color(
            MHWRender::MShaderInstance* shader,
            const float color_values[4]);
    MStatus set_shader_texture(
            MHWRender::MShaderInstance* shader,
            MHWRender::MTexture *texture,
            MString file_path,
            float exposure);

    // Internal state.
    MObject m_locator_node;
    bool m_is_instance_mode;
    bool m_are_ui_drawables_dirty;

    // Cached attribute values
    float m_size;
    MString m_file_path;
    float m_exposure;

    struct InstanceInfo {
        MMatrix m_matrix;
        MColor m_color;

        InstanceInfo() {}

        InstanceInfo(const MMatrix &matrix,
                     const MColor &color) : m_matrix(matrix),
                                            m_color(color) {}
    };

    typedef std::map<unsigned int, InstanceInfo> InstanceInfoMap;
    InstanceInfoMap m_instance_info_cache;

    // Callbacks on instance added/removed.
    static void InstanceChangedCallback(MDagPath &child, MDagPath &parent,
                                        void *clientData);

    MCallbackId m_instance_added_cb_id;
    MCallbackId m_instance_removed_cb_id;
    MDagPathArray m_instance_dag_paths;

    // Shaders and Textures
    MHWRender::MShaderInstance *m_shader;
    MHWRender::MTexture *m_texture;

    // Shader Constants
    static MString m_texture_name;
    static MString m_shader_color_parameter_name;
    static MString m_shader_texture_parameter_name;
    static MString m_shader_texture_sampler_parameter_name;
    static MString m_wireframe_render_item_name;
    static MString m_shaded_render_item_name;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SUB_SCENE_OVERRIDE_H
