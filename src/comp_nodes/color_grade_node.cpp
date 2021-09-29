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

MObject ColorGradeNode::m_process_r_attr;
MObject ColorGradeNode::m_process_g_attr;
MObject ColorGradeNode::m_process_b_attr;
MObject ColorGradeNode::m_process_a_attr;

MObject ColorGradeNode::m_black_point_r_attr;
MObject ColorGradeNode::m_black_point_g_attr;
MObject ColorGradeNode::m_black_point_b_attr;
MObject ColorGradeNode::m_black_point_a_attr;

MObject ColorGradeNode::m_white_point_r_attr;
MObject ColorGradeNode::m_white_point_g_attr;
MObject ColorGradeNode::m_white_point_b_attr;
MObject ColorGradeNode::m_white_point_a_attr;

MObject ColorGradeNode::m_lift_r_attr;
MObject ColorGradeNode::m_lift_g_attr;
MObject ColorGradeNode::m_lift_b_attr;
MObject ColorGradeNode::m_lift_a_attr;

MObject ColorGradeNode::m_gain_r_attr;
MObject ColorGradeNode::m_gain_g_attr;
MObject ColorGradeNode::m_gain_b_attr;
MObject ColorGradeNode::m_gain_a_attr;

MObject ColorGradeNode::m_multiply_r_attr;
MObject ColorGradeNode::m_multiply_g_attr;
MObject ColorGradeNode::m_multiply_b_attr;
MObject ColorGradeNode::m_multiply_a_attr;

MObject ColorGradeNode::m_offset_r_attr;
MObject ColorGradeNode::m_offset_g_attr;
MObject ColorGradeNode::m_offset_b_attr;
MObject ColorGradeNode::m_offset_a_attr;

MObject ColorGradeNode::m_gamma_r_attr;
MObject ColorGradeNode::m_gamma_g_attr;
MObject ColorGradeNode::m_gamma_b_attr;
MObject ColorGradeNode::m_gamma_a_attr;

MObject ColorGradeNode::m_reverse_attr;
MObject ColorGradeNode::m_clamp_black_attr;
MObject ColorGradeNode::m_clamp_white_attr;
MObject ColorGradeNode::m_premult_attr;
MObject ColorGradeNode::m_mix_attr;


// Output Attributes
MObject ColorGradeNode::m_out_stream_attr;

