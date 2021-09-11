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
#include <maya/MDataHandle.h>
#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MTime.h>
#include <maya/MDistance.h>
#include <maya/MFnPluginData.h>

// STL
#include <cmath>
#include <tuple>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include "graph_data.h"
#include "logger.h"
#include "../node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {
namespace utils {

// Radians / Degrees
#define RADIANS_TO_DEGREES 57.295779513082323 // 180.0/M_PI
#define DEGREES_TO_RADIANS 0.017453292519943295 // M_PI/180.0

double getAngleOfView(
    const double filmBackSize, // millimeters
    const double focalLength,  // millimeters
    bool asDegrees) {
    double angleOfView = 2.0 * std::atan(filmBackSize * (0.5 / focalLength));
    if (asDegrees) {
        angleOfView *= RADIANS_TO_DEGREES;
    }
    return angleOfView;
}

#undef RADIANS_TO_DEGREES
#undef DEGREES_TO_RADIANS

double getCameraPlaneScale(
    const double filmBackSize,  // millimeters
    const double focalLength) { // millimeters
    const bool asDegrees = true;
    double aov = getAngleOfView(filmBackSize, focalLength, asDegrees);
    // Hard-code 'pi' so we don't have cross-platform problems
    // between Linux and Windows.
    const double pi = 3.14159265358979323846;
    double scale = std::tan(aov * 0.5 * pi / 180.0);
    return scale;
}

// Get distance attribute value.
std::tuple<float, bool>
get_plug_value_distance_float(MPlug plug, float old_value) {
    bool has_changed = false;
    float value = old_value;
    if (!plug.isNull()) {
        float new_value = 0.0;
        MDistance temp_value;
        if (plug.getValue(temp_value)) {
            new_value = static_cast<float>(temp_value.asCentimeters());
        }
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get floating point attribute value.
std::tuple<float, bool>
get_plug_value_frame_float(MPlug plug, float old_value) {
    MStatus status;
    bool has_changed = false;
    float value = old_value;
    if (!plug.isNull()) {
        MTime new_value_time = plug.asMTime(&status);
        CHECK_MSTATUS(status);
        float new_value =
            static_cast<float>(new_value_time.asUnits(MTime::uiUnit()));
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get boolean attribute value.
std::tuple<bool, bool>
get_plug_value_bool(MPlug plug, bool old_value) {
    MStatus status;
    bool has_changed = false;
    bool value = old_value;
    if (!plug.isNull()) {
        bool new_value = plug.asBool(&status);
        CHECK_MSTATUS(status);
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get unsigned integer attribute value.
std::tuple<uint32_t, bool>
get_plug_value_uint32(MPlug plug, uint32_t old_value) {
    MStatus status;
    bool has_changed = false;
    uint32_t value = old_value;
    if (!plug.isNull()) {
        uint32_t new_value = plug.asInt(&status);
        CHECK_MSTATUS(status);
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get floating point attribute value.
std::tuple<float, bool> get_plug_value_float(MPlug plug, float old_value) {
    MStatus status;
    bool has_changed = false;
    float value = old_value;
    if (!plug.isNull()) {
        float new_value = plug.asFloat(&status);
        CHECK_MSTATUS(status);
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get String attribute value.
std::tuple<MString, bool>
get_plug_value_string(MPlug plug, MString old_value) {
    MStatus status;
    bool has_changed = false;
    MString value = old_value;
    if (!plug.isNull()) {
        MString new_value = plug.asString();
        has_changed = old_value != new_value;
        if (has_changed) {
            value = new_value;
        }
    }
    return std::make_tuple(value, has_changed);
}

// Get the ocgStreamData type from the given plug.
std::tuple<ocg::Node, bool>
get_plug_value_stream(MPlug &plug, ocg::Node old_value) {
    MStatus status;
    auto log = log::get_logger();

    auto shared_graph = get_shared_graph();
    ocg::Node value = old_value;
    ocg::Node new_value = ocg::Node(ocg::NodeType::kNull, 0);
    status = open_comp_graph_maya::utils::get_plug_ocg_stream_value(
        plug, shared_graph, new_value);
    if (status != MS::kSuccess) {
        new_value = ocg::Node(ocg::NodeType::kNull, 0);
    }

    bool has_changed =
        (shared_graph->state() != ocg::GraphState::kClean)
        || (old_value.get_id() != new_value.get_id());
    if (has_changed) {
        value = new_value;
    }

    return std::make_tuple(value, has_changed);
}

} // namespace utils
} // namespace image_plane
} // namespace open_comp_graph_maya
