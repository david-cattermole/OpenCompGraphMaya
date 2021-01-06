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
 * General geometry buffer related functions.
 */

#ifndef OPENCOMPGRAPHMAYA_GEOMETRY_BUFFER_H
#define OPENCOMPGRAPHMAYA_GEOMETRY_BUFFER_H

// Maya Viewport 2.0
#include <maya/MHWGeometry.h>

namespace open_comp_graph_maya {
namespace geometry_buffer {

MHWRender::MVertexBuffer* build_vertex_buffer_positions(
    const size_t divisions_x,
    const size_t divisions_y);

MHWRender::MVertexBuffer* build_vertex_buffer_uvs(
    const size_t divisions_x,
    const size_t divisions_y);

MHWRender::MIndexBuffer* build_index_buffer_triangles(
    const size_t divisions_x,
    const size_t divisions_y);

} // namespace geometry_buffer
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_GEOMETRY_BUFFER_H
