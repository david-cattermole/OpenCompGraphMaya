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
 * Apply a 2D Crop to an image.
 *
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
#include "image_crop_node.h"
#include "node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ImageCropNode::m_id(OCGM_IMAGE_CROP_TYPE_ID);

// Input Attributes
MObject ImageCropNode::m_in_stream_attr;
MObject ImageCropNode::m_enable_attr;
MObject ImageCropNode::m_window_min_x_attr;
MObject ImageCropNode::m_window_min_y_attr;
MObject ImageCropNode::m_window_max_x_attr;
MObject ImageCropNode::m_window_max_y_attr;
MObject ImageCropNode::m_reformat_attr;
MObject ImageCropNode::m_black_outside_attr;
MObject ImageCropNode::m_intersect_attr;

// Output Attributes
MObject ImageCropNode::m_out_stream_attr;

ImageCropNode::ImageCropNode()
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageCropNode::~ImageCropNode() {}

MString ImageCropNode::nodeName() {
    return MString(OCGM_IMAGE_CROP_TYPE_NAME);
}

MStatus ImageCropNode::updateOcgNodes(
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
        MString node_name = "crop";
        auto node_hash = utils::generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kCropImage,
            node_hash);
    }
    auto input_ocg_node = input_ocg_nodes[0];

    uint8_t input_num = 0;
    status = utils::join_ocg_nodes(
        shared_graph,
        input_ocg_node,
        m_ocg_node,
        input_num);
    CHECK_MSTATUS(status);

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        bool reformat = utils::get_attr_value_bool(data, m_reformat_attr);
        bool black_outside = utils::get_attr_value_bool(data, m_black_outside_attr);
        bool intersect = utils::get_attr_value_bool(data, m_intersect_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "reformat", static_cast<int32_t>(reformat));
        shared_graph->set_node_attr_i32(
            m_ocg_node, "black_outside", static_cast<int32_t>(black_outside));
        shared_graph->set_node_attr_i32(
            m_ocg_node, "intersect", static_cast<int32_t>(intersect));

        // Translation attributes
        float window_min_x = utils::get_attr_value_int(data, m_window_min_x_attr);
        float window_min_y = utils::get_attr_value_int(data, m_window_min_y_attr);
        float window_max_x = utils::get_attr_value_int(data, m_window_max_x_attr);
        float window_max_y = utils::get_attr_value_int(data, m_window_max_y_attr);

        shared_graph->set_node_attr_i32(m_ocg_node, "window_min_x", window_min_x);
        shared_graph->set_node_attr_i32(m_ocg_node, "window_min_y", window_min_y);
        shared_graph->set_node_attr_i32(m_ocg_node, "window_max_x", window_max_x);
        shared_graph->set_node_attr_i32(m_ocg_node, "window_max_y", window_max_y);

        shared_graph->set_node_attr_i32(m_ocg_node, "reformat", reformat);
        shared_graph->set_node_attr_i32(m_ocg_node, "black_outside", black_outside);
        shared_graph->set_node_attr_i32(m_ocg_node, "intersect", intersect);
    }

    return status;
}

MStatus ImageCropNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageCropNode::creator() {
    return (new ImageCropNode());
}

MStatus ImageCropNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Window X and Y
    int window_soft_min = 0;
    int window_soft_max = 4096;
    int window_default_a = 0;
    int window_default_b = 100;
    m_window_min_x_attr = nAttr.create(
        "windowMinX", "wnminx",
        MFnNumericData::kInt, window_default_a);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(window_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(window_soft_max));

    m_window_min_y_attr = nAttr.create(
        "windowMinY", "wnminy",
        MFnNumericData::kInt, window_default_a);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(window_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(window_soft_max));

    m_window_max_x_attr = nAttr.create(
        "windowMaxX", "wnmaxx",
        MFnNumericData::kInt, window_default_b);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(window_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(window_soft_max));

    m_window_max_y_attr = nAttr.create(
        "windowMaxY", "wnmaxy",
        MFnNumericData::kInt, window_default_b);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(window_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(window_soft_max));

    // 'reformat' toggle
    m_reformat_attr = nAttr.create(
        "reformat", "rfmt",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // 'black_outside' toggle
    m_black_outside_attr = nAttr.create(
        "blackOutside", "blkosd",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // 'intersect' toggle
    m_intersect_attr = nAttr.create(
        "intersect", "intst",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_window_min_x_attr));
    CHECK_MSTATUS(addAttribute(m_window_min_y_attr));
    CHECK_MSTATUS(addAttribute(m_window_max_x_attr));
    CHECK_MSTATUS(addAttribute(m_window_max_y_attr));
    CHECK_MSTATUS(addAttribute(m_reformat_attr));
    CHECK_MSTATUS(addAttribute(m_black_outside_attr));
    CHECK_MSTATUS(addAttribute(m_intersect_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_window_min_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_window_min_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_window_max_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_window_max_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_reformat_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_black_outside_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_intersect_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
