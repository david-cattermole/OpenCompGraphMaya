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

#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_WINDOW_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_WINDOW_H

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

class GeometryWindow {
public:

    GeometryWindow();
    ~GeometryWindow();

    ocg::BBox2Di bounding_box() const noexcept;
    void set_bounding_box(ocg::BBox2Di value);

    void fill_vertex_buffer_positions(MHWRender::MVertexBuffer* vertex_buffer);
    void fill_index_buffer_border_lines(MHWRender::MIndexBuffer *index_buffer);

    MHWRender::MVertexBuffer* vertex_buffer_positions() const noexcept;
    MHWRender::MIndexBuffer* index_buffer_border_lines() const noexcept;

    // Create VertexBuffers and Index Buffers.
    void rebuild_vertex_buffer_positions();
    void rebuild_index_buffer_border_lines();
    void rebuild_buffer_all();

    // delete VertexBuffers and Index Buffers.
    void clear_vertex_positions();
    void clear_index_border_lines();
    void clear_all();

private:

    // Describe how we will generate the geometry.
    ocg::BBox2Di m_bbox;

    // The internal buffer data.
    MHWRender::MVertexBuffer* m_position_buffer;
    MHWRender::MIndexBuffer* m_border_lines_index_buffer;
};

} // namespace image_plane
} // namespace open_comp_graph_maya

#endif //OPENCOMPGRAPHMAYA_IMAGE_PLANE_GEOMETRY_WINDOW_H
