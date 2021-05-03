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
 * Image plane shape node.
 */

// Maya
#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MDistance.h>
#include <maya/MSelectionContext.h>
#include <maya/MDagMessage.h>
#include <maya/MEvaluationNode.h>
#include <maya/MFnStringData.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnCamera.h>
#include <maya/MFnPluginData.h>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "graph_data.h"
#include "image_plane_shape.h"
#include "comp_nodes/base_node.h"

namespace open_comp_graph_maya {
namespace image_plane {

// Constants for the shape node.
MTypeId ShapeNode::m_id(OCGM_IMAGE_PLANE_SHAPE_TYPE_ID);
MString ShapeNode::m_draw_db_classification(OCGM_IMAGE_PLANE_DRAW_CLASSIFY);
MString ShapeNode::m_draw_registrant_id(OCGM_IMAGE_PLANE_DRAW_REGISTRANT_ID);
MString ShapeNode::m_selection_type_name(OCGM_IMAGE_PLANE_SHAPE_SELECTION_TYPE_NAME);
MString ShapeNode::m_display_filter_name(OCGM_IMAGE_PLANE_SHAPE_DISPLAY_FILTER_NAME);
MString ShapeNode::m_display_filter_label(OCGM_IMAGE_PLANE_SHAPE_DISPLAY_FILTER_LABEL);

// Input Attributes
MObject ShapeNode::m_camera_attr;
MObject ShapeNode::m_in_stream_attr;
MObject ShapeNode::m_card_depth_attr;
MObject ShapeNode::m_card_size_x_attr;
MObject ShapeNode::m_card_size_y_attr;
MObject ShapeNode::m_card_res_x_attr;
MObject ShapeNode::m_card_res_y_attr;
MObject ShapeNode::m_time_attr;

// Output Attributes
MObject ShapeNode::m_out_stream_attr;

// Defines the Node name as a callable static function.
MString ShapeNode::nodeName() {
    return MString(OCGM_IMAGE_PLANE_SHAPE_TYPE_NAME);
}

ShapeNode::ShapeNode() {}

ShapeNode::~ShapeNode() {}

MStatus ShapeNode::compute(const MPlug & /*plug*/, MDataBlock & /*data*/ ) {
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
    return MS::kUnknownParameter;
}

// Called by legacy default viewport
/*
void ShapeNode::draw(M3dView &view, const MDagPath &path,
                           M3dView::DisplayStyle style,
                           M3dView::DisplayStatus status) {
    // Get the size
    MObject thisNode = thisMObject();
    MPlug plug(thisNode, m_card_size_x_attr);
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

    // Draw the name of the ShapeNode
    view.setDrawColor(MColor(0.1f, 0.8f, 0.8f, 1.0f));
    view.drawText(
        MString("Open Comp Graph Maya"),
        MPoint(0.0, 0.0, 0.0),
        M3dView::kCenter);

    view.endGL();
}
*/

bool ShapeNode::isBounded() const {
    return true;
}

MBoundingBox ShapeNode::boundingBox() const {
    MObject this_node = thisMObject();
    MPlug card_size_x_plug(this_node, m_card_size_x_attr);
    MPlug card_size_y_plug(this_node, m_card_size_y_attr);
    MDistance card_size_x_value;
    MDistance card_size_y_value;
    card_size_x_plug.getValue(card_size_x_value);
    card_size_y_plug.getValue(card_size_y_value);

    double multiplier_x = card_size_x_value.asCentimeters();
    double multiplier_y = card_size_y_value.asCentimeters();

    // TODO: Replace this with a call to the OCG library.
    MPoint corner1(-1.0 * multiplier_x, -1.0 * multiplier_y, 0.0);
    MPoint corner2(1.0 * multiplier_x, 1.0 * multiplier_y, 0.0);
    return MBoundingBox(corner1, corner2);
}

MSelectionMask ShapeNode::getShapeSelectionMask() const {
    return MSelectionMask("ocgImagePlaneSelection");
}

bool ShapeNode::excludeAsLocator() const {
    // Returning 'false' here means that when the user toggles
    // locators on/off with the (per-viewport) "Show" menu, this shape
    // node will not be affected.
    return false;
}

// Called before this node is evaluated by Evaluation Manager.
//
// Helps to trigger the node to be evaluated in viewport 2.0.
MStatus ShapeNode::preEvaluation(
        const MDGContext& context,
        const MEvaluationNode& evaluationNode) {
    if (context.isNormal()) {
        MStatus status;
        if ((evaluationNode.dirtyPlugExists(m_in_stream_attr, &status) && status)
            || (evaluationNode.dirtyPlugExists(m_out_stream_attr, &status) && status)) {
            MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
        }
    }
    return MStatus::kSuccess;
}

// Called before this node is evaluated by Evaluation Manager.
//
// Helps to trigger the node to be evaluated in viewport 2.0.
MStatus ShapeNode::postEvaluation(
        const MDGContext& context,
        const MEvaluationNode& evaluationNode,
        PostEvaluationType evalType) {
    if (context.isNormal() && evalType != kLeaveDirty) {
        MStatus status;
        if ((evaluationNode.dirtyPlugExists(m_in_stream_attr, &status) && status)
            || (evaluationNode.dirtyPlugExists(m_out_stream_attr, &status) && status)) {
            MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
        }
    }
    return MStatus::kSuccess;
}

void *ShapeNode::creator() {
    return new ShapeNode();
}

MStatus ShapeNode::initialize() {
    MStatus status;
    MFnUnitAttribute    uAttr;
    MFnTypedAttribute   tAttr;
    MFnNumericAttribute nAttr;
    MFnEnumAttribute    eAttr;
    MFnMessageAttribute mAttr;

    // Camera
    m_camera_attr = mAttr.create("camera", "cam");

    // Geometry Type Attribute
    //
    // The type of geometry that will draw the image.
    //
    // Possible values:
    // - Camera Plane
    // - Card / Flat Plane
    // - Sphere
    // - Custom Mesh
    //
    // // aTransformType = eAttr.create("transformType", "tt", kTranslate);
    // // eAttr.addField("Camera Plane", kTranslate);
    // // eAttr.addField("Card", kRotate);
    // // eAttr.addField("Sphere", kScale);
    // // eAttr.addField("Custom Mesh", kShear);
    // // addAttribute(aTransformType);

    // Card Depth Attribute
    float card_depth_min = -1000.0f;
    float card_depth_default = 1.0f;
    m_card_depth_attr = uAttr.create(
        "cardDepth", "cdph",
        MFnUnitAttribute::kDistance);
    CHECK_MSTATUS(uAttr.setMin(card_depth_min));
    CHECK_MSTATUS(uAttr.setDefault(card_depth_default));

    // Card Size Attribute
    float card_size_x_min = 0.0f;
    float card_size_x_default = 1.0f;
    m_card_size_x_attr = uAttr.create(
        "cardSizeX", "cszx",
        MFnUnitAttribute::kDistance);
    CHECK_MSTATUS(uAttr.setMin(card_size_x_min));
    CHECK_MSTATUS(uAttr.setDefault(card_size_x_default));

    // Card Size Attribute
    float card_size_y_default = 1.0f;
    float card_size_y_min = 0.0f;
    m_card_size_y_attr = uAttr.create(
        "cardSizeY", "cszy",
        MFnUnitAttribute::kDistance);
    CHECK_MSTATUS(uAttr.setMin(card_size_y_min));
    CHECK_MSTATUS(uAttr.setDefault(card_size_y_default));

    // Card Resolution X
    uint32_t card_res_x_min = 2;
    uint32_t card_res_x_soft_max = 128;
    uint32_t card_res_x_max = 2048;
    uint32_t card_res_x_default = 32;
    m_card_res_x_attr = nAttr.create(
        "cardResolutionX", "crzx",
        MFnNumericData::kInt, card_res_x_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setMin(card_res_x_min));
    CHECK_MSTATUS(nAttr.setMax(card_res_x_max));
    CHECK_MSTATUS(nAttr.setSoftMax(card_res_x_soft_max));

    // Card Resolution Y
    uint32_t card_res_y_min = 2;
    uint32_t card_res_y_soft_max = 128;
    uint32_t card_res_y_max = 2048;
    uint32_t card_res_y_default = 32;
    m_card_res_y_attr = nAttr.create(
        "cardResolutionY", "crzy",
        MFnNumericData::kInt, card_res_y_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setMin(card_res_y_min));
    CHECK_MSTATUS(nAttr.setMax(card_res_y_max));
    CHECK_MSTATUS(nAttr.setSoftMax(card_res_y_soft_max));

    // Camera Plane Depth Attribute

    // Camera Plane Resolution Attribute

    // Override Screen Depth Attribute

    // Screen Depth Attribute

    // Time
    m_time_attr = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0);
    CHECK_MSTATUS(uAttr.setStorable(true));

    // Create Common Attributes
    CHECK_MSTATUS(BaseNode::create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(BaseNode::create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_camera_attr));
    CHECK_MSTATUS(addAttribute(m_card_depth_attr));
    CHECK_MSTATUS(addAttribute(m_card_size_x_attr));
    CHECK_MSTATUS(addAttribute(m_card_size_y_attr));
    CHECK_MSTATUS(addAttribute(m_card_res_x_attr));
    CHECK_MSTATUS(addAttribute(m_card_res_y_attr));
    CHECK_MSTATUS(addAttribute(m_time_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace image_plane
} // namespace open_comp_graph_maya
