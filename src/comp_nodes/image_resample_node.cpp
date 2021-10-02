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
 * Apply Resample to an image, to up/down-res an image, very quickly,
 * albeit with bad quality.
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
#include "image_resample_node.h"
#include "node_utils.h"
#include "attr_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ImageResampleNode::m_id(OCGM_IMAGE_RESAMPLE_TYPE_ID);

// Input Attributes
MObject ImageResampleNode::m_in_stream_attr;
MObject ImageResampleNode::m_enable_attr;
MObject ImageResampleNode::m_factor_attr;
MObject ImageResampleNode::m_interpolate_attr;

// Output Attributes
MObject ImageResampleNode::m_out_stream_attr;

ImageResampleNode::ImageResampleNode()
    : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageResampleNode::~ImageResampleNode() {}

MString ImageResampleNode::nodeName() {
    return MString(OCGM_IMAGE_RESAMPLE_TYPE_NAME);
}

MStatus ImageResampleNode::updateOcgNodes(
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
        MString node_name = "resample";
        auto node_hash = utils::generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kResampleImage,
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

        int factor = utils::get_attr_value_int(data, m_factor_attr);
        shared_graph->set_node_attr_i32(m_ocg_node, "factor", factor);

        bool interpolate = utils::get_attr_value_bool(data, m_interpolate_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "interpolate", static_cast<int32_t>(interpolate));
    }

    return status;
}

MStatus ImageResampleNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageResampleNode::creator() {
    return (new ImageResampleNode());
}

MStatus ImageResampleNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Factor
    int factor_soft_min = -4;
    int factor_soft_max = 2;
    int factor_default = 0;
    m_factor_attr = nAttr.create(
        "factor", "fctr",
        MFnNumericData::kInt, factor_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setSoftMin(factor_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(factor_soft_max));

    // 'interpolate' toggle
    m_interpolate_attr = nAttr.create(
        "interpolate", "intprte",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // Create Common Attributes
    CHECK_MSTATUS(utils::create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(utils::create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(utils::create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_factor_attr));
    CHECK_MSTATUS(addAttribute(m_interpolate_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_factor_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_interpolate_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
