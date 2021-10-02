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
#include "node_utils.h"
#include "attr_utils.h"

#include "image_cache_node.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId ImageCacheNode::m_id(OCGM_IMAGE_CACHE_TYPE_ID);

// Input Attributes
MObject ImageCacheNode::m_in_stream_attr;
MObject ImageCacheNode::m_disk_cache_enable_attr;
MObject ImageCacheNode::m_disk_cache_file_path_attr;

// Output Attributes
MObject ImageCacheNode::m_out_stream_attr;

ImageCacheNode::ImageCacheNode()
    : m_ocg_read_node(ocg::Node(ocg::NodeType::kNull, 0)) {
}

ImageCacheNode::~ImageCacheNode() {}

MString ImageCacheNode::nodeName() {
    return MString(OCGM_IMAGE_CACHE_TYPE_NAME);
}

MStatus ImageCacheNode::updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node) {
    MStatus status = MS::kSuccess;
    if (input_ocg_nodes.size() != 1) {
        return MS::kFailure;
    }

    bool read_exists = shared_graph->node_exists(m_ocg_read_node);
    if (!read_exists) {
        MString node_name = "read_cache";
        auto read_node_hash = utils::generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_read_node =
            shared_graph->create_node(
                ocg::NodeType::kReadImage,
                read_node_hash);
    }

    auto input_ocg_node = input_ocg_nodes[0];
    uint8_t input_num = 0;
    status = utils::join_ocg_nodes(
        shared_graph,
        input_ocg_node,
        m_ocg_read_node,
        input_num);
    CHECK_MSTATUS(status);

    bool use_disk_cache = utils::get_attr_value_bool(
        data,
        m_disk_cache_enable_attr);

    // Set fallback output
    output_ocg_node = input_ocg_node;

    if (m_ocg_read_node.get_id() != 0) {
        if (use_disk_cache) {
            output_ocg_node = m_ocg_read_node;
        }

        // Disk Cache File Path
        MString file_path = utils::get_attr_value_string(
            data, m_disk_cache_file_path_attr);
        shared_graph->set_node_attr_str(
            m_ocg_read_node, "file_path", file_path.asChar());
    }

    return status;
}

MStatus ImageCacheNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageCacheNode::creator() {
    return (new ImageCacheNode());
}

MStatus ImageCacheNode::initialize() {
    MStatus status;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Create Common Attributes
    CHECK_MSTATUS(utils::create_node_disk_cache_attributes(
                      m_disk_cache_enable_attr,
                      m_disk_cache_file_path_attr));
    CHECK_MSTATUS(utils::create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(utils::create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_disk_cache_enable_attr));
    CHECK_MSTATUS(addAttribute(m_disk_cache_file_path_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_disk_cache_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_disk_cache_file_path_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
