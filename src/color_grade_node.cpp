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
#include <maya/MFnCompoundAttribute.h>
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

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "graph_maya_data.h"
#include "color_grade_node.h"

// OCG
#include "opencompgraph.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ColorGradeNode::m_id(OCGM_COLOR_GRADE_TYPE_ID);

// Input Attributes
MObject ColorGradeNode::m_in_stream_attr;
MObject ColorGradeNode::m_enable_attr;
MObject ColorGradeNode::m_file_path_attr;
MObject ColorGradeNode::m_k1_attr;
MObject ColorGradeNode::m_k2_attr;

// Output Attributes
MObject ColorGradeNode::m_out_stream_attr;

ColorGradeNode::ColorGradeNode() : m_ocg_node_hash(0),
                                   m_ocg_node_id(0) {}

ColorGradeNode::~ColorGradeNode() {}

MString ColorGradeNode::nodeName() {
    return MString("ocgColorGrade");
}

MStatus ColorGradeNode::compute(const MPlug &plug, MDataBlock &data) {
    MStatus status = MS::kUnknownParameter;
    if (m_ocg_node_hash == 0) {
        // No OCG hash has been created yet, this node is not ready
        // to be computed.
        return status;
    }

    if (plug == m_out_stream_attr) {
        // Enable Attribute toggle
        MDataHandle enable_handle = data.inputValue(m_enable_attr, &status);
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
        GraphMayaData* input_stream_data =
            static_cast<GraphMayaData*>(in_stream_handle.asPluginData());
        std::shared_ptr<ocg::Graph> shared_graph;
        if (input_stream_data != nullptr) {
            shared_graph = input_stream_data->get_graph();
        }

        // Output Stream
        MDataHandle out_stream_handle = data.outputValue(m_out_stream_attr);
        GraphMayaData* new_data =
            static_cast<GraphMayaData*>(fn_plugin_data.data(&status));
        if (enable) {
            // Modify the OCG Graph, and initialize the node values.
            bool exists = shared_graph->node_with_hash_exists(m_ocg_node_hash);
            if (!exists) {
                auto new_grade_node = ocg::Node(
                    ocg::NodeType::kGrade,
                    m_ocg_node_hash);
                m_ocg_node_id = shared_graph->add_node(new_grade_node);
            }
            auto grade_node = shared_graph->node_with_hash(m_ocg_node_hash);
            if (grade_node) {
                grade_node->set_attr_i32("enable", static_cast<int32_t>(enable));

                // K1 Attribute
                MDataHandle k1_handle = data.inputValue(m_k1_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                float k1 = k1_handle.asFloat();
                grade_node->set_attr_f32("multiply", k1);

                // K2 Attribute
                MDataHandle k2_handle = data.inputValue(m_k2_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                float k2 = k2_handle.asFloat();
                grade_node->set_attr_f32("k2", k2);
            }
        }
        new_data->set_graph(shared_graph);
        out_stream_handle.setMPxData(new_data);
        out_stream_handle.setClean();
        status = MS::kSuccess;
    }

    return status;
}

void ColorGradeNode::postConstructor() {
    // Get the size
    MObject this_node = ColorGradeNode::thisMObject();

    // Get Node UUID
    MStatus status = MS::kSuccess;
    MFnDependencyNode fn_depend_node(this_node, &status);
    CHECK_MSTATUS(status)
    MUuid uuid = fn_depend_node.uuid();
    MString uuid_string = uuid.asString();
    const char *uuid_char = uuid_string.asChar();

    // Generate a 64-bit hash id from the 128-bit UUID.
    ColorGradeNode::m_ocg_node_hash =
        ocg::internal::generate_id_from_name(uuid_char);
};

void *ColorGradeNode::creator() {
    return (new ColorGradeNode());
}

MStatus ColorGradeNode::initialize() {
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

    // K1
    m_k1_attr = nAttr.create(
        "k1", "k1",
        MFnNumericData::kDouble, 0.0);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(addAttribute(m_k1_attr));

    // K2
    m_k2_attr = nAttr.create(
        "k2", "k2",
        MFnNumericData::kDouble, 0.0);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(addAttribute(m_k2_attr));

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
    CHECK_MSTATUS(attributeAffects(m_file_path_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_k1_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_k2_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
