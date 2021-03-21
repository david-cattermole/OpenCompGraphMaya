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
 *
 * TODO: Add 'pivot' point for the transform effect.
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
#include "node_utils.h"

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
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageTransformNode::~ImageTransformNode() {}

MString ImageTransformNode::nodeName() {
    return MString(OCGM_IMAGE_TRANSFORM_TYPE_NAME);
}

MStatus ImageTransformNode::updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node) {
    MStatus status = MS::kSuccess;
    if (input_ocg_nodes.size() != 1) {
        return MS::kFailure;
    }
    bool exists = shared_graph->node_exists(m_ocg_node);
    if (!exists) {
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kTransform,
            m_ocg_node_hash);
    }
    auto input_ocg_node = input_ocg_nodes[0];
    shared_graph->connect(input_ocg_node, m_ocg_node, 0);

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        // Translation attributes
        float tx = utils::get_attr_value_float(data, m_translate_x_attr);
        float ty = utils::get_attr_value_float(data, m_translate_y_attr);
        float r = utils::get_attr_value_float(data, m_rotate_attr);
        float rx = utils::get_attr_value_float(data, m_rotate_center_x_attr);
        float ry = utils::get_attr_value_float(data, m_rotate_center_y_attr);
        float scale_x = utils::get_attr_value_float(data, m_scale_x_attr);
        float scale_y = utils::get_attr_value_float(data, m_scale_y_attr);
        shared_graph->set_node_attr_f32(m_ocg_node, "translate_x", tx);
        shared_graph->set_node_attr_f32(m_ocg_node, "translate_y", ty);
        shared_graph->set_node_attr_f32(m_ocg_node, "rotate", r);
        shared_graph->set_node_attr_f32(m_ocg_node, "rotate_center_x", rx);
        shared_graph->set_node_attr_f32(m_ocg_node, "rotate_center_y", ry);
        shared_graph->set_node_attr_f32(m_ocg_node, "scale_x", scale_x);
        shared_graph->set_node_attr_f32(m_ocg_node, "scale_y", scale_y);
    }

    return status;
}

MStatus ImageTransformNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageTransformNode::creator() {
    return (new ImageTransformNode());
}

MStatus ImageTransformNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

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

    m_translate_y_attr = nAttr.create(
        "translateY", "ty",
        MFnNumericData::kFloat, translate_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(translate_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(translate_soft_max));

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

    m_rotate_center_y_attr = nAttr.create(
        "rotateCenterY", "ry",
        MFnNumericData::kFloat, center_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMax(center_soft_max));
    CHECK_MSTATUS(nAttr.setSoftMin(center_soft_min));

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

    m_scale_y_attr = nAttr.create(
            "scaleY", "scly",
            MFnNumericData::kFloat, scale_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(scale_min));
    CHECK_MSTATUS(nAttr.setSoftMax(scale_soft_max));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_translate_x_attr));
    CHECK_MSTATUS(addAttribute(m_translate_y_attr));
    CHECK_MSTATUS(addAttribute(m_rotate_attr));
    CHECK_MSTATUS(addAttribute(m_rotate_center_x_attr));
    CHECK_MSTATUS(addAttribute(m_rotate_center_y_attr));
    CHECK_MSTATUS(addAttribute(m_scale_x_attr));
    CHECK_MSTATUS(addAttribute(m_scale_y_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_translate_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_translate_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_rotate_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_rotate_center_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_rotate_center_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_scale_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_scale_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
