// Maya
#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MDistance.h>
#include <maya/MFnStringData.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MSelectionContext.h>
#include <maya/MDagMessage.h>

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "image_plane_shape.h"

namespace open_comp_graph_maya {

// Constants for the shape node.
MTypeId ImagePlaneShape::m_id(OCGM_IMAGE_PLANE_SHAPE_TYPE_ID);
MString ImagePlaneShape::m_draw_db_classification("drawdb/subscene/ocgImagePlane_SubSceneOverride");
MString ImagePlaneShape::m_draw_registrant_id("ocgImagePlaneNode_SubSceneOverridePlugin");
MString ImagePlaneShape::m_selection_type_name("ocgImagePlaneSelection");

// Attributes
MObject ImagePlaneShape::m_size_attr;
MObject ImagePlaneShape::m_file_path_attr;
MObject ImagePlaneShape::m_exposure_attr;

// Defines the Node name as a callable static function.
MString ImagePlaneShape::nodeName() {
    return MString("ocgImagePlane");
}

ImagePlaneShape::ImagePlaneShape() {}

ImagePlaneShape::~ImagePlaneShape() {}

MStatus ImagePlaneShape::compute(const MPlug & /*plug*/, MDataBlock & /*data*/ ) {
    return MS::kUnknownParameter;
}

// Called by legacy default viewport
/*
void ImagePlaneShape::draw(M3dView &view, const MDagPath &path,
                           M3dView::DisplayStyle style,
                           M3dView::DisplayStatus status) {
    // Get the size
    MObject thisNode = thisMObject();
    MPlug plug(thisNode, m_size_attr);
    MDistance sizeVal;
    plug.getValue(sizeVal);

    float multiplier = (float) sizeVal.asCentimeters();

    view.beginGL();
    if ((style == M3dView::kFlatShaded) ||
        (style == M3dView::kGouraudShaded)) {
        // Push the color settings
        glPushAttrib(GL_CURRENT_BIT);

        if (status == M3dView::kActive) {
            view.setDrawColor(13, M3dView::kActiveColors);
        } else {
            view.setDrawColor(13, M3dView::kDormantColors);
        }

        glBegin(GL_TRIANGLE_FAN);
        int i;
        int last = shape_vertices_count - 1;
        for (i = 0; i < last; ++i) {
            glVertex3f(shape_vertices[i][0] * multiplier,
                       shape_vertices[i][1] * multiplier,
                       shape_vertices[i][2] * multiplier);
        }
        glEnd();

        glPopAttrib();
    }

    // Draw the outline
    //
    glBegin(GL_LINES);
    int i;
    int last = shape_vertices_count - 1;
    for (i = 0; i < last; ++i) {
        glVertex3f(shape_vertices[i][0] * multiplier,
                   shape_vertices[i][1] * multiplier,
                   shape_vertices[i][2] * multiplier);
        glVertex3f(shape_vertices[i + 1][0] * multiplier,
                   shape_vertices[i + 1][1] * multiplier,
                   shape_vertices[i + 1][2] * multiplier);
    }
    glEnd();

    // Draw the name of the ImagePlaneShape
    view.setDrawColor(MColor(0.1f, 0.8f, 0.8f, 1.0f));
    view.drawText(
        MString("Open Comp Graph Maya"),
        MPoint(0.0, 0.0, 0.0),
        M3dView::kCenter);

    view.endGL();
}
*/

bool ImagePlaneShape::isBounded() const {
    return true;
}

MBoundingBox ImagePlaneShape::boundingBox() const {
    // Get the size
    MObject this_node = thisMObject();
    MPlug plug(this_node, m_size_attr);
    MDistance size_value;
    plug.getValue(size_value);

    double multiplier = size_value.asCentimeters();

    MPoint corner1(-1.0, -1.0, -1.0);
    MPoint corner2(1.0, 1.0, 1.0);

    corner1 = corner1 * multiplier;
    corner2 = corner2 * multiplier;

    return MBoundingBox(corner1, corner2);
}

MSelectionMask ImagePlaneShape::getShapeSelectionMask() const {
    return MSelectionMask("ocgImagePlaneSelection");
}

void *ImagePlaneShape::creator() {
    return new ImagePlaneShape();
}

MStatus ImagePlaneShape::initialize() {
    MFnUnitAttribute    uAttr;
    MFnTypedAttribute   tAttr;
    MFnNumericAttribute nAttr;
    MFnGenericAttribute gAttr;
    MFnEnumAttribute    eAttr;
    MStatus status;

    // Create empty string data to be used as attribute default
    // (string) value.
    MFnStringData empty_string_data;
    MObject empty_string_data_obj = empty_string_data.create("");

    // Size Attribute
    m_size_attr = uAttr.create("size", "sz", MFnUnitAttribute::kDistance);
    uAttr.setDefault(1.0);
    status = MPxNode::addAttribute(m_size_attr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // File Path Attribute
    m_file_path_attr = tAttr.create(
            "filePath", "fp",
        MFnData::kString, empty_string_data_obj);
    status = tAttr.setStorable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = tAttr.setUsedAsFilename(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MPxNode::addAttribute(m_file_path_attr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Exposure Attribute
    //
    // Increase/Decrease the image brightness with EV (exposure
    // values).
    m_exposure_attr = nAttr.create("exposure", "exp", MFnNumericData::kFloat, 0.0f);
    status = nAttr.setWritable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = nAttr.setStorable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = nAttr.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MPxNode::addAttribute(m_exposure_attr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // // Time
    // m_time_attr = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0);
    // status = uAttr.setStorable(true);
    // CHECK_MSTATUS_AND_RETURN_IT(status);
    // status = MPxNode::addAttribute(m_time_attr);
    // CHECK_MSTATUS_AND_RETURN_IT(status);

    // Geometry Type Attribute
    //
    // The type of geometry that will draw the image.
    //
    // Possible values:
    // - Camera Plane
    // - Flat Plane
    // - Sphere
    // - Input Mesh
    // // aTransformType = eAttr.create("transformType", "tt", kTranslate);
    // // eAttr.addField("Translate", kTranslate);
    // // eAttr.addField("Rotate", kRotate);
    // // eAttr.addField("Scale", kScale);
    // // eAttr.addField("Shear", kShear);
    // // MPxNode::addAttribute(aTransformType);

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
