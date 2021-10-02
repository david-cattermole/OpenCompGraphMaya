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
#include "logger.h"
#include "graph_data.h"
#include "image_plane_shape.h"
#include "attr_utils.h"
#include "../node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

// Constants for the shape node.
MTypeId ShapeNode::m_id(OCGM_IMAGE_PLANE_SHAPE_TYPE_ID);
MString ShapeNode::m_draw_db_classification(OCGM_IMAGE_PLANE_DRAW_CLASSIFY);
MString ShapeNode::m_draw_registrant_id(OCGM_IMAGE_PLANE_DRAW_REGISTRANT_ID);
MString ShapeNode::m_selection_type_name(OCGM_IMAGE_PLANE_SHAPE_SELECTION_TYPE_NAME);
MString ShapeNode::m_display_filter_name(OCGM_IMAGE_PLANE_SHAPE_DISPLAY_FILTER_NAME);
MString ShapeNode::m_display_filter_label(OCGM_IMAGE_PLANE_SHAPE_DISPLAY_FILTER_LABEL);

// Precompute index for enum.
const int32_t kBakeOptionNothing = static_cast<int32_t>(ocg::BakeOption::kNothing);
const int32_t kBakeOptionColorSpace = static_cast<int32_t>(ocg::BakeOption::kColorSpace);
const int32_t kBakeOptionColorSpaceAndGrade = static_cast<int32_t>(ocg::BakeOption::kColorSpaceAndGrade);
const int32_t kBakeOptionAll = static_cast<int32_t>(ocg::BakeOption::kAll);

const int32_t kDataTypeFloat32 = static_cast<int32_t>(ocg::DataType::kFloat32);
const int32_t kDataTypeHalf16 = static_cast<int32_t>(ocg::DataType::kHalf16);
const int32_t kDataTypeUInt8 = static_cast<int32_t>(ocg::DataType::kUInt8);
const int32_t kDataTypeUInt16 = static_cast<int32_t>(ocg::DataType::kUInt16);
const int32_t kDataTypeUnknown = static_cast<int32_t>(ocg::DataType::kUnknown);

// Input Attributes
MObject ShapeNode::m_camera_attr;
MObject ShapeNode::m_in_stream_attr;
MObject ShapeNode::m_display_mode_attr;
MObject ShapeNode::m_display_color_attr;
MObject ShapeNode::m_display_alpha_attr;
MObject ShapeNode::m_display_saturation_attr;
MObject ShapeNode::m_display_exposure_attr;
MObject ShapeNode::m_display_gamma_attr;
MObject ShapeNode::m_display_soft_clip_attr;
MObject ShapeNode::m_display_use_draw_depth_attr;
MObject ShapeNode::m_display_draw_depth_attr;
MObject ShapeNode::m_card_depth_attr;
MObject ShapeNode::m_card_size_x_attr;
MObject ShapeNode::m_card_size_y_attr;
MObject ShapeNode::m_card_res_x_attr;
MObject ShapeNode::m_card_res_y_attr;
MObject ShapeNode::m_color_space_name_attr;
MObject ShapeNode::m_lut_edge_size_attr;
MObject ShapeNode::m_cache_option_attr;
MObject ShapeNode::m_cache_pixel_data_type_attr;
MObject ShapeNode::m_cache_crop_on_format_attr;
MObject ShapeNode::m_disk_cache_enable_attr;
MObject ShapeNode::m_disk_cache_file_path_attr;
MObject ShapeNode::m_time_attr;

// Output Attributes
MObject ShapeNode::m_out_stream_attr;

// Defines the Node name as a callable static function.
MString ShapeNode::nodeName() {
    return MString(OCGM_IMAGE_PLANE_SHAPE_TYPE_NAME);
}

ShapeNode::ShapeNode()
        : m_node_uuid()
        , m_out_stream_node(ocg::Node(ocg::NodeType::kNull, 0))
{}

ShapeNode::~ShapeNode() {}

// Called after the node is created.
void ShapeNode::postConstructor() {
    MObject this_node = ShapeNode::thisMObject();
    MStatus status = MS::kSuccess;

    MFnDependencyNode fn_depend_node(this_node, &status);
    CHECK_MSTATUS(status);

    // Get Node UUID
    m_node_uuid = fn_depend_node.uuid();
};

