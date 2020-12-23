
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
#include <maya/MFnDagNode.h>
#include <maya/MSelectionContext.h>
#include <maya/MDagMessage.h>

// STL
#include <unordered_map>

// OCG Maya
#include <OpenCompGraphMaya/nodeTypeIds.h>
#include "shape.h"

namespace open_comp_graph_maya {

// Constants for the shape node.
MTypeId ImagePlaneShape::m_id(OCGM_IMAGE_PLANE_SHAPE_TYPE_ID);
MString ImagePlaneShape::drawDbClassification(
    "drawdb/subscene/ocgImagePlane_SubSceneOverride");
MString ImagePlaneShape::drawRegistrantId(
    "ocgImagePlaneNode_SubSceneOverridePlugin");
MString ImagePlaneShape::selectionTypeName(
    "ocgImagePlaneSelection");

// Attributes
MObject ImagePlaneShape::size;

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
void ImagePlaneShape::draw(M3dView &view, const MDagPath & /*path*/,
                           M3dView::DisplayStyle style,
                           M3dView::DisplayStatus status) {
    // Get the size
    MObject thisNode = thisMObject();
    MPlug plug(thisNode, size);
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
        int last = shapeVerticesCountA - 1;
        for (i = 0; i < last; ++i) {
            glVertex3f(shapeVerticesA[i][0] * multiplier,
                       shapeVerticesA[i][1] * multiplier,
                       shapeVerticesA[i][2] * multiplier);
        }
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        last = shapeVerticesCountB - 1;
        for (i = 0; i < last; ++i) {
            glVertex3f(shapeVerticesB[i][0] * multiplier,
                       shapeVerticesB[i][1] * multiplier,
                       shapeVerticesB[i][2] * multiplier);
        }
        glEnd();

        glPopAttrib();
    }

    // Draw the outline
    //
    glBegin(GL_LINES);
    int i;
    int last = shapeVerticesCountA - 1;
    for (i = 0; i < last; ++i) {
        glVertex3f(shapeVerticesA[i][0] * multiplier,
                   shapeVerticesA[i][1] * multiplier,
                   shapeVerticesA[i][2] * multiplier);
        glVertex3f(shapeVerticesA[i + 1][0] * multiplier,
                   shapeVerticesA[i + 1][1] * multiplier,
                   shapeVerticesA[i + 1][2] * multiplier);
    }
    last = shapeVerticesCountB - 1;
    for (i = 0; i < last; ++i) {
        glVertex3f(shapeVerticesB[i][0] * multiplier,
                   shapeVerticesB[i][1] * multiplier,
                   shapeVerticesB[i][2] * multiplier);
        glVertex3f(shapeVerticesB[i + 1][0] * multiplier,
                   shapeVerticesB[i + 1][1] * multiplier,
                   shapeVerticesB[i + 1][2] * multiplier);
    }
    glEnd();

    // Draw the name of the ImagePlaneShape
    view.setDrawColor(MColor(0.1f, 0.8f, 0.8f, 1.0f));
    view.drawText(MString("Open Comp Graph Maya"), MPoint(0.0, 0.0, 0.0),
                  M3dView::kCenter);

    view.endGL();
}

bool ImagePlaneShape::isBounded() const {
    return true;
}

MBoundingBox ImagePlaneShape::boundingBox() const {
    // Get the size
    MObject thisNode = thisMObject();
    MPlug plug(thisNode, size);
    MDistance sizeVal;
    plug.getValue(sizeVal);

    double multiplier = sizeVal.asCentimeters();

    MPoint corner1(-0.17, 0.0, -0.7);
    MPoint corner2(0.17, 0.0, 0.3);

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
    MFnUnitAttribute unitFn;
    MStatus stat;

    size = unitFn.create("size", "sz", MFnUnitAttribute::kDistance);
    unitFn.setDefault(1.0);

    stat = addAttribute(size);
    if (!stat) {
        stat.perror("addAttribute");
        return stat;
    }

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya 
