/*
 * Copyright (C) 2020 David Cattermole.
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

#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_H

// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// STL
#include <map>
#include <memory>

// OCG
#include "opencompgraph.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya{
namespace image_plane{

class Geometry {
public:

    Geometry();
    ~Geometry();

    size_t divisions_x() const noexcept;
    size_t divisions_y() const noexcept;

    void set_divisions_x(size_t value);
    void set_divisions_y(size_t value);

    MHWRender::MVertexBuffer* vertex_positions() const noexcept;
    MHWRender::MVertexBuffer* vertex_uvs() const noexcept;
    MHWRender::MIndexBuffer* index_triangles() const noexcept;

    // Create VertexBuffers and Index Buffers.
    void rebuild_vertex_positions(ocg::StreamData &stream_data);
    void rebuild_vertex_uvs();
    void rebuild_index_triangles();
    void rebuild_all(ocg::StreamData &stream_data);

    // delete VertexBuffers and Index Buffers.
    void clear_vertex_positions();
    void clear_vertex_uvs();
    void clear_index_triangles();
    void clear_all();

private:

    // Describe how we will generate the geometry.
    size_t m_divisions_x;
    size_t m_divisions_y;

    // The internal buffer data.
    MHWRender::MVertexBuffer* m_position_buffer;
    MHWRender::MVertexBuffer* m_uv_buffer;
    MHWRender::MIndexBuffer* m_shaded_index_buffer;
};

} // namespace image_plane
} // namespace open_comp_graph_maya

#endif //OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_H
