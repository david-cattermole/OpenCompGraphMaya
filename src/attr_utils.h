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
 * Attribute utilities.
 */

#ifndef OPENCOMPGRAPHMAYA_ATTR_UTILS_H
#define OPENCOMPGRAPHMAYA_ATTR_UTILS_H

// Maya
#include <maya/MStatus.h>
#include <maya/MObject.h>

namespace open_comp_graph_maya {
namespace utils {

extern
MStatus create_node_disk_cache_attributes(
    MObject &enable_attr,
    MObject &file_name_attr);

} // namespace utils
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_ATTR_UTILS_H
