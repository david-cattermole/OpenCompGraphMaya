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

// STL
#include <memory>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "global_cache.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace cache {

std::shared_ptr<ocg::Cache> &get_shared_cache() {
    static std::shared_ptr<ocg::Cache> shared_cache = std::make_shared<ocg::Cache>();
    return shared_cache;
}

std::shared_ptr<ocg::Cache> &get_shared_color_transform_cache() {
    static std::shared_ptr<ocg::Cache> shared_color_transform_cache = \
        std::make_shared<ocg::Cache>();
    return shared_color_transform_cache;
}

} // namespace cache
} // namespace open_comp_graph_maya
