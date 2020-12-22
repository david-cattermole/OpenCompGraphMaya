#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H

// Maya
#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MDistance.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MSelectionContext.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MDrawRegistry.h>
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// STL
#include <unordered_map>

namespace open_comp_graph_maya{

// Shape Data
static const float shapeA[21][3] = {
    {0.00f,  0.0f, -0.70f},
    {0.04f,  0.0f, -0.69f},
    {0.09f,  0.0f, -0.65f},
    {0.13f,  0.0f, -0.61f},
    {0.16f,  0.0f, -0.54f},
    {0.17f,  0.0f, -0.46f},
    {0.17f,  0.0f, -0.35f},
    {0.16f,  0.0f, -0.25f},
    {0.15f,  0.0f, -0.14f},
    {0.13f,  0.0f, 0.00f},
    {0.00f,  0.0f, 0.00f},
    {-0.13f, 0.0f, 0.00f},
    {-0.15f, 0.0f, -0.14f},
    {-0.16f, 0.0f, -0.25f},
    {-0.17f, 0.0f, -0.35f},
    {-0.17f, 0.0f, -0.46f},
    {-0.16f, 0.0f, -0.54f},
    {-0.13f, 0.0f, -0.61f},
    {-0.09f, 0.0f, -0.65f},
    {-0.04f, 0.0f, -0.69f},
    {-0.00f, 0.0f, -0.70f}};
static const float shapeB[17][3] = {
    {0.00f,  0.0f, 0.06f},
    {0.13f,  0.0f, 0.06f},
    {0.14f,  0.0f, 0.15f},
    {0.14f,  0.0f, 0.21f},
    {0.13f,  0.0f, 0.25f},
    {0.11f,  0.0f, 0.28f},
    {0.09f,  0.0f, 0.29f},
    {0.04f,  0.0f, 0.30f},
    {0.00f,  0.0f, 0.30f},
    {-0.04f, 0.0f, 0.30f},
    {-0.09f, 0.0f, 0.29f},
    {-0.11f, 0.0f, 0.28f},
    {-0.13f, 0.0f, 0.25f},
    {-0.14f, 0.0f, 0.21f},
    {-0.14f, 0.0f, 0.15f},
    {-0.13f, 0.0f, 0.06f},
    {-0.00f, 0.0f, 0.06f}};
static const int shapeCountA = 21;
static const int shapeCountB = 17;

class ImagePlaneShape : public MPxLocatorNode {
public:
    ImagePlaneShape();
    ~ImagePlaneShape() override;

    MStatus compute(const MPlug &plug, MDataBlock &data) override;

    void draw(M3dView &view, const MDagPath &path,
              M3dView::DisplayStyle style,
              M3dView::DisplayStatus status) override;

    bool isBounded() const override;
    MBoundingBox boundingBox() const override;
    MSelectionMask getShapeSelectionMask() const override;
    static void *creator();
    static MStatus initialize();

    static MString nodeName();

    // Attribute MObjects
    static MObject size;

    // Node Constants.
    static MTypeId m_id;
    static MString drawDbClassification;
    static MString drawRegistrantId;
    static MString selectionTypeName;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H
