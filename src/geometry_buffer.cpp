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

// VertexBuffer for positions.
MHWRender::MVertexBuffer* build_vertex_buffer_positions(
    const size_t divisions_x,
    const size_t divisions_y,
    ocg::StreamData &stream_data) {
    auto log = log::get_logger();

    auto geom = ocg::internal::create_geometry_plane_box(divisions_x, divisions_y);

    const auto per_vertex_pos_count = 3;
    const MHWRender::MVertexBufferDescriptor vb_desc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        per_vertex_pos_count);
    MHWRender::MVertexBuffer* vertex_buffer = new MHWRender::MVertexBuffer(vb_desc);
    if (vertex_buffer) {
        auto pos_buffer_size = geom->calc_buffer_size_vertex_positions();
        auto pos_count = geom->calc_count_vertex_positions();
        bool write_only = true;  // We don't need the current buffer values
        float *buffer = static_cast<float *>(
            vertex_buffer->acquire(pos_count, write_only));
        if (buffer) {
            rust::Slice<float> slice{buffer, pos_buffer_size};
            geom->fill_buffer_vertex_positions(slice);
            if (stream_data.deformers_len() > 0) {
                log->warn("applying lens distortion!");
                stream_data.apply_deformers(slice);
            }
            // for (int i = 0; i < pos_count; ++i) {
            //     int index = i * per_vertex_pos_count;
            //     log->debug(
            //         "ocgImagePlane: positions: {}={} {}={}",
            //         index + 0, buffer[index + 0],
            //         index + 1, buffer[index + 1]
            //     );
            // }
            vertex_buffer->commit(buffer);
        }
    }
    return vertex_buffer;
}

// UV Vertex Buffer
MHWRender::MVertexBuffer* build_vertex_buffer_uvs(
    const size_t divisions_x,
    const size_t divisions_y) {
    auto log = log::get_logger();

    auto geom = ocg::internal::create_geometry_plane_box(divisions_x, divisions_y);

    const auto per_vertex_uv_count = 2;
    const MHWRender::MVertexBufferDescriptor uv_desc(
        "",
        MHWRender::MGeometry::kTexture,
        MHWRender::MGeometry::kFloat,
        per_vertex_uv_count);
    MHWRender::MVertexBuffer* vertex_buffer = new MHWRender::MVertexBuffer(uv_desc);
    if (vertex_buffer) {
        auto uv_buffer_size = geom->calc_buffer_size_vertex_uvs();
        auto uv_count = geom->calc_count_vertex_uvs();
        bool write_only = true;  // We don't need the current buffer values
        float *buffer = static_cast<float *>(
            vertex_buffer->acquire(uv_count, write_only));
        if (buffer) {
            rust::Slice<float> slice{buffer, uv_buffer_size};
            geom->fill_buffer_vertex_uvs(slice);
            // for (int i = 0; i < uv_count; ++i) {
            //     int index = i * per_vertex_uv_count;
            //     log->debug(
            //         "ocgImagePlane: uvs: {}={} {}={}",
            //         index + 0, buffer[index + 0],
            //         index + 1, buffer[index + 1]
            //     );
            // }
            vertex_buffer->commit(buffer);
        }
    }
    return vertex_buffer;
}

// Index buffer for triangles.
MHWRender::MIndexBuffer* build_index_buffer_triangles(
    const size_t divisions_x,
    const size_t divisions_y) {

    auto geom = ocg::internal::create_geometry_plane_box(divisions_x, divisions_y);

    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        auto tri_count = geom->calc_buffer_size_index_tris();
        bool write_only = true;  // We don't need the current buffer values
        uint32_t *buffer = static_cast<uint32_t *>(
            index_buffer->acquire(tri_count, write_only));
        if (buffer) {
            rust::Slice<uint32_t> slice{buffer, tri_count};
            geom->fill_buffer_index_tris(slice);
            // for (int i = 0; i < tri_count; ++i) {
            //     log->debug("ocgImagePlane: indices {}={}",
            //                i, buffer[i]);
            // }
            index_buffer->commit(buffer);
        }
    }
    return index_buffer;
}


// Index buffer for border lines.
MHWRender::MIndexBuffer* build_index_buffer_border_lines(
    const size_t divisions_x,
    const size_t divisions_y) {

    auto geom = ocg::internal::create_geometry_plane_box(divisions_x, divisions_y);

    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        auto tri_count = geom->calc_buffer_size_index_border_lines();
        bool write_only = true;  // We don't need the current buffer values
        uint32_t *buffer = static_cast<uint32_t *>(
            index_buffer->acquire(tri_count, write_only));
        if (buffer) {
            rust::Slice<uint32_t> slice{buffer, tri_count};
            geom->fill_buffer_index_border_lines(slice);
            // for (int i = 0; i < tri_count; ++i) {
            //     log->debug("ocgImagePlane: indices {}={}",
            //                i, buffer[i]);
            // }
            index_buffer->commit(buffer);
        }
    }
    return index_buffer;
}


// Index buffer for wire lines.
MHWRender::MIndexBuffer* build_index_buffer_wire_lines(
    const size_t divisions_x,
    const size_t divisions_y) {

    auto geom = ocg::internal::create_geometry_plane_box(divisions_x, divisions_y);

    MHWRender::MIndexBuffer* index_buffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (index_buffer) {
        auto tri_count = geom->calc_buffer_size_index_wire_lines();
        bool write_only = true;  // We don't need the current buffer values
        uint32_t *buffer = static_cast<uint32_t *>(
            index_buffer->acquire(tri_count, write_only));
        if (buffer) {
            rust::Slice<uint32_t> slice{buffer, tri_count};
            geom->fill_buffer_index_wire_lines(slice);
            // for (int i = 0; i < tri_count; ++i) {
            //     log->debug("ocgImagePlane: indices {}={}",
            //                i, buffer[i]);
            // }
            index_buffer->commit(buffer);
        }
    }
    return index_buffer;
}


} // namespace geometry_buffer
} // namespace open_comp_graph_maya