ColorGradeNode::ColorGradeNode()
    : m_ocg_grade_node(ocg::Node(ocg::NodeType::kNull, 0)) {
}

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

    bool grade_exists = shared_graph->node_exists(m_ocg_grade_node);
    if (!grade_exists) {
        MString node_name = "grade";
        auto grade_node_hash = utils::generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_grade_node =
            shared_graph->create_node(
                ocg::NodeType::kGrade,
                grade_node_hash);
    }
    auto input_ocg_node = input_ocg_nodes[0];
    uint8_t input_num = 0;
    status = utils::join_ocg_nodes(
        shared_graph,
        input_ocg_node,
        m_ocg_grade_node,
        input_num);
    CHECK_MSTATUS(status);

    if (m_ocg_grade_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_grade_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "enable", static_cast<int32_t>(enable));

        // Process Attribute RGBA
        bool process_r = utils::get_attr_value_bool(data, m_process_r_attr);
        bool process_g = utils::get_attr_value_bool(data, m_process_g_attr);
        bool process_b = utils::get_attr_value_bool(data, m_process_b_attr);
        bool process_a = utils::get_attr_value_bool(data, m_process_a_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "process_r", static_cast<int32_t>(process_r));
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "process_g", static_cast<int32_t>(process_g));
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "process_b", static_cast<int32_t>(process_b));
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "process_a", static_cast<int32_t>(process_a));

        // Black Point Attribute RGBA
        float black_point_r = utils::get_attr_value_float(data, m_black_point_r_attr);
        float black_point_g = utils::get_attr_value_float(data, m_black_point_g_attr);
        float black_point_b = utils::get_attr_value_float(data, m_black_point_b_attr);
        float black_point_a = utils::get_attr_value_float(data, m_black_point_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "black_point_r", black_point_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "black_point_g", black_point_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "black_point_b", black_point_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "black_point_a", black_point_a);

        // White Point Attribute RGBA
        float white_point_r = utils::get_attr_value_float(data, m_white_point_r_attr);
        float white_point_g = utils::get_attr_value_float(data, m_white_point_g_attr);
        float white_point_b = utils::get_attr_value_float(data, m_white_point_b_attr);
        float white_point_a = utils::get_attr_value_float(data, m_white_point_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "white_point_r", white_point_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "white_point_g", white_point_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "white_point_b", white_point_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "white_point_a", white_point_a);

        // Lift Attribute RGBA
        float lift_r = utils::get_attr_value_float(data, m_lift_r_attr);
        float lift_g = utils::get_attr_value_float(data, m_lift_g_attr);
        float lift_b = utils::get_attr_value_float(data, m_lift_b_attr);
        float lift_a = utils::get_attr_value_float(data, m_lift_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "lift_r", lift_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "lift_g", lift_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "lift_b", lift_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "lift_a", lift_a);

        // Gain Attribute RGBA
        float gain_r = utils::get_attr_value_float(data, m_gain_r_attr);
        float gain_g = utils::get_attr_value_float(data, m_gain_g_attr);
        float gain_b = utils::get_attr_value_float(data, m_gain_b_attr);
        float gain_a = utils::get_attr_value_float(data, m_gain_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gain_r", gain_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gain_g", gain_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gain_b", gain_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gain_a", gain_a);

        // Multiply Attribute RGBA
        float multiply_r = utils::get_attr_value_float(data, m_multiply_r_attr);
        float multiply_g = utils::get_attr_value_float(data, m_multiply_g_attr);
        float multiply_b = utils::get_attr_value_float(data, m_multiply_b_attr);
        float multiply_a = utils::get_attr_value_float(data, m_multiply_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "multiply_r", multiply_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "multiply_g", multiply_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "multiply_b", multiply_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "multiply_a", multiply_a);

        // Offset Attribute RGBA
        float offset_r = utils::get_attr_value_float(data, m_offset_r_attr);
        float offset_g = utils::get_attr_value_float(data, m_offset_g_attr);
        float offset_b = utils::get_attr_value_float(data, m_offset_b_attr);
        float offset_a = utils::get_attr_value_float(data, m_offset_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "offset_r", offset_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "offset_g", offset_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "offset_b", offset_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "offset_a", offset_a);

        // Gamma Attribute RGBA
        float gamma_r = utils::get_attr_value_float(data, m_gamma_r_attr);
        float gamma_g = utils::get_attr_value_float(data, m_gamma_g_attr);
        float gamma_b = utils::get_attr_value_float(data, m_gamma_b_attr);
        float gamma_a = utils::get_attr_value_float(data, m_gamma_a_attr);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gamma_r", gamma_r);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gamma_g", gamma_g);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gamma_b", gamma_b);
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "gamma_a", gamma_a);

        // Misc Attributes
        bool reverse = utils::get_attr_value_bool(data, m_reverse_attr);
        bool clamp_black = utils::get_attr_value_bool(data, m_clamp_black_attr);
        bool clamp_white = utils::get_attr_value_bool(data, m_clamp_white_attr);
        bool premult = utils::get_attr_value_bool(data, m_premult_attr);
        float mix = utils::get_attr_value_float(data, m_mix_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "reverse", static_cast<int32_t>(reverse));
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "clamp_black", static_cast<int32_t>(clamp_black));
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "clamp_white", static_cast<int32_t>(clamp_white));
        shared_graph->set_node_attr_i32(
            m_ocg_grade_node, "premult", static_cast<int32_t>(premult));
        shared_graph->set_node_attr_f32(m_ocg_grade_node, "mix", mix);
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

    // Process RGBA
    //
    // By default, only affect RGB, do not affect Alpha.
    m_process_r_attr = nAttr.create(
        "processR", "prcr",
        MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_process_g_attr = nAttr.create(
        "processG", "prcg",
        MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_process_b_attr = nAttr.create(
        "processB", "prcb",
        MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_process_a_attr = nAttr.create(
        "processA", "prca",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // Black Point RGBA
    float black_point_soft_min = 0.0f;
    float black_point_soft_max = 1.0f;
    float black_point_default = 0.0f;
    m_black_point_r_attr = nAttr.create(
        "blackPointR", "bkptr",
        MFnNumericData::kFloat, black_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(black_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(black_point_soft_max));

    m_black_point_g_attr = nAttr.create(
        "blackPointG", "bkptg",
        MFnNumericData::kFloat, black_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(black_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(black_point_soft_max));

    m_black_point_b_attr = nAttr.create(
        "blackPointB", "bkptb",
        MFnNumericData::kFloat, black_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(black_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(black_point_soft_max));

    m_black_point_a_attr = nAttr.create(
        "blackPointA", "bkpta",
        MFnNumericData::kFloat, black_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(black_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(black_point_soft_max));

    // White Point RGBA
    float white_point_soft_min = 0.0f;
    float white_point_soft_max = 1.0f;
    float white_point_default = 1.0f;
    m_white_point_r_attr = nAttr.create(
        "whitePointR", "wtptr",
        MFnNumericData::kFloat, white_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(white_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(white_point_soft_max));

    m_white_point_g_attr = nAttr.create(
        "whitePointG", "wtptg",
        MFnNumericData::kFloat, white_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(white_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(white_point_soft_max));

    m_white_point_b_attr = nAttr.create(
        "whitePointB", "wtptb",
        MFnNumericData::kFloat, white_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(white_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(white_point_soft_max));

    m_white_point_a_attr = nAttr.create(
        "whitePointA", "wtpta",
        MFnNumericData::kFloat, white_point_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(white_point_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(white_point_soft_max));

    // Lift RGBA
    float lift_soft_min = 0.0f;
    float lift_soft_max = 1.0f;
    float lift_default = 0.0f;
    m_lift_r_attr = nAttr.create(
        "liftR", "lftr",
        MFnNumericData::kFloat, lift_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(lift_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(lift_soft_max));

    m_lift_g_attr = nAttr.create(
        "liftG", "lftg",
        MFnNumericData::kFloat, lift_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(lift_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(lift_soft_max));

    m_lift_b_attr = nAttr.create(
        "liftB", "lftb",
        MFnNumericData::kFloat, lift_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(lift_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(lift_soft_max));

    m_lift_a_attr = nAttr.create(
        "liftA", "lfta",
        MFnNumericData::kFloat, lift_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(lift_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(lift_soft_max));

    // Gain RGBA
    float gain_soft_min = 0.0f;
    float gain_soft_max = 1.0f;
    float gain_default = 1.0f;
    m_gain_r_attr = nAttr.create(
        "gainR", "ginr",
        MFnNumericData::kFloat, gain_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(gain_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gain_soft_max));

    m_gain_g_attr = nAttr.create(
        "gainG", "ging",
        MFnNumericData::kFloat, gain_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(gain_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gain_soft_max));

    m_gain_b_attr = nAttr.create(
        "gainB", "ginb",
        MFnNumericData::kFloat, gain_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(gain_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gain_soft_max));

    m_gain_a_attr = nAttr.create(
        "gainA", "gina",
        MFnNumericData::kFloat, gain_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(gain_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gain_soft_max));

    // Multiply RGBA
    float multiply_soft_min = 0.0f;
    float multiply_soft_max = 1.0f;
    float multiply_default = 1.0f;
    m_multiply_r_attr = nAttr.create(
        "multiplyR", "mulr",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(multiply_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    m_multiply_g_attr = nAttr.create(
        "multiplyG", "mulg",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(multiply_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    m_multiply_b_attr = nAttr.create(
        "multiplyB", "mulb",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(multiply_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    m_multiply_a_attr = nAttr.create(
        "multiplyA", "mula",
        MFnNumericData::kFloat, multiply_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(multiply_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(multiply_soft_max));

    // Offset RGBA
    float offset_soft_min = 0.0f;
    float offset_soft_max = 1.0f;
    float offset_default = 0.0f;
    m_offset_r_attr = nAttr.create(
        "offsetR", "ofsr",
        MFnNumericData::kFloat, offset_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(offset_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(offset_soft_max));

    m_offset_g_attr = nAttr.create(
        "offsetG", "ofsg",
        MFnNumericData::kFloat, offset_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(offset_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(offset_soft_max));

    m_offset_b_attr = nAttr.create(
        "offsetB", "ofsb",
        MFnNumericData::kFloat, offset_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(offset_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(offset_soft_max));

    m_offset_a_attr = nAttr.create(
        "offsetA", "ofsa",
        MFnNumericData::kFloat, offset_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(offset_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(offset_soft_max));

    // Gamma RGBA
    float gamma_min = 0.0f;
    float gamma_soft_min = 0.0f;
    float gamma_soft_max = 2.0f;
    float gamma_default = 1.0f;
    m_gamma_r_attr = nAttr.create(
        "gammaR", "gamr",
        MFnNumericData::kFloat, gamma_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(gamma_min));
    CHECK_MSTATUS(nAttr.setSoftMin(gamma_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gamma_soft_max));

    m_gamma_g_attr = nAttr.create(
        "gammaG", "gamg",
        MFnNumericData::kFloat, gamma_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(gamma_min));
    CHECK_MSTATUS(nAttr.setSoftMin(gamma_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gamma_soft_max));

    m_gamma_b_attr = nAttr.create(
        "gammaB", "gamb",
        MFnNumericData::kFloat, gamma_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(gamma_min));
    CHECK_MSTATUS(nAttr.setSoftMin(gamma_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gamma_soft_max));

    m_gamma_a_attr = nAttr.create(
        "gammaA", "gama",
        MFnNumericData::kFloat, gamma_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(gamma_min));
    CHECK_MSTATUS(nAttr.setSoftMin(gamma_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(gamma_soft_max));

    // Misc Attributes
    m_reverse_attr = nAttr.create(
        "reverse", "rev",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_clamp_black_attr = nAttr.create(
        "clampBlack", "clpbk",
        MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_clamp_white_attr = nAttr.create(
        "clampWhite", "clpwt",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_premult_attr = nAttr.create(
        "premult", "premt",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    m_mix_attr = nAttr.create(
        "mix", "mix",
        MFnNumericData::kFloat, 1.0);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(0.0));
    CHECK_MSTATUS(nAttr.setMax(1.0));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));

    CHECK_MSTATUS(addAttribute(m_process_r_attr));
    CHECK_MSTATUS(addAttribute(m_process_g_attr));
    CHECK_MSTATUS(addAttribute(m_process_b_attr));
    CHECK_MSTATUS(addAttribute(m_process_a_attr));

    CHECK_MSTATUS(addAttribute(m_black_point_r_attr));
    CHECK_MSTATUS(addAttribute(m_black_point_g_attr));
    CHECK_MSTATUS(addAttribute(m_black_point_b_attr));
    CHECK_MSTATUS(addAttribute(m_black_point_a_attr));

    CHECK_MSTATUS(addAttribute(m_white_point_r_attr));
    CHECK_MSTATUS(addAttribute(m_white_point_g_attr));
    CHECK_MSTATUS(addAttribute(m_white_point_b_attr));
    CHECK_MSTATUS(addAttribute(m_white_point_a_attr));

    CHECK_MSTATUS(addAttribute(m_lift_r_attr));
    CHECK_MSTATUS(addAttribute(m_lift_g_attr));
    CHECK_MSTATUS(addAttribute(m_lift_b_attr));
    CHECK_MSTATUS(addAttribute(m_lift_a_attr));

    CHECK_MSTATUS(addAttribute(m_gain_r_attr));
    CHECK_MSTATUS(addAttribute(m_gain_g_attr));
    CHECK_MSTATUS(addAttribute(m_gain_b_attr));
    CHECK_MSTATUS(addAttribute(m_gain_a_attr));

    CHECK_MSTATUS(addAttribute(m_multiply_r_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_g_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_b_attr));
    CHECK_MSTATUS(addAttribute(m_multiply_a_attr));

    CHECK_MSTATUS(addAttribute(m_offset_r_attr));
    CHECK_MSTATUS(addAttribute(m_offset_g_attr));
    CHECK_MSTATUS(addAttribute(m_offset_b_attr));
    CHECK_MSTATUS(addAttribute(m_offset_a_attr));

    CHECK_MSTATUS(addAttribute(m_gamma_r_attr));
    CHECK_MSTATUS(addAttribute(m_gamma_g_attr));
    CHECK_MSTATUS(addAttribute(m_gamma_b_attr));
    CHECK_MSTATUS(addAttribute(m_gamma_a_attr));

    CHECK_MSTATUS(addAttribute(m_reverse_attr));
    CHECK_MSTATUS(addAttribute(m_clamp_black_attr));
    CHECK_MSTATUS(addAttribute(m_clamp_white_attr));
    CHECK_MSTATUS(addAttribute(m_premult_attr));
    CHECK_MSTATUS(addAttribute(m_mix_attr));

    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_process_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_process_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_process_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_process_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_black_point_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_black_point_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_black_point_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_black_point_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_white_point_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_white_point_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_white_point_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_white_point_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_lift_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_lift_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_lift_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_lift_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_gain_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_gain_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_gain_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_gain_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_multiply_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_multiply_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_offset_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_offset_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_offset_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_offset_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_gamma_r_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_gamma_g_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_gamma_b_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_gamma_a_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_reverse_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_clamp_black_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_clamp_white_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_premult_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_mix_attr, m_out_stream_attr));

    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
