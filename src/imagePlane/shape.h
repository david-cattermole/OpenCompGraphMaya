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
static const float shape_vertices_a[6][3] = {
    // First Triangle - counter clockwise direction.
    {-1.0, -1.0, 0.0}, // Lower left
    {1.0, 1.0, 0.0},   // Upper right
    {-1.0, 1.0, 0.0},  // Upper left

    // Second Triangle - counter clockwise direction.
    {-1.0, -1.0, 0.0}, // Lower left
    {1.0, -1.0, 0.0},  // Lower Right
    {1.0, 1.0, 0.0}};  // Upper right
static const int shape_vertices_count_a = 6;

static const float shape_uvs_a[6][2] = {
   // First Triangle - counter clockwise direction.
   {0.0, 0.0}, // Lower left
   {1.0, 1.0}, // Upper right
   {0.0, 1.0}, // Upper left

   // Second Triangle - counter clockwise direction.
   {0.0, 0.0},  // Lower left
   {1.0, 0.0},  // Lower Right
   {1.0, 1.0}}; // Upper right
static const int shape_uvs_count_a = 6;

static const float shape_vertices_b[17][3] = {
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
static const int shape_vertices_count_b = 17;

static const float shape_uvs_b[17][2] = {
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f},
   {0.0f, 0.0f}};
static const int shape_uvs_count_b = 17;

class ImagePlaneShape : public MPxLocatorNode {
public:
    ImagePlaneShape();
    ~ImagePlaneShape() override;

    MStatus compute(const MPlug &plug, MDataBlock &data) override;

    // void draw(M3dView &view, const MDagPath &path,
    //           M3dView::DisplayStyle style,
    //           M3dView::DisplayStatus status) override;

    bool isBounded() const override;
    MBoundingBox boundingBox() const override;
    MSelectionMask getShapeSelectionMask() const override;
    static void *creator();
    static MStatus initialize();

    static MString nodeName();

    // Attribute MObjects
    static MObject m_size_attr;
    static MObject m_file_path_attr;
    static MObject m_exposure_attr;

    // Node Constants.
    static MTypeId m_id;
    static MString m_draw_db_classification;
    static MString m_draw_registrant_id;
    static MString m_selection_type_name;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H
