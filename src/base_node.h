/*
 * Copyright (C) 2020, 2021 David Cattermole.
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
 * Base class node for all OCG nodes.
 */

#ifndef OPENCOMPGRAPHMAYA_BASE_NODE_H
#define OPENCOMPGRAPHMAYA_BASE_NODE_H

// Maya
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MTypeId.h>

// OCG
#include "opencompgraph.h"

namespace open_comp_graph_maya {

class BaseNode : public MPxNode {
public:

    BaseNode();

    virtual ~BaseNode();

    void postConstructor();

    static MStatus
    create_enable_attribute(MObject &attr);

    static MStatus
    create_input_stream_attribute(MObject &attr);

    static MStatus
    create_output_stream_attribute(MObject &attr);

protected:
    uint64_t m_ocg_node_hash;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_BASE_NODE_H
