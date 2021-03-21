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
 * Adjust the grade of the RGB linear colors.
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
#include "color_grade_node.h"
#include "node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ColorGradeNode::m_id(OCGM_COLOR_GRADE_TYPE_ID);

// Input Attributes
MObject ColorGradeNode::m_in_stream_attr;
MObject ColorGradeNode::m_enable_attr;
MObject ColorGradeNode::m_multiply_r_attr;
MObject ColorGradeNode::m_multiply_g_attr;
MObject ColorGradeNode::m_multiply_b_attr;
MObject ColorGradeNode::m_multiply_a_attr;

// Output Attributes
MObject ColorGradeNode::m_out_stream_attr;

ColorGradeNode::ColorGradeNode()
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ColorGradeNode::~ColorGradeNode() {}

MString ColorGradeNode::nodeName() {
    return MString(OCGM_COLOR_GRADE_TYPE_NAME);
}

MStatus ColorGradeNode::updateOcgNodes(
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
            ocg::NodeType::kGrade,
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

        // Multiply Attribute RGBA
        float temp_r = utils::get_attr_value_float(data, m_multiply_r_attr);
        float temp_g = utils::get_attr_value_float(data, m_multiply_g_attr);
        float temp_b = utils::get_attr_value_float(data, m_multiply_b_attr);
        float temp_a = utils::get_attr_value_float(data, m_multiply_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_node, "multiply_r", temp_r);
        shared_graph->set_node_attr_f32(m_ocg_node, "multiply_g", temp_g);
        shared_graph->set_node_attr_f32(m_ocg_node, "multiply_b", temp_b);
        shared_graph->set_node_attr_f32(m_ocg_node, "multiply_a", temp_a);
    }

    return status;
}

MStatus ColorGradeNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ColorGradeNode::creator() {
    return (new ColorGradeNode());
}

MStatus ColorGradeNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Multiply RGBA
    float multiply_min = 0.0f;
    float multiply_soft_max = 10.0f;
    float multiply_default = 1.0f;
    m_multiply_r_attr = nAttr.create(
        "multiplyR", "mulr",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(multiply_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    m_multiply_g_attr = nAttr.create(
        "multiplyG", "mulg",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(multiply_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    m_multiply_b_attr = nAttr.create(
        "multiplyB", "mulb",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(multiply_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    m_multiply_a_attr = nAttr.create(
        "multiplyA", "mula",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(multiply_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_r_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_g_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_b_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_a_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_a_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
