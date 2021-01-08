/*
 * Copyright (C) 2021 David Cattermole.
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
 * Apply a 2D Transform to an image (with matrix concatenation)
 */

// Maya
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MUuid.h>

// STL
#include <cstring>
#include <cmath>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "logger.h"
#include "graph_data.h"
#include "image_transform_node.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ImageTransformNode::m_id(OCGM_IMAGE_TRANSFORM_TYPE_ID);

// Input Attributes
MObject ImageTransformNode::m_in_stream_attr;
MObject ImageTransformNode::m_enable_attr;
MObject ImageTransformNode::m_translate_x_attr;
MObject ImageTransformNode::m_translate_y_attr;
MObject ImageTransformNode::m_rotate_attr;
MObject ImageTransformNode::m_rotate_center_x_attr;
MObject ImageTransformNode::m_rotate_center_y_attr;
MObject ImageTransformNode::m_scale_x_attr;
MObject ImageTransformNode::m_scale_y_attr;

// Output Attributes
MObject ImageTransformNode::m_out_stream_attr;

ImageTransformNode::ImageTransformNode()
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)),
      m_ocg_node_hash(0) {}

ImageTransformNode::~ImageTransformNode() {}

MString ImageTransformNode::nodeName() {
    return MString(OCGM_IMAGE_TRANSFORM_TYPE_NAME);
}

