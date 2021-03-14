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
 * Perform lens distortion correction on the image stream.
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
#include "lens_distort_node.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId LensDistortNode::m_id(OCGM_LENS_DISTORT_TYPE_ID);

// Input Attributes
MObject LensDistortNode::m_in_stream_attr;
MObject LensDistortNode::m_enable_attr;
MObject LensDistortNode::m_k1_attr;
MObject LensDistortNode::m_k2_attr;
MObject LensDistortNode::m_center_x_attr;
MObject LensDistortNode::m_center_y_attr;

// Output Attributes
MObject LensDistortNode::m_out_stream_attr;

LensDistortNode::LensDistortNode()
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

LensDistortNode::~LensDistortNode() {}

MString LensDistortNode::nodeName() {
    return MString(OCGM_LENS_DISTORT_TYPE_NAME);
}

MStatus LensDistortNode::updateOcgNodes(
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
            ocg::NodeType::kLensDistort,
            m_ocg_node_hash);
    }
    auto input_ocg_node = input_ocg_nodes[0];
    shared_graph->connect(input_ocg_node, m_ocg_node, 0);

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        MDataHandle enable_handle = data.inputValue(
            m_enable_attr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        bool enable = enable_handle.asBool();
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        // Multiply Attribute RGBA
        MDataHandle k1_handle = data.inputValue(m_k1_attr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDataHandle k2_handle = data.inputValue(m_k2_attr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDataHandle center_x_handle = data.inputValue(m_center_x_attr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDataHandle center_y_handle = data.inputValue(m_center_y_attr, &status);

        float temp_r = k1_handle.asFloat();
        float temp_g = k2_handle.asFloat();
        float temp_b = center_x_handle.asFloat();
        float temp_a = center_y_handle.asFloat();

        shared_graph->set_node_attr_f32(m_ocg_node, "k1", temp_r);
        shared_graph->set_node_attr_f32(m_ocg_node, "k2", temp_g);
        shared_graph->set_node_attr_f32(m_ocg_node, "center_x", temp_b);
        shared_graph->set_node_attr_f32(m_ocg_node, "center_y", temp_a);
    }

    return status;
}

MStatus LensDistortNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *LensDistortNode::creator() {
    return (new LensDistortNode());
}

MStatus LensDistortNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Distortion Parameters
    float value_soft_min = -0.1f;
    float value_soft_max = 0.1f;
    float value_default = 0.0f;
    m_k1_attr = nAttr.create(
        "k1", "k1",
        MFnNumericData::kFloat, value_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(value_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(value_soft_max));

    m_k2_attr = nAttr.create(
        "k2", "k2",
        MFnNumericData::kFloat, value_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(value_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(value_soft_max));

    m_center_x_attr = nAttr.create(
        "centerX", "cx",
        MFnNumericData::kFloat, value_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(value_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(value_soft_max));

    m_center_y_attr = nAttr.create(
        "centerY", "cy",
        MFnNumericData::kFloat, value_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(value_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(value_soft_max));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_k1_attr));
    CHECK_MSTATUS(addAttribute(m_k2_attr));
    CHECK_MSTATUS(addAttribute(m_center_x_attr));
    CHECK_MSTATUS(addAttribute(m_center_y_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_k1_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_k2_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_center_x_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_center_y_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
