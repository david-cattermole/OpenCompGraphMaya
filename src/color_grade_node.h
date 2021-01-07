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
 * OCG node to read an image from disk.
 */

#ifndef OPENCOMPGRAPHMAYA_COLOR_GRADE_NODE_H
#define OPENCOMPGRAPHMAYA_COLOR_GRADE_NODE_H

// Maya
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MTypeId.h>

// OCG
#include "opencompgraph.h"
namespace ocg = open_comp_graph;


namespace open_comp_graph_maya {

class ColorGradeNode : public MPxNode {
public:
    ColorGradeNode();

    virtual ~ColorGradeNode();

    virtual MStatus compute(const MPlug &plug, MDataBlock &data);

    void postConstructor();

    static void *creator();

    static MStatus initialize();

    static MString nodeName();

    static MTypeId m_id;

    // Input Attributes
    static MObject m_in_stream_attr;
    static MObject m_enable_attr;
    static MObject m_multiply_r_attr;
    static MObject m_multiply_g_attr;
    static MObject m_multiply_b_attr;
    static MObject m_multiply_a_attr;

    // Output Attributes
    static MObject m_out_stream_attr;

private:
    ocg::Node m_ocg_node;
    uint64_t m_ocg_node_hash;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_COLOR_GRADE_NODE_H
