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
 * Image Plane utilities.
 */

// Maya
#include <maya/MObject.h>
#include <maya/MDataBlock.h>
#include <maya/MString.h>
#include <maya/MPlug.h>

// STL
#include <tuple>

// OCG
#include "opencompgraph.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {
namespace utils {

double getAngleOfView(
    const double filmBackSize, // millimeters
    const double focalLength,  // millimeters
    bool asDegrees);

double getCameraPlaneScale(
    const double filmBackSize, // millimeters
    const double focalLength); // millimeters

// Get distance attribute value.
std::tuple<float, bool>
get_plug_value_distance_float(MPlug plug, float old_value);

// Get floating point attribute value.
std::tuple<float, bool>
get_plug_value_frame_float(MPlug plug, float old_value);

// Get boolean attribute value.
std::tuple<bool, bool>
get_plug_value_bool(MPlug plug, bool old_value);

// Get unsigned integer attribute value.
std::tuple<uint32_t, bool>
get_plug_value_uint32(MPlug plug, uint32_t old_value);

// Get floating point attribute value.
std::tuple<float, bool>
get_plug_value_float(MPlug plug, float old_value);

// Get String attribute value.
std::tuple<MString, bool>
get_plug_value_string(MPlug plug, MString old_value);

// Get the ocgStreamData type from the given plug.
std::tuple<ocg::Node, bool>
get_plug_value_stream(MPlug plug, ocg::Node old_value);

} // namespace utils
} // namespace image_plane
} // namespace open_comp_graph_maya
