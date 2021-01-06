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
 * Image Plane Geometry Buffers.
 */

// Maya Viewport 2.0
#include <maya/MHWGeometry.h>

// STL
#include <algorithm>
#include <memory>
#include <cstdlib>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "logger.h"
#include "geometry_buffer.h"
#include "image_plane_geometry_buffer.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

GeometryBuffer::GeometryBuffer() : m_divisions_x(16), m_divisions_y(16),
                                   m_position_buffer(nullptr),
                                   m_uv_buffer(nullptr),
                                   m_shaded_index_buffer(nullptr) {}

GeometryBuffer::~GeometryBuffer() {
    this->clear();
};


size_t GeometryBuffer::divisions_x() const noexcept {
    return m_divisions_x;
}

size_t GeometryBuffer::divisions_y() const noexcept {
    return m_divisions_y;
}

void GeometryBuffer::set_divisions_x(size_t value) {
    m_divisions_x = std::max<size_t>(2, value);
}

void GeometryBuffer::set_divisions_y(size_t value) {
    m_divisions_y = std::max<size_t>(2, value);
}

MHWRender::MVertexBuffer* GeometryBuffer::vertex_positions() const noexcept {
    return GeometryBuffer::m_position_buffer;
}

MHWRender::MVertexBuffer* GeometryBuffer::vertex_uvs() const noexcept {
    return GeometryBuffer::m_uv_buffer;
}

MHWRender::MIndexBuffer* GeometryBuffer::index_triangles() const noexcept {
    return GeometryBuffer::m_shaded_index_buffer;
}

void GeometryBuffer::rebuild() {
    auto log = log::get_logger();
    this->clear();

    log->debug("rebuild geometry buffers");
    log->debug("divisions: {}x{}", m_divisions_x, m_divisions_y);

    m_position_buffer = geometry_buffer::build_vertex_buffer_positions(
        m_divisions_x, m_divisions_y);
    m_uv_buffer = geometry_buffer::build_vertex_buffer_uvs(
        m_divisions_x, m_divisions_y);
    m_shaded_index_buffer = geometry_buffer::build_index_buffer_triangles(
        m_divisions_x, m_divisions_y);
}

void GeometryBuffer::clear() {
    auto log = log::get_logger();
    log->debug("Clearing geometry buffers...");

    delete m_position_buffer;
    m_position_buffer = nullptr;

    delete m_uv_buffer;
    m_uv_buffer = nullptr;

    delete m_shaded_index_buffer;
    m_shaded_index_buffer = nullptr;
}

} // namespace image_plane
} // namespace open_comp_graph_maya
