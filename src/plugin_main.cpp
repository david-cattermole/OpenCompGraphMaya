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
 * Main Maya plugin entry point.
 */

#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MPxTransform.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MDrawRegistry.h>

// OCG
#include <opencompgraph.h>

// OCG Maya
#include <opencompgraphmaya/build_constants.h>  // Build-Time constant values.
#include <opencompgraphmaya/node_type_ids.h>
#include <image_plane/image_plane_shape.h>
#if OCG_USE_SUB_SCENE_OVERRIDE == 1
    #include <image_plane/image_plane_sub_scene_override.h>
#else
    #include <image_plane/image_plane_geometry_override.h>
#endif
#include <comp_nodes/color_grade_node.h>
#include <comp_nodes/image_read_node.h>
#include <comp_nodes/image_merge_node.h>
#include <comp_nodes/lens_distort_node.h>
#include <comp_nodes/image_transform_node.h>
#include <comp_nodes/image_crop_node.h>
#include <preferences_node.h>
#include <graph_data.h>
#include "global_cache.h"
#include "logger.h"

namespace ocg = open_comp_graph;
namespace ocgm = open_comp_graph_maya;

#define REGISTER_NODE(plugin, name, id, creator, initialize, stat) \
    stat = plugin.registerNode(name, id, creator, initialize);     \
    if (!stat) {                                                   \
        stat.perror(MString(name) + ": registerNode");             \
        return (stat);                                             \
    }

#define REGISTER_DATA(plugin, name,                     \
                      id, creator,                      \
                      stat)                             \
    stat = plugin.registerData(name,                    \
                               id, creator);            \
    if (!stat) {                                        \
        stat.perror(MString(name) + ": registerData");  \
        return (stat);                                  \
    }

#define DEREGISTER_NODE(plugin, name, id, stat)          \
    stat = plugin.deregisterNode(id);                    \
    if (!stat) {                                         \
        stat.perror(MString(name) + ": deregisterNode"); \
        return (stat);                                   \
    }

#define DEREGISTER_DATA(plugin, name, id, stat)             \
    stat = plugin.deregisterData(id);                       \
    if (!stat) {                                            \
        stat.perror(MString(name) + ": deregisterData");    \
        return (stat);                                      \
    }

#undef PLUGIN_COMPANY  // Maya API defines this, we override it.
#define PLUGIN_COMPANY PROJECT_NAME
#define PLUGIN_VERSION PROJECT_VERSION

// Register the plug-in with Maya.
MStatus initializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, PLUGIN_VERSION, "Any");

    // Initialize both plug-in and core library loggers.
    ocg::log::initialize();
    ocgm::log::initialize();
    // TODO: Parse environment variables and pass the log level.
    ocgm::log::set_level("warn");
    auto log = ocgm::log::get_logger();
    log->info("Initializing OpenCompGraphMaya plug-in...");

    // Initial size of the cache, when the user loads the plug-in.
    auto shared_cache = ocgm::cache::get_shared_cache();
    auto shared_color_tfm_cache = ocgm::cache::get_shared_color_transform_cache();
    const size_t bytes_to_gigabytes = 1073741824;
    // Set RAM used for the cache.
    //
    // TODO: Allow user to change this value.
    // TODO: Use environment variable to configure the default, if given.
    // TODO: When a scene is closed, the Cache should automatically flush.
    shared_cache->set_capacity_bytes(20 * bytes_to_gigabytes);  // 20GB of RAM
    shared_color_tfm_cache->set_capacity_bytes(0.1 * bytes_to_gigabytes);  // 100MB of RAM

    // Register data types first, so the nodes and commands below can
    // reference them.
    REGISTER_DATA(
        plugin,
        ocgm::GraphData::typeName(),
        ocgm::GraphData::m_id,
        ocgm::GraphData::creator,
        status);

    REGISTER_NODE(plugin,
                  ocgm::PreferencesNode::nodeName(),
                  ocgm::PreferencesNode::m_id,
                  ocgm::PreferencesNode::creator,
                  ocgm::PreferencesNode::initialize,
                  status);

    REGISTER_NODE(plugin,
                  ocgm::ImageReadNode::nodeName(),
                  ocgm::ImageReadNode::m_id,
                  ocgm::ImageReadNode::creator,
                  ocgm::ImageReadNode::initialize,
                  status);

    REGISTER_NODE(plugin,
                  ocgm::ImageMergeNode::nodeName(),
                  ocgm::ImageMergeNode::m_id,
                  ocgm::ImageMergeNode::creator,
                  ocgm::ImageMergeNode::initialize,
                  status);

    REGISTER_NODE(plugin,
                  ocgm::ImageCropNode::nodeName(),
                  ocgm::ImageCropNode::m_id,
                  ocgm::ImageCropNode::creator,
                  ocgm::ImageCropNode::initialize,
                  status);

    REGISTER_NODE(plugin,
                  ocgm::ImageTransformNode::nodeName(),
                  ocgm::ImageTransformNode::m_id,
                  ocgm::ImageTransformNode::creator,
                  ocgm::ImageTransformNode::initialize,
                  status);

    REGISTER_NODE(plugin,
                  ocgm::ColorGradeNode::nodeName(),
                  ocgm::ColorGradeNode::m_id,
                  ocgm::ColorGradeNode::creator,
                  ocgm::ColorGradeNode::initialize,
                  status);

    REGISTER_NODE(plugin,
                  ocgm::LensDistortNode::nodeName(),
                  ocgm::LensDistortNode::m_id,
                  ocgm::LensDistortNode::creator,
                  ocgm::LensDistortNode::initialize,
                  status);

    // Image Plane Shape Node
    status = plugin.registerNode(
        ocgm::image_plane::ShapeNode::nodeName(),
        ocgm::image_plane::ShapeNode::m_id,
        &ocgm::image_plane::ShapeNode::creator,
        &ocgm::image_plane::ShapeNode::initialize,
        MPxNode::kLocatorNode,
        &ocgm::image_plane::ShapeNode::m_draw_db_classification);
    if (!status) {
        status.perror("registerNode");
        return status;
    }

    // Image Plane (Viewport 2.0)
