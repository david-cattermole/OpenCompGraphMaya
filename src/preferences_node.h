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
 * A Maya node interface to store OCG preferences in the scene file.
 */

#ifndef OPENCOMPGRAPHMAYA_PREFERENCES_NODE_H
#define OPENCOMPGRAPHMAYA_PREFERENCES_NODE_H

// Maya
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MTypeId.h>

// OCG
#include "opencompgraph.h"

namespace open_comp_graph_maya {

class PreferencesNode : public MPxNode {
public:
    PreferencesNode();

    virtual ~PreferencesNode();

    virtual MStatus compute(const MPlug &plug, MDataBlock &data);

    static void *creator();

    static MStatus initialize();

    static MString nodeName();

    static MTypeId m_id;

    // Attributes
    static MObject m_mem_cache_enable_attr;
    static MObject m_mem_cache_size_attr;
    static MObject m_disk_cache_base_dir_attr;
    static MObject m_color_space_name_linear_attr;
    static MObject m_ocio_path_enable_attr;
    static MObject m_ocio_path_attr;
};

} // namespace open_comp_graph_maya

#endif //OPENCOMPGRAPHMAYA_PREFERENCES_NODE_H
