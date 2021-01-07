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
 * Holds OCG Stream Data in the Maya DG.
 */

#ifndef OPENCOMPGRAPHMAYA_GRAPH_DATA_H
#define OPENCOMPGRAPHMAYA_GRAPH_DATA_H

// Maya
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MPxData.h>
#include <maya/MTypeId.h>

// STL
#include <memory>

// OCG
#include "opencompgraph.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya{

class GraphData : public MPxData {
public:
    GraphData();
    virtual ~GraphData();

    virtual MStatus readASCII(const MArgList&, unsigned& lastElement);
    virtual MStatus readBinary(istream& in, unsigned length);
    virtual MStatus writeASCII(ostream& out);
    virtual MStatus writeBinary(ostream& out);

    virtual void copy(const MPxData&);
    MTypeId typeId() const;
    MString name() const;

    std::shared_ptr<ocg::Graph> get_graph() const;
    bool is_valid_graph() const;
    void set_graph(std::shared_ptr<ocg::Graph> value);

    ocg::Node get_node() const;
    void set_node(ocg::Node value);

    static MString typeName();

    static const MString m_type_name;
    static const MTypeId m_id;
    static void* creator();

private:
    std::shared_ptr<ocg::Graph> m_graph;
    ocg::Node m_ocg_node;
};

} // namespace open_comp_graph_maya

#endif //OPENCOMPGRAPHMAYA_GRAPH_DATA_H
