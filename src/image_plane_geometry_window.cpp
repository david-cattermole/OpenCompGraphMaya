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
 * Image Plane Geometry Window Buffers.
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
#include "image_plane_geometry_window.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

GeometryWindow::GeometryWindow() : m_bbox(),
                                   m_position_buffer(nullptr),
                                   m_border_lines_index_buffer(nullptr) {
    m_bbox.min_x = 0;
    m_bbox.min_y = 0;
    m_bbox.max_x = 0;
    m_bbox.max_y = 0;
}

GeometryWindow::~GeometryWindow() {
    this->clear_all();
};


ocg::BBox2Di GeometryWindow::bounding_box() const noexcept {
    return m_bbox;
}

void GeometryWindow::set_bounding_box(ocg::BBox2Di value) {
    m_bbox = value;
}

MHWRender::MVertexBuffer* GeometryWindow::vertex_positions() const noexcept {
    return GeometryWindow::m_position_buffer;
}

MHWRender::MIndexBuffer* GeometryWindow::index_border_lines() const noexcept {
    return GeometryWindow::m_border_lines_index_buffer;
}

void GeometryWindow::rebuild_vertex_positions() {
    this->clear_vertex_positions();
    m_position_buffer =
        geometry_buffer::build_window_vertex_buffer_positions(m_bbox);
}

void GeometryWindow::rebuild_index_border_lines() {
    this->clear_index_border_lines();
    m_border_lines_index_buffer =
        geometry_buffer::build_window_index_buffer_border_lines();
}

void GeometryWindow::rebuild_all() {
    auto log = log::get_logger();
    this->clear_all();
    log->debug("rebuild_all geometry buffers");
    log->debug("bbox: {},{} to {},{}",
               m_bbox.min_x, m_bbox.min_y,
               m_bbox.max_x, m_bbox.max_y);
    m_position_buffer =
        geometry_buffer::build_window_vertex_buffer_positions(m_bbox);
    m_border_lines_index_buffer =
        geometry_buffer::build_window_index_buffer_border_lines();
}

void GeometryWindow::clear_vertex_positions() {
    delete m_position_buffer;
    m_position_buffer = nullptr;
}

void GeometryWindow::clear_index_border_lines() {
    delete m_border_lines_index_buffer;
    m_border_lines_index_buffer = nullptr;
}

void GeometryWindow::clear_all() {
    this->clear_vertex_positions();
    this->clear_index_border_lines();
}

} // namespace image_plane
} // namespace open_comp_graph_maya
