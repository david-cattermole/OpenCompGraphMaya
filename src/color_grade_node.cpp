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
#include <maya/MStreamUtils.h>

// STL
#include <cstring>
#include <cmath>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "graph_maya_data.h"
#include "color_grade_node.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ColorGradeNode::m_id(OCGM_COLOR_GRADE_TYPE_ID);

// Input Attributes
MObject ColorGradeNode::m_in_stream_attr;
MObject ColorGradeNode::m_enable_attr;
MObject ColorGradeNode::m_multiply_attr;

// Output Attributes
MObject ColorGradeNode::m_out_stream_attr;

ColorGradeNode::ColorGradeNode()
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)),
      m_ocg_node_hash(0) {}

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
        MDataHandle enable_handle = data.inputValue(
                ColorGradeNode::m_enable_attr, &status);
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
        if (input_stream_data == nullptr) {
            status = MS::kFailure;
            return status;
        }
        std::shared_ptr<ocg::Graph> shared_graph = input_stream_data->get_graph();
        auto input_ocg_node = input_stream_data->get_node();

        // Output Stream
        MDataHandle out_stream_handle = data.outputValue(m_out_stream_attr);
        GraphMayaData* new_data =
            static_cast<GraphMayaData*>(fn_plugin_data.data(&status));
        if (shared_graph) {
            // Modify the OCG Graph, and initialize the node values.
            bool exists = shared_graph->node_exists(m_ocg_node);
            MStreamUtils::stdErrorStream()
                    << "ColorGradeNode: node exists: " << exists << '\n';
            if (!exists) {
                m_ocg_node = shared_graph->create_node(
                    ocg::NodeType::kGrade,
                    m_ocg_node_hash);
            }
            shared_graph->connect(input_ocg_node, m_ocg_node, 0);
            MStreamUtils::stdErrorStream()
                    << "ColorGradeNode: input id: " << input_ocg_node.get_id()
                    << '\n';
            MStreamUtils::stdErrorStream()
                    << "ColorGradeNode: node  id: " << m_ocg_node.get_id()
                    << '\n';
            if (m_ocg_node.get_id() != 0) {
                shared_graph->set_node_attr_i32(
                    m_ocg_node, "enable", static_cast<int32_t>(enable));

                // Multiply Attribute
                MDataHandle multiply_handle = data.inputValue(ColorGradeNode::m_multiply_attr, &status);
                CHECK_MSTATUS_AND_RETURN_IT(status);
                float temp = multiply_handle.asFloat();
                MStreamUtils::stdErrorStream()
                        << "ColorGradeNode: multiply: " << static_cast<double>(temp) << '\n';
                shared_graph->set_node_attr_f32(m_ocg_node, "multiply", temp);
            }
        }
        std::cout << "Graph as string:\n"
                  << shared_graph->data_debug_string();
        new_data->set_node(m_ocg_node);
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

    // Multiply
    m_multiply_attr = nAttr.create(
        "multiply", "mul",
        MFnNumericData::kFloat, 1.0);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(addAttribute(m_multiply_attr));

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
    CHECK_MSTATUS(attributeAffects(m_multiply_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
