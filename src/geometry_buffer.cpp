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

// Maya Viewport 2.0
#include <maya/MHWGeometry.h>

// STL
#include <cstdlib>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "geometry_buffer.h"
#include "logger.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace geometry_buffer {

// Vertex Positions.
void generate_vertex_positions(
        MHWRender::MVertexBuffer *vertex_buffer,
        const size_t divisions_x,
        const size_t divisions_y,
        ocg::StreamData &stream_data) {
    auto log = log::get_logger();

    auto center_x = -0.5f;
    auto center_y = -0.5f;
    auto size_x = 1.0f;
    auto size_y = 1.0f;
    auto geom = ocg::internal::create_geometry_plane_box(
        center_x, center_y,
        size_x, size_y,
        divisions_x, divisions_y);

    auto pos_buffer_size = geom->calc_buffer_size_vertex_positions();
    auto pos_count = geom->calc_count_vertex_positions();
    bool write_only = true;  // We don't need the current buffer values
    float *buffer = static_cast<float *>(
        vertex_buffer->acquire(pos_count, write_only));
    if (buffer) {
        rust::Slice<float> slice{buffer, pos_buffer_size};
        geom->fill_buffer_vertex_positions(slice);

        if (stream_data.deformers_len() > 0) {
            // TODO: Work out the correct maths to ensure lens
            // distortion considers the deformation relative to the
            // display window.
            auto display_window_int = stream_data.display_window();
            auto data_window_int = stream_data.data_window();

            auto display_window = ocg::BBox2Df();
            display_window.min_x = -0.0;
            display_window.min_y = -0.0;
            display_window.max_x = +1.0;
            display_window.max_y = +1.0;

            auto data_window = ocg::BBox2Df();
            data_window.min_x = -0.0;
            data_window.min_y = -0.0;
            data_window.max_x = +1.0;
            data_window.max_y = +1.0;

            log->debug("applying lens distortion!");
            log->debug("display window: min_x={} min_y={} max_x={} max_y={}",
                       display_window.min_x, display_window.min_y,
                       display_window.max_x, display_window.max_y);
            log->debug("data window: min_x={} min_y={} max_x={} max_y={}",
                       data_window.min_x, data_window.min_y,
                       data_window.max_x, data_window.max_y);
            stream_data.apply_deformers(slice, display_window, data_window);
        }
        vertex_buffer->commit(buffer);
    }
    return;
}


// UVs for Vertices.
void generate_vertex_uvs(
        MHWRender::MVertexBuffer* vertex_buffer,
        const size_t divisions_x,
        const size_t divisions_y) {
    auto log = log::get_logger();

    auto center_x = -0.5f;
    auto center_y = -0.5f;
    auto size_x = 1.0f;
    auto size_y = 1.0f;
    auto geom = ocg::internal::create_geometry_plane_box(
        center_x, center_y,
        size_x, size_y,
        divisions_x, divisions_y);

    auto uv_buffer_size = geom->calc_buffer_size_vertex_uvs();
    auto uv_count = geom->calc_count_vertex_uvs();
    bool write_only = true;  // We don't need the current buffer values
    float *buffer = static_cast<float *>(
        vertex_buffer->acquire(uv_count, write_only));
    if (buffer) {
        rust::Slice<float> slice{buffer, uv_buffer_size};
        geom->fill_buffer_vertex_uvs(slice);
        vertex_buffer->commit(buffer);
    }
    return;
}


// Indexes for triangles.
void generate_index_triangles(
        MHWRender::MIndexBuffer *index_buffer,
        const size_t divisions_x,
        const size_t divisions_y) {
    auto center_x = -0.5f;
    auto center_y = -0.5f;
    auto size_x = 1.0f;
    auto size_y = 1.0f;
    auto geom = ocg::internal::create_geometry_plane_box(
        center_x, center_y,
        size_x, size_y,
        divisions_x, divisions_y);

    auto tri_count = geom->calc_buffer_size_index_tris();
    bool write_only = true;  // We don't need the current buffer values
    uint32_t *buffer = static_cast<uint32_t *>(
        index_buffer->acquire(tri_count, write_only));
    if (buffer) {
        rust::Slice<uint32_t> slice{buffer, tri_count};
        geom->fill_buffer_index_tris(slice);
        index_buffer->commit(buffer);
    }
    return;
}