#if OCG_USE_SUB_SCENE_OVERRIDE == 1
    status = MHWRender::MDrawRegistry::registerSubSceneOverrideCreator(
        ocgm::image_plane::ShapeNode::m_draw_db_classification,
        ocgm::image_plane::ShapeNode::m_draw_registrant_id,
        ocgm::image_plane::SubSceneOverride::Creator);
    if (!status) {
        status.perror("registerSubSceneOverrideCreator");
        return status;
    }
#else
    status = MHWRender::MDrawRegistry::registerGeometryOverrideCreator(
        ocgm::image_plane::ShapeNode::m_draw_db_classification,
        ocgm::image_plane::ShapeNode::m_draw_registrant_id,
        ocgm::image_plane::GeometryOverride::Creator);
    if (!status) {
        status.perror("registerGeometryOverrideCreator");
        return status;
    }
#endif

    // Register a custom selection mask with priority 2 (same as
    // locators by default).
    MSelectionMask::registerSelectionType(
        ocgm::image_plane::ShapeNode::m_selection_type_name, 2);
    MString mel_cmd = "selectType -byName \"";
    mel_cmd += ocgm::image_plane::ShapeNode::m_selection_type_name;
    mel_cmd += "\" 1";
    status = MGlobal::executeCommand(mel_cmd);

    // Register plugin display filter.
    // The filter is registered in both interactive and batch mode (Hardware 2.0)
    plugin.registerDisplayFilter(
        ocgm::image_plane::ShapeNode::m_display_filter_name,
        ocgm::image_plane::ShapeNode::m_display_filter_label,
        ocgm::image_plane::ShapeNode::m_draw_db_classification);

    return status;
}

// Deregister the plug-in from Maya.
MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj);

    // Deregister plugin display filter
    const MString displayFilterLabel("ocgImagePlaneDisplayFilter");
    plugin.deregisterDisplayFilter(displayFilterLabel);

    // Viewport 2.0 override.
#if OCG_USE_SUB_SCENE_OVERRIDE == 1
    status = MHWRender::MDrawRegistry::deregisterSubSceneOverrideCreator(
        ocgm::image_plane::ShapeNode::m_draw_db_classification,
        ocgm::image_plane::ShapeNode::m_draw_registrant_id);
    if (!status) {
        status.perror("deregisterSubSceneOverrideCreator");
        return status;
    }
#else
    status = MHWRender::MDrawRegistry::deregisterGeometryOverrideCreator(
        ocgm::image_plane::ShapeNode::m_draw_db_classification,
        ocgm::image_plane::ShapeNode::m_draw_registrant_id);
    if (!status) {
        status.perror("deregisterGeometryOverrideCreator");
        return status;
    }
#endif

    status = plugin.deregisterNode(ocgm::image_plane::ShapeNode::m_id);
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }

    // Deregister custom selection mask
    MSelectionMask::deregisterSelectionType(
        ocgm::image_plane::ShapeNode::m_selection_type_name);

    DEREGISTER_NODE(plugin, ocgm::PreferencesNode::nodeName(),
                    ocgm::PreferencesNode::m_id, status);

    DEREGISTER_NODE(plugin, ocgm::ImageReadNode::nodeName(),
                    ocgm::ImageReadNode::m_id, status);

    DEREGISTER_NODE(plugin, ocgm::ImageMergeNode::nodeName(),
                    ocgm::ImageMergeNode::m_id, status);

    DEREGISTER_NODE(plugin, ocgm::ImageCropNode::nodeName(),
                    ocgm::ImageCropNode::m_id, status);

    DEREGISTER_NODE(plugin, ocgm::ImageTransformNode::nodeName(),
                    ocgm::ImageTransformNode::m_id, status);

    DEREGISTER_NODE(plugin, ocgm::ColorGradeNode::nodeName(),
                    ocgm::ColorGradeNode::m_id, status);

    DEREGISTER_NODE(plugin, ocgm::LensDistortNode::nodeName(),
                    ocgm::LensDistortNode::m_id, status);

    // Unloaded last, so that all nodes needing it are unloaded first
    // and we won't get a potential crash.
    DEREGISTER_DATA(plugin,
                    ocgm::GraphData::typeName(),
                    ocgm::GraphData::m_id, status);

    ocgm::log::deinitialize();
    return status;
}