MStatus ShapeNode::compute(const MPlug & plug, MDataBlock & data) {
    auto log = log::get_logger();
    MStatus status = MS::kUnknownParameter;

    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());

    const MUuid empty_uuid = MUuid();
    if (m_node_uuid == empty_uuid) {
        // No OCG hash has been created yet, this node is not ready
        // to be computed.
        return status;
    }

    // Pass the output node though the Maya DG.
    if (plug == m_out_stream_attr) {
        auto shared_graph = get_shared_graph();
        if (shared_graph) {
            if ((m_out_stream_node.get_id() != 0)
                    && shared_graph->node_exists(m_out_stream_node)) {
                log->debug(
                    "ImagePlaneShape: Graph as string:\n{}",
                    shared_graph->data_debug_string());

                // Create initial plug-in data structure. We don't need to
                // 'new' the data type directly.
                MFnPluginData fn_plugin_data;
                MTypeId data_type_id(OCGM_GRAPH_DATA_TYPE_ID);
                fn_plugin_data.create(data_type_id, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                // Output Stream
                MDataHandle out_stream_handle = data.outputValue(m_out_stream_attr);
                GraphData* new_data =
                    static_cast<GraphData*>(fn_plugin_data.data(&status));
                CHECK_MSTATUS_AND_RETURN_IT(status);
                new_data->set_node(m_out_stream_node);
                out_stream_handle.setMPxData(new_data);
                out_stream_handle.setClean();
                status = MS::kSuccess;
            }
        }
    }

    return status;
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

// Called after this node is evaluated by Evaluation Manager.
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


    // Image Channels
    //
    // Allows to re-interpret the input stream channels. Allows users
    // to use or disable the stream alpha channel.
    //
    m_display_mode_attr = eAttr.create(
        "displayMode", "dspmd", 0);
    CHECK_MSTATUS(eAttr.addField("rgba", 0));
    CHECK_MSTATUS(eAttr.addField("rgb", 1));
    CHECK_MSTATUS(eAttr.addField("r", 2));
    CHECK_MSTATUS(eAttr.addField("g", 3));
    CHECK_MSTATUS(eAttr.addField("b", 4));
    CHECK_MSTATUS(eAttr.addField("a", 5));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // Add 'color' attribute.
    m_display_color_attr = nAttr.createColor(
        "displayColor", "dspcol");
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(true));
    CHECK_MSTATUS(nAttr.setDefault(1.0f, 1.0f, 1.0f));

    // Add 'alpha' attribute.
    float alpha_min = 0.0f;
    float alpha_max = 1.0f;
    float alpha_default = 1.0f;
    m_display_alpha_attr = nAttr.create(
        "displayAlpha", "dspalpha",
        MFnNumericData::kFloat, alpha_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(alpha_min));
    CHECK_MSTATUS(nAttr.setMax(alpha_max));

    // Add 'saturation' attribute.
    float saturation_min = 0.0f;
    float saturation_soft_max = 2.0f;
    float saturation_default = 1.0f;
    m_display_saturation_attr = nAttr.create(
        "displaySaturation", "dspstrtn",
        MFnNumericData::kFloat, saturation_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(saturation_min));
    CHECK_MSTATUS(nAttr.setSoftMax(saturation_soft_max));

    // Add 'exposure' attribute.
    float exposure_soft_min = -9.0f;
    float exposure_soft_max = +9.0f;
    float exposure_default = 0.0f;
    m_display_exposure_attr = nAttr.create(
        "displayExposure", "dspexpsr",
        MFnNumericData::kFloat, exposure_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(exposure_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(exposure_soft_max));

    // Add 'gamma' attribute.
    float gamma_min = 0.0f;
    float gamma_soft_max = +2.0f;
    float gamma_default = 1.0f;
    m_display_gamma_attr = nAttr.create(
        "displayGamma", "dspgmma",
        MFnNumericData::kFloat, gamma_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(gamma_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gamma_soft_max));

    // Add 'soft clip' attribute.
    float soft_clip_min = 0.0f;
    float soft_clip_max = 1.0f;
    float soft_clip_default = 0.0f;
    m_display_soft_clip_attr = nAttr.create(
        "displaySoftClip", "dspsftclp",
        MFnNumericData::kFloat, soft_clip_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(soft_clip_min));
    CHECK_MSTATUS(nAttr.setMax(soft_clip_max));

    // Add 'Use Draw Depth' attribute
    bool use_draw_depth_default = false;
    m_display_use_draw_depth_attr = nAttr.create(
        "displayUseDrawDepth", "dspusdrwdpth",
        MFnNumericData::kBoolean, use_draw_depth_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));

    // Add 'depth' attribute.
    float draw_depth_min = 0.0f;
    float draw_depth_max = 100.0f;
    float draw_depth_default = 100.0f;
    m_display_draw_depth_attr = nAttr.create(
        "displayDrawDepth", "dspdrwdpth",
        MFnNumericData::kFloat, draw_depth_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(draw_depth_min));
    CHECK_MSTATUS(nAttr.setMax(draw_depth_max));

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

    // 3D LUT Edge Size (larger size means more accurate 3D LUT).
    uint32_t lut_edge_size_min = 8;
    uint32_t lut_edge_size_soft_max = 64;
    uint32_t lut_edge_size_max = 128;
    uint32_t lut_edge_size_default = 20;
    m_lut_edge_size_attr = nAttr.create(
        "lutEdgeSize", "ltedgsz",
        MFnNumericData::kInt, lut_edge_size_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));
    CHECK_MSTATUS(nAttr.setMin(lut_edge_size_min));
    CHECK_MSTATUS(nAttr.setMax(lut_edge_size_max));
    CHECK_MSTATUS(nAttr.setSoftMax(lut_edge_size_soft_max));

    // Color Space Name
    MFnStringData color_space_string_data;
    MObject color_space_string_data_obj = color_space_string_data.create("Linear");
    m_color_space_name_attr = tAttr.create(
            "colorSpaceName", "clspcnm",
            MFnData::kString, color_space_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(false));

    // Cache - Option
    m_cache_option_attr = eAttr.create(
        "cacheOption", "cchopt", kBakeOptionNothing);
    CHECK_MSTATUS(eAttr.addField("none", kBakeOptionNothing));
    CHECK_MSTATUS(eAttr.addField("colorSpace", kBakeOptionColorSpace));
    CHECK_MSTATUS(eAttr.addField("colorSpaceAndGrade", kBakeOptionColorSpaceAndGrade));
    CHECK_MSTATUS(eAttr.addField("all", kBakeOptionAll));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // Pixel Data Type
    m_cache_pixel_data_type_attr = eAttr.create(
        "cachePixelDataType", "cchpxldtyp",
        kDataTypeUnknown);
    CHECK_MSTATUS(eAttr.addField("auto", kDataTypeUnknown));
    CHECK_MSTATUS(eAttr.addField("uint8", kDataTypeUInt8));
    CHECK_MSTATUS(eAttr.addField("uint16", kDataTypeUInt16));
    CHECK_MSTATUS(eAttr.addField("half16", kDataTypeHalf16));
    CHECK_MSTATUS(eAttr.addField("float32", kDataTypeFloat32));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // Cache - Crop on Format
    bool cache_crop_on_format_default = false;
    m_cache_crop_on_format_attr = nAttr.create(
        "cacheCropOnFormat", "cchcpofmt",
        MFnNumericData::kBoolean, cache_crop_on_format_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(false));

    // Time
    m_time_attr = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0);
    CHECK_MSTATUS(uAttr.setStorable(true));

    // Create Common Attributes
    CHECK_MSTATUS(utils::create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(utils::create_output_stream_attribute(m_out_stream_attr));
    CHECK_MSTATUS(utils::create_node_disk_cache_attributes(
                      m_disk_cache_enable_attr,
                      m_disk_cache_file_path_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_camera_attr));
    //
    CHECK_MSTATUS(addAttribute(m_display_mode_attr));
    CHECK_MSTATUS(addAttribute(m_display_color_attr));
    CHECK_MSTATUS(addAttribute(m_display_alpha_attr));
    CHECK_MSTATUS(addAttribute(m_display_saturation_attr));
    CHECK_MSTATUS(addAttribute(m_display_exposure_attr));
    CHECK_MSTATUS(addAttribute(m_display_gamma_attr));
    CHECK_MSTATUS(addAttribute(m_display_soft_clip_attr));
    CHECK_MSTATUS(addAttribute(m_display_use_draw_depth_attr));
    CHECK_MSTATUS(addAttribute(m_display_draw_depth_attr));
    //
    CHECK_MSTATUS(addAttribute(m_card_depth_attr));
    CHECK_MSTATUS(addAttribute(m_card_size_x_attr));
    CHECK_MSTATUS(addAttribute(m_card_size_y_attr));
    CHECK_MSTATUS(addAttribute(m_card_res_x_attr));
    CHECK_MSTATUS(addAttribute(m_card_res_y_attr));
    //
    CHECK_MSTATUS(addAttribute(m_color_space_name_attr));
    CHECK_MSTATUS(addAttribute(m_lut_edge_size_attr));
    //
    CHECK_MSTATUS(addAttribute(m_cache_option_attr));
    CHECK_MSTATUS(addAttribute(m_cache_pixel_data_type_attr));
    CHECK_MSTATUS(addAttribute(m_cache_crop_on_format_attr));
    //
    CHECK_MSTATUS(addAttribute(m_disk_cache_enable_attr));
    CHECK_MSTATUS(addAttribute(m_disk_cache_file_path_attr));
    //
    CHECK_MSTATUS(addAttribute(m_time_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_time_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_mode_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_color_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_alpha_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_saturation_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_exposure_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_gamma_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_soft_clip_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_use_draw_depth_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_display_draw_depth_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_color_space_name_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_lut_edge_size_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_cache_option_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_cache_pixel_data_type_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_cache_crop_on_format_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_disk_cache_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_disk_cache_file_path_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace image_plane
} // namespace open_comp_graph_maya
