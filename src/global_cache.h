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
 * A managing the shared (global) cache.
 */

#ifndef OPENCOMPGRAPHMAYA_GLOBAL_CACHE_H
#define OPENCOMPGRAPHMAYA_GLOBAL_CACHE_H

// STL
#include <iostream>
#include <memory>

// OCG
#include <opencompgraph.h>

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace cache {

std::shared_ptr<ocg::Cache> &get_shared_cache();

std::shared_ptr<ocg::Cache> &get_shared_color_transform_cache();

} // namespace cache
} // namespace open_comp_graph_maya

#endif //OPENCOMPGRAPHMAYA_GLOBAL_CACHE_H