// Indexes for border lines.
void generate_index_border_lines(
        MHWRender::MIndexBuffer *index_buffer,
        const size_t divisions_x,
        const size_t divisions_y) {
    auto center_x = -0.5f;
    auto center_y = -0.5f;
    auto size_x = 1.0f;
    auto size_y = 1.0f;
    auto geom = ocg::internal::create_geometry_plane_box(
        center_x, center_y,
        size_x, size_y,
        divisions_x, divisions_y);

    auto tri_count = geom->calc_buffer_size_index_border_lines();
    bool write_only = true;  // We don't need the current buffer values
    uint32_t *buffer = static_cast<uint32_t *>(
        index_buffer->acquire(tri_count, write_only));
    if (buffer) {
        rust::Slice<uint32_t> slice{buffer, tri_count};
        geom->fill_buffer_index_border_lines(slice);
        index_buffer->commit(buffer);
    }
    return;
}


// Indexes for wire lines.
void generate_index_wire_lines(
        MHWRender::MIndexBuffer *index_buffer,
        const size_t divisions_x,
        const size_t divisions_y) {
    auto center_x = -0.5f;
    auto center_y = -0.5f;
    auto size_x = 1.0f;
    auto size_y = 1.0f;
    auto geom = ocg::internal::create_geometry_plane_box(
        center_x, center_y,
        size_x, size_y,
        divisions_x, divisions_y);

    auto tri_count = geom->calc_buffer_size_index_wire_lines();
    bool write_only = true;  // We don't need the current buffer values
    uint32_t *buffer = static_cast<uint32_t *>(
        index_buffer->acquire(tri_count, write_only));
    if (buffer) {
        rust::Slice<uint32_t> slice{buffer, tri_count};
        geom->fill_buffer_index_wire_lines(slice);
        index_buffer->commit(buffer);
    }

    return;
}


// Vertices for positions.
void generate_window_vertex_positions(
        MHWRender::MVertexBuffer* vertex_buffer,
        const ocg::BBox2Di bounding_box) {
    auto log = log::get_logger();

    const auto per_vertex_pos_count = 3;
    auto count = 4;
    bool write_only = true;
    float *buffer = static_cast<float *>(
        vertex_buffer->acquire(count, write_only));
    if (buffer) {
        // Vertex 0 - lower-left
        auto vertex_idx = 0;
        buffer[vertex_idx + 0] = static_cast<float>(bounding_box.min_x);
        buffer[vertex_idx + 1] = static_cast<float>(bounding_box.min_y);
        buffer[vertex_idx + 2] = 0.0;
        vertex_idx += per_vertex_pos_count;

        // Vertex 1 - upper-left
        buffer[vertex_idx + 0] = static_cast<float>(bounding_box.min_x);
        buffer[vertex_idx + 1] = static_cast<float>(bounding_box.max_y);
        buffer[vertex_idx + 2] = 0.0;
        vertex_idx += per_vertex_pos_count;

        // Vertex 2 - upper-right
        buffer[vertex_idx + 0] = static_cast<float>(bounding_box.max_x);
        buffer[vertex_idx + 1] = static_cast<float>(bounding_box.max_y);
        buffer[vertex_idx + 2] = 0.0;
        vertex_idx += per_vertex_pos_count;

        // Vertex 3 - lower-right
        buffer[vertex_idx + 0] = static_cast<float>(bounding_box.max_x);
        buffer[vertex_idx + 1] = static_cast<float>(bounding_box.min_y);
        buffer[vertex_idx + 2] = 0.0;
        vertex_idx += per_vertex_pos_count;

        vertex_buffer->commit(buffer);
    }
    return;
}


