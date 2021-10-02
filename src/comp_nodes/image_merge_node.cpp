/*
 * Copyright (C) 2020, 2021 David Cattermole.
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
 * Merge 2 images together.
 */

// Maya
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
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
#include "image_merge_node.h"
#include "node_utils.h"
#include "attr_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

// Precompute index for enum.
const int32_t kMergeModeAdd = static_cast<int32_t>(ocg::MergeImageMode::kAdd);
const int32_t kMergeModeOver = static_cast<int32_t>(ocg::MergeImageMode::kOver);
const int32_t kMergeModeMultiply = static_cast<int32_t>(ocg::MergeImageMode::kMultiply);

MTypeId ImageMergeNode::m_id(OCGM_IMAGE_MERGE_TYPE_ID);

// Input Attributes
MObject ImageMergeNode::m_in_stream_a_attr;
MObject ImageMergeNode::m_in_stream_b_attr;
MObject ImageMergeNode::m_enable_attr;
MObject ImageMergeNode::m_merge_mode_attr;
MObject ImageMergeNode::m_mix_attr;

// Output Attributes
MObject ImageMergeNode::m_out_stream_attr;

ImageMergeNode::ImageMergeNode()
        : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageMergeNode::~ImageMergeNode() {}

MString ImageMergeNode::nodeName() {
    return MString(OCGM_IMAGE_MERGE_TYPE_NAME);
}

MStatus ImageMergeNode::updateOcgNodes(
    MDataBlock &data,
    std::shared_ptr<ocg::Graph> &shared_graph,
    std::vector<ocg::Node> input_ocg_nodes,
    ocg::Node &output_ocg_node)
{
    MStatus status = MS::kSuccess;
    auto log = log::get_logger();
    if (input_ocg_nodes.size() != 2) {
        return MS::kFailure;
    }
    bool exists = shared_graph->node_exists(m_ocg_node);
    if (!exists) {
        MString node_name = "merge";
        auto node_hash = utils::generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kMergeImage,
            node_hash);
    }

    auto input_ocg_node_a = input_ocg_nodes[0];
    uint8_t input_num_a = 0;
    status = utils::join_ocg_nodes(
        shared_graph,
        input_ocg_node_a,
        m_ocg_node,
        input_num_a);
    CHECK_MSTATUS(status);

    auto input_ocg_node_b = input_ocg_nodes[1];
    uint8_t input_num_b = 1;
    status = utils::join_ocg_nodes(
        shared_graph,
        input_ocg_node_b,
        m_ocg_node,
        input_num_b);
    CHECK_MSTATUS(status);

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        // Merge Mode Attribute
        int16_t merge_mode = utils::get_attr_value_short(data, m_merge_mode_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "mode", static_cast<int32_t>(merge_mode));

        // Mix Attribute
        float mix = utils::get_attr_value_float(data, m_mix_attr);
        shared_graph->set_node_attr_f32(m_ocg_node, "mix", mix);
    }

    return status;
}

MStatus ImageMergeNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_a_attr);
    in_attr_array.append(m_in_stream_b_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageMergeNode::creator() {
    return (new ImageMergeNode());
}

MStatus ImageMergeNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute   tAttr;
    MFnEnumAttribute    eAttr;

    // Merge Mode
    m_merge_mode_attr = eAttr.create("mode", "md", kMergeModeOver);
    CHECK_MSTATUS(eAttr.addField("add", kMergeModeAdd));
    CHECK_MSTATUS(eAttr.addField("over", kMergeModeOver));
    CHECK_MSTATUS(eAttr.addField("multiply", kMergeModeMultiply));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // Mix
    float mix_soft_min = 0.0f;
    float mix_soft_max = 1.0f;
    float mix_default = 1.0f;
    m_mix_attr = nAttr.create(
        "mix", "mx",
        MFnNumericData::kFloat, mix_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(mix_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(mix_soft_max));

    // Create Common Attributes
    CHECK_MSTATUS(utils::create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(utils::create_input_stream_attribute(m_in_stream_a_attr, "A"));
    CHECK_MSTATUS(utils::create_input_stream_attribute(m_in_stream_b_attr, "B"));
    CHECK_MSTATUS(utils::create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_merge_mode_attr));
    CHECK_MSTATUS(addAttribute(m_mix_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_a_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_b_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_merge_mode_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_mix_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_a_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_b_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
