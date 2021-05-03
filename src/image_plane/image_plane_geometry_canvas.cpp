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
 * Image Plane Canvas Geometry Buffers.
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
#include "image_plane_geometry_canvas.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

GeometryCanvas::GeometryCanvas()
        : m_divisions_x(16)
        , m_divisions_y(16)
        , m_position_buffer(nullptr)
        , m_uv_buffer(nullptr)
        , m_wire_lines_index_buffer(nullptr)
        , m_border_lines_index_buffer(nullptr)
        , m_shaded_index_buffer(nullptr) {}

GeometryCanvas::~GeometryCanvas() {
    this->clear_all();
};


size_t GeometryCanvas::divisions_x() const noexcept {
    return m_divisions_x;
}

size_t GeometryCanvas::divisions_y() const noexcept {
    return m_divisions_y;
}

void GeometryCanvas::set_divisions_x(size_t value) {
    m_divisions_x = std::max<size_t>(2, value);
}

void GeometryCanvas::set_divisions_y(size_t value) {
    m_divisions_y = std::max<size_t>(2, value);
}

///////////////////////////////////////////////////////////////////////////////

void GeometryCanvas::fill_vertex_buffer_positions(
        MHWRender::MVertexBuffer *vertex_buffer,
        ocg::StreamData &stream_data) {
    geometry_buffer::generate_vertex_positions(
        vertex_buffer,
        m_divisions_x,
        m_divisions_y,
        std::move(stream_data));
    return;
}

void GeometryCanvas::fill_vertex_buffer_uvs(
        MHWRender::MVertexBuffer *vertex_buffer) {
    geometry_buffer::generate_vertex_uvs(
        vertex_buffer,
        m_divisions_x,
        m_divisions_y);
    return;
}

void GeometryCanvas::fill_index_buffer_triangles(
        MHWRender::MIndexBuffer* index_buffer) {
    geometry_buffer::generate_index_triangles(
        index_buffer,
        m_divisions_x,
        m_divisions_y);
    return;
}

void GeometryCanvas::fill_index_buffer_border_lines(
        MHWRender::MIndexBuffer* index_buffer) {
    geometry_buffer::generate_index_border_lines(
        index_buffer,
        m_divisions_x,
        m_divisions_y);
    return;
}

void GeometryCanvas::fill_index_buffer_wire_lines(
        MHWRender::MIndexBuffer* index_buffer) {
    geometry_buffer::generate_index_wire_lines(
        index_buffer,
        m_divisions_x,
        m_divisions_y);
    return;
}

///////////////////////////////////////////////////////////////////////////////

MHWRender::MVertexBuffer* GeometryCanvas::vertex_buffer_positions() const noexcept {
    return GeometryCanvas::m_position_buffer;
}

MHWRender::MVertexBuffer* GeometryCanvas::vertex_buffer_uvs() const noexcept {
    return GeometryCanvas::m_uv_buffer;
}

MHWRender::MIndexBuffer* GeometryCanvas::index_buffer_triangles() const noexcept {
    return GeometryCanvas::m_shaded_index_buffer;
}

MHWRender::MIndexBuffer* GeometryCanvas::index_buffer_border_lines() const noexcept {
    return GeometryCanvas::m_border_lines_index_buffer;
}

MHWRender::MIndexBuffer* GeometryCanvas::index_buffer_wire_lines() const noexcept {
    return GeometryCanvas::m_wire_lines_index_buffer;
}

///////////////////////////////////////////////////////////////////////////////

void GeometryCanvas::rebuild_vertex_buffer_positions(ocg::StreamData &stream_data) {
    this->clear_vertex_positions();
    m_position_buffer = geometry_buffer::build_vertex_buffer_positions(
        m_divisions_x, m_divisions_y, std::move(stream_data));
}

void GeometryCanvas::rebuild_vertex_buffer_uvs() {
    this->clear_vertex_uvs();
    m_uv_buffer = geometry_buffer::build_vertex_buffer_uvs(
        m_divisions_x, m_divisions_y);
}

void GeometryCanvas::rebuild_index_buffer_triangles() {
    this->clear_index_triangles();
    m_shaded_index_buffer = geometry_buffer::build_index_buffer_triangles(
        m_divisions_x, m_divisions_y);
}

void GeometryCanvas::rebuild_index_buffer_border_lines() {
    this->clear_index_border_lines();
    m_border_lines_index_buffer = geometry_buffer::build_index_buffer_border_lines(
        m_divisions_x, m_divisions_y);
}

void GeometryCanvas::rebuild_index_buffer_wire_lines() {
    this->clear_index_wire_lines();
    m_wire_lines_index_buffer = geometry_buffer::build_index_buffer_wire_lines(
        m_divisions_x, m_divisions_y);
}

void GeometryCanvas::rebuild_buffer_all(ocg::StreamData &stream_data) {
    auto log = log::get_logger();
    this->clear_all();
    log->debug("rebuild_buffer_all geometry buffers");
    log->debug("divisions: {}x{}", m_divisions_x, m_divisions_y);
    m_position_buffer = geometry_buffer::build_vertex_buffer_positions(
        m_divisions_x, m_divisions_y, std::move(stream_data));
    m_uv_buffer = geometry_buffer::build_vertex_buffer_uvs(
        m_divisions_x, m_divisions_y);
    m_shaded_index_buffer = geometry_buffer::build_index_buffer_triangles(
        m_divisions_x, m_divisions_y);
    m_border_lines_index_buffer = geometry_buffer::build_index_buffer_border_lines(
        m_divisions_x, m_divisions_y);
    m_wire_lines_index_buffer = geometry_buffer::build_index_buffer_wire_lines(
        m_divisions_x, m_divisions_y);
}

///////////////////////////////////////////////////////////////////////////////

void GeometryCanvas::clear_vertex_positions() {
    delete m_position_buffer;
    m_position_buffer = nullptr;
}

void GeometryCanvas::clear_vertex_uvs() {
    delete m_uv_buffer;
    m_uv_buffer = nullptr;
}

void GeometryCanvas::clear_index_triangles() {
    delete m_shaded_index_buffer;
    m_shaded_index_buffer = nullptr;
}

void GeometryCanvas::clear_index_border_lines() {
    delete m_border_lines_index_buffer;
    m_border_lines_index_buffer = nullptr;
}

void GeometryCanvas::clear_index_wire_lines() {
    delete m_wire_lines_index_buffer;
    m_wire_lines_index_buffer = nullptr;
}

void GeometryCanvas::clear_all() {
    clear_vertex_positions();
    clear_vertex_uvs();
    clear_index_triangles();
    clear_index_border_lines();
    clear_index_wire_lines();
}

} // namespace image_plane
} // namespace open_comp_graph_maya
