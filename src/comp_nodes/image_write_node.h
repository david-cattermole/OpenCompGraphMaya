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
 * Node to write an image from disk.
 */

#ifndef OPENCOMPGRAPHMAYA_IMAGE_WRITE_NODE_H
#define OPENCOMPGRAPHMAYA_IMAGE_WRITE_NODE_H

// Maya
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MTypeId.h>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "base_node.h"

namespace open_comp_graph_maya {

class ImageWriteNode : public BaseNode {
public:
    ImageWriteNode();

    virtual ~ImageWriteNode();

    virtual MStatus compute(const MPlug &plug, MDataBlock &data);

    static void *creator();

    static MStatus initialize();

    static MString nodeName();

    virtual MStatus updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node);

    // Maya Node Type Id
    static MTypeId m_id;

    // Input Attributes
    static MObject m_in_stream_attr;
    static MObject m_enable_attr;
    static MObject m_file_path_attr;
    static MObject m_crop_on_write_attr;
    static MObject m_pixel_data_type_attr;
    static MObject m_exr_compression_attr;
    static MObject m_exr_dwa_compression_level_attr;
    static MObject m_png_compression_level_attr;
    static MObject m_jpeg_compression_level_attr;
    static MObject m_jpeg_chroma_sub_sampling_attr;
    static MObject m_jpeg_progressive_attr;

    // Output Attributes
    static MObject m_out_stream_attr;

private:
    ocg::Node m_ocg_node;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_WRITE_NODE_H
