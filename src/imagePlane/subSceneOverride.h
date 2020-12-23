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
#include <unordered_map>

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
        return fAreUIDrawablesDirty;
    }

    void addUIDrawables(
            MHWRender::MUIDrawManager &drawManager,
            const MHWRender::MFrameContext &frameContext) override;

    bool getSelectionPath(
            const MHWRender::MRenderItem &renderItem,
            MDagPath &dagPath) const override;

    bool getInstancedSelectionPath(
            const MHWRender::MRenderItem &renderItem,
            const MHWRender::MIntersection &intersection,
            MDagPath &dagPath) const override;

private:

    ImagePlaneSubSceneOverride(const MObject &obj);

    // Create and delete VB/IB
    void rebuildGeometryBuffers();

    void deleteGeometryBuffers();

    MObject fLocatorNode;
    float fMultiplier;
    bool fIsInstanceMode;
    bool fAreUIDrawablesDirty;

    MHWRender::MVertexBuffer *fPositionBuffer;
    MHWRender::MIndexBuffer *fWireIndexBuffer;
    MHWRender::MIndexBuffer *fShadedIndexBuffer;

    struct InstanceInfo {
        MMatrix fMatrix;
        MColor fColor;

        InstanceInfo() {}

        InstanceInfo(const MMatrix &m, const MColor &c) : fMatrix(m),
                                                          fColor(c) {}
    };

    typedef std::map<unsigned int, InstanceInfo> InstanceInfoMap;
    InstanceInfoMap fInstanceInfoCache;

    // Callbacks on instance added/removed.
    static void InstanceChangedCallback(MDagPath &child, MDagPath &parent,
                                        void *clientData);

    MCallbackId fInstanceAddedCbId;
    MCallbackId fInstanceRemovedCbId;
    MDagPathArray fInstanceDagPaths;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SUB_SCENE_OVERRIDE_H