// Indexes for border lines.
void generate_window_index_border_lines(
        MHWRender::MIndexBuffer* index_buffer) {
    auto count = 8;
    bool write_only = true;
    uint32_t *buffer = static_cast<uint32_t *>(
        index_buffer->acquire(count, write_only));
    if (buffer) {
        // Left
        buffer[0] = 1;
        buffer[1] = 0;
        // Right
        buffer[2] = 2;
        buffer[3] = 3;
        // Top
        buffer[4] = 1;
        buffer[5] = 2;
        // Bottom
        buffer[6] = 0;
        buffer[7] = 3;
        index_buffer->commit(buffer);
    }
    return;
}


// VertexBuffer for positions.
MHWRender::MVertexBuffer* build_vertex_buffer_positions(
        const size_t divisions_x,
        const size_t divisions_y,
        ocg::StreamData &stream_data) {
    const auto per_vertex_pos_count = 3;
    const MHWRender::MVertexBufferDescriptor vb_desc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        per_vertex_pos_count);
    MHWRender::MVertexBuffer* vertex_buffer = new MHWRender::MVertexBuffer(vb_desc);
    if (vertex_buffer) {
        generate_vertex_positions(
            vertex_buffer,
            divisions_x,
            divisions_y,
            stream_data);
    }
    return vertex_buffer;
}


// UV Vertex Buffer
MHWRender::MVertexBuffer* build_vertex_buffer_uvs(
        const size_t divisions_x,
        const size_t divisions_y) {
    const auto per_vertex_uv_count = 2;
    const MHWRender::MVertexBufferDescriptor uv_desc(
        "",
        MHWRender::MGeometry::kTexture,
        MHWRender::MGeometry::kFloat,
        per_vertex_uv_count);
    MHWRender::MVertexBuffer* vertex_buffer = new MHWRender::MVertexBuffer(uv_desc);
    if (vertex_buffer) {
        generate_vertex_uvs(vertex_buffer, divisions_x, divisions_y);
    }
    return vertex_buffer;
}

// Index buffer for triangles.
MHWRender::MIndexBuffer* build_index_buffer_triangles(
        const size_t divisions_x,
        const size_t divisions_y) {
    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        generate_index_triangles(index_buffer, divisions_x, divisions_y);
    }
    return index_buffer;
}


// Index buffer for border lines.
MHWRender::MIndexBuffer* build_index_buffer_border_lines(
        const size_t divisions_x,
        const size_t divisions_y) {
    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        generate_index_border_lines(
            index_buffer,
            divisions_x,
            divisions_y);
    }
    return index_buffer;
}


// Index buffer for wire lines.
MHWRender::MIndexBuffer* build_index_buffer_wire_lines(
        const size_t divisions_x,
        const size_t divisions_y) {
    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        generate_index_wire_lines(
            index_buffer,
            divisions_x,
            divisions_y);
    }
    return index_buffer;
}


// VertexBuffer for positions, of window.
MHWRender::MVertexBuffer* build_window_vertex_buffer_positions(
        const ocg::BBox2Di bounding_box) {
    auto log = log::get_logger();

    const auto per_vertex_pos_count = 3;
    const MHWRender::MVertexBufferDescriptor vb_desc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        per_vertex_pos_count);
    MHWRender::MVertexBuffer* vertex_buffer = new MHWRender::MVertexBuffer(vb_desc);
    if (vertex_buffer) {
        generate_window_vertex_positions(
            vertex_buffer,
            bounding_box);
    }
    return vertex_buffer;
}


// Index buffer for border lines, of window.
MHWRender::MIndexBuffer* build_window_index_buffer_border_lines() {
    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        generate_window_index_border_lines(index_buffer);
    }
    return index_buffer;
}


} // namespace geometry_buffer
} // namespace open_comp_graph_maya