MStatus ImageTransformNode::compute(const MPlug &plug, MDataBlock &data) {
    auto log = log::get_logger();
    MStatus status = MS::kUnknownParameter;
    if (m_ocg_node_hash == 0) {
        // No OCG hash has been created yet, this node is not ready
        // to be computed.
        return status;
    }

    if (plug == m_out_stream_attr) {
        // Enable Attribute toggle
        MDataHandle enable_handle = data.inputValue(
                ImageTransformNode::m_enable_attr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        bool enable = enable_handle.asBool();

        // Create initial plug-in data structure. We don't need to
        // 'new' the data type directly.
        MFnPluginData fn_plugin_data;
        MTypeId data_type_id(OCGM_GRAPH_DATA_TYPE_ID);
        fn_plugin_data.create(data_type_id, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        // Get Input Stream
        MDataHandle in_stream_handle = data.inputValue(m_in_stream_attr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        GraphData* input_stream_data =
            static_cast<GraphData*>(in_stream_handle.asPluginData());
        if (input_stream_data == nullptr) {
            status = MS::kFailure;
            return status;
        }
        std::shared_ptr<ocg::Graph> shared_graph = input_stream_data->get_graph();
        auto input_ocg_node = input_stream_data->get_node();

        // Output Stream
        MDataHandle out_stream_handle = data.outputValue(m_out_stream_attr);
        GraphData* new_data =
            static_cast<GraphData*>(fn_plugin_data.data(&status));
        if (shared_graph) {
            // Modify the OCG Graph, and initialize the node values.
            bool exists = shared_graph->node_exists(m_ocg_node);
            log->debug("ImageTransformNode: node exists: {}", exists);
            if (!exists) {
                m_ocg_node = shared_graph->create_node(
                    ocg::NodeType::kTransform,
                    m_ocg_node_hash);
            }
            shared_graph->connect(input_ocg_node, m_ocg_node, 0);

            log->debug("ImageTransformNode: input id: {}", input_ocg_node.get_id());
            log->debug("ImageTransformNode: node id: {}", m_ocg_node.get_id());
            if (m_ocg_node.get_id() != 0) {
                shared_graph->set_node_attr_i32(
                    m_ocg_node, "enable", static_cast<int32_t>(enable));
                log->debug("ImageTransformNode: enable: {}", enable);

                MDataHandle translate_x_handle = data.inputValue(m_translate_x_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                MDataHandle translate_y_handle = data.inputValue(m_translate_y_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                MDataHandle rotate_handle = data.inputValue(m_rotate_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                MDataHandle rotate_center_x_handle = data.inputValue(m_rotate_center_x_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                MDataHandle rotate_center_y_handle = data.inputValue(m_rotate_center_y_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                MDataHandle scale_x_handle = data.inputValue(m_scale_x_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                MDataHandle scale_y_handle = data.inputValue(m_scale_y_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                float tx = translate_x_handle.asFloat();
                float ty = translate_y_handle.asFloat();
                float r = rotate_handle.asFloat();
                float rx = rotate_center_x_handle.asFloat();
                float ry = rotate_center_y_handle.asFloat();
                float scale_x = scale_x_handle.asFloat();
                float scale_y = scale_y_handle.asFloat();

                log->debug("ImageTransformNode: translate_xy: x={} y={}", tx, ty);
                log->debug("ImageTransformNode: rotate: r={} rx={} ry={}", r, rx, ry);
                log->debug("ImageTransformNode: scale_xy: x={} y={}", scale_x, scale_y);

                shared_graph->set_node_attr_f32(m_ocg_node, "translate_x", tx);
                shared_graph->set_node_attr_f32(m_ocg_node, "translate_y", ty);
                shared_graph->set_node_attr_f32(m_ocg_node, "rotate", r);
                shared_graph->set_node_attr_f32(m_ocg_node, "rotate_center_x", rx);
                shared_graph->set_node_attr_f32(m_ocg_node, "rotate_center_y", ry);
                shared_graph->set_node_attr_f32(m_ocg_node, "scale_x", scale_x);
                shared_graph->set_node_attr_f32(m_ocg_node, "scale_y", scale_y);
            }
        }
        log->debug(
            "ImageTransformNode: Graph as string:\n{}",
            shared_graph->data_debug_string());
        new_data->set_node(m_ocg_node);
        new_data->set_graph(shared_graph);
        out_stream_handle.setMPxData(new_data);
        out_stream_handle.setClean();
        status = MS::kSuccess;
    }

    return status;
}

void ImageTransformNode::postConstructor() {
    // Get the size
    MObject this_node = ImageTransformNode::thisMObject();

    // Get Node UUID
    MStatus status = MS::kSuccess;
    MFnDependencyNode fn_depend_node(this_node, &status);
    CHECK_MSTATUS(status)
    MUuid uuid = fn_depend_node.uuid();
    MString uuid_string = uuid.asString();
    const char *uuid_char = uuid_string.asChar();

    // Generate a 64-bit hash id from the 128-bit UUID.
    ImageTransformNode::m_ocg_node_hash =
        ocg::internal::generate_id_from_name(uuid_char);
};

void *ImageTransformNode::creator() {
    return (new ImageTransformNode());
}

MStatus ImageTransformNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;
    MTypeId stream_data_type_id(OCGM_GRAPH_DATA_TYPE_ID);

    // Create empty string data to be used as attribute default
    // (string) value.
    MFnStringData empty_string_data;
    MObject empty_string_data_obj = empty_string_data.create("");

    // In Stream
    m_in_stream_attr = tAttr.create(
            "inStream", "istm",
            stream_data_type_id);
    CHECK_MSTATUS(tAttr.setStorable(false));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setReadable(true));
    CHECK_MSTATUS(tAttr.setWritable(true));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));

    // Enable
    m_enable_attr = nAttr.create(
            "enable", "enb",
            MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(addAttribute(m_enable_attr));

    // Translate X and Y
    float translate_soft_min = -1.0f;
    float translate_soft_max = 1.0f;
    float translate_default = 0.0f;
    m_translate_x_attr = nAttr.create(
        "translateX", "tx",
        MFnNumericData::kFloat, translate_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(translate_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(translate_soft_max));
    CHECK_MSTATUS(addAttribute(m_translate_x_attr));

    m_translate_y_attr = nAttr.create(
        "translateY", "ty",
        MFnNumericData::kFloat, translate_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(translate_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(translate_soft_max));
    CHECK_MSTATUS(addAttribute(m_translate_y_attr));

    // Rotate
    float rotate_soft_min = -180.0f;
    float rotate_soft_max = 180.0f;
    float rotate_default = 0.0f;
    m_rotate_attr = nAttr.create(
        "rotate", "rt",
        MFnNumericData::kFloat, rotate_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMax(rotate_soft_max));
    CHECK_MSTATUS(nAttr.setSoftMin(rotate_soft_min));
    CHECK_MSTATUS(addAttribute(m_rotate_attr));

    // Rotate Center X and Y
    float center_soft_min = -1.0f;
    float center_soft_max = 1.0f;
    float center_default = 0.0f;
    m_rotate_center_x_attr = nAttr.create(
        "rotateCenterX", "rx",
        MFnNumericData::kFloat, center_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMax(center_soft_max));
    CHECK_MSTATUS(nAttr.setSoftMin(center_soft_min));
    CHECK_MSTATUS(addAttribute(m_rotate_center_x_attr));

    m_rotate_center_y_attr = nAttr.create(
        "rotateCenterY", "ry",
        MFnNumericData::kFloat, center_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMax(center_soft_max));
    CHECK_MSTATUS(nAttr.setSoftMin(center_soft_min));
    CHECK_MSTATUS(addAttribute(m_rotate_center_y_attr));

    // Scale X and Y
    float scale_min = 0.0f;
    float scale_soft_max = 10.0f;
    float scale_default = 1.0f;
    m_scale_x_attr = nAttr.create(
        "scaleX", "sx",
        MFnNumericData::kFloat, scale_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(scale_min));
    CHECK_MSTATUS(nAttr.setSoftMax(scale_soft_max));
    CHECK_MSTATUS(addAttribute(m_scale_x_attr));

    m_scale_y_attr = nAttr.create(
            "scaleY", "scly",
            MFnNumericData::kFloat, scale_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(scale_min));
    CHECK_MSTATUS(nAttr.setSoftMax(scale_soft_max));
    CHECK_MSTATUS(addAttribute(m_scale_y_attr));

    // Out Stream
    m_out_stream_attr = tAttr.create(
            "outStream", "ostm",
            stream_data_type_id);
    CHECK_MSTATUS(tAttr.setStorable(false));
    CHECK_MSTATUS(tAttr.setKeyable(false));
    CHECK_MSTATUS(tAttr.setReadable(true));
    CHECK_MSTATUS(tAttr.setWritable(false));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_translate_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_translate_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_rotate_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_rotate_center_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_rotate_center_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_scale_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_scale_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
