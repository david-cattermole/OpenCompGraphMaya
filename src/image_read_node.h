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
 * Node to read an image from disk.
 */

#ifndef OPENCOMPGRAPHMAYA_IMAGE_READ_NODE_H
#define OPENCOMPGRAPHMAYA_IMAGE_READ_NODE_H

// Maya
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MTypeId.h>

namespace open_comp_graph_maya {

class ImageReadNode : public MPxNode {
public:
    ImageReadNode();

    virtual ~ImageReadNode();

    virtual MStatus compute(const MPlug &plug, MDataBlock &data);

    void postConstructor();

    static void *creator();

    static MStatus initialize();

    static MString nodeName();

    static MTypeId m_id;

    // Input Attributes
    static MObject m_enable_attr;
    static MObject m_file_path_attr;
    static MObject m_k1_attr;
    static MObject m_k2_attr;
    static MObject m_time_attr;

    // Output Attributes
    static MObject m_out_stream_attr;

private:
    uint64_t m_ocg_node_hash;
    uint64_t m_ocg_node_id;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_READ_NODE_H
