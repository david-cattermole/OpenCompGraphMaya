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
 * Store OCG preferences for a scene file.
 *
 * The preferences should only be edited using the ocgPreferenes
 * command, the attribute values on this node should not be modified
 * directly.
 *
 * Store:
 * - Is the cache enabled/disabled.
 * - Where should disk-cache files be searched?
 * - Color Space
 *   - Use Maya Color Management (bool)
 *   - Default 8-bit color space (string)
 *   - Default 16-bit color space (string)
 *   - Default log color space (string)
 *   - Default 32-bit color space (string)
 *   - OpenColorIO path (string)
 */

// Maya
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MUuid.h>

// STL
#include <cstring>
#include <cmath>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "logger.h"
#include "graph_data.h"
#include "preferences_node.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

MTypeId PreferencesNode::m_id(OCGM_PREFERENCES_TYPE_ID);

// Attributes
MObject PreferencesNode::m_color_space_name_linear_attr;
MObject PreferencesNode::m_ocio_path_enable_attr;
MObject PreferencesNode::m_ocio_path_attr;
MObject PreferencesNode::m_mem_cache_enable_attr;
MObject PreferencesNode::m_mem_cache_size_attr;
MObject PreferencesNode::m_disk_cache_base_dir_attr;

PreferencesNode::PreferencesNode() {}

PreferencesNode::~PreferencesNode() {}

MString PreferencesNode::nodeName() {
    return MString(OCGM_PREFERENCES_TYPE_NAME);
}

MStatus PreferencesNode::compute(const MPlug &plug, MDataBlock &data) {
    return MS::kUnknownParameter;
}

void *PreferencesNode::creator() {
    return (new PreferencesNode());
}

MStatus PreferencesNode::initialize() {
    MStatus status;
    MFnUnitAttribute    uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute   tAttr;

    // TODO: Add more color space attributes.
    //
    //  - Use Maya Color Management (bool)
    //  - Default 8-bit color space (string)
    //  - Default 16-bit color space (string)
    //  - Default log color space (string)
    //  - Default 32-bit color space (string)
    //  - OpenColorIO path (string)
    //

    // Internal Color Space Name
    MFnStringData color_space_string_data;
    MObject color_space_string_data_obj = color_space_string_data.create("Linear");
    m_color_space_name_linear_attr = tAttr.create(
            "colorSpaceNameLinear", "clspcnmlin",
            MFnData::kString, color_space_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(false));

    // OpenColorIO Path Enable
    m_ocio_path_enable_attr = nAttr.create(
            "ocioPathEnable", "ociopthenb",
            MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // OpenColorIO path (string)
    MFnStringData ocio_path_string_data;
    MObject ocio_path_string_data_obj = ocio_path_string_data.create("${OCIO}");
    m_ocio_path_attr = tAttr.create(
            "ocioPath", "ociopth",
            MFnData::kString, ocio_path_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(true));

    // Memory Cache Enable
    m_mem_cache_enable_attr = nAttr.create(
            "memoryCacheEnable", "cchenb",
            MFnNumericData::kBoolean, true);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // Memory Cache Size
    //
    // TODO: Get the maximum cache size from the current
    // system. Ensure users cannot make the cache larger than the
    // maximum RAM available, and the number should be 90% of the
    // maximum, roughly. We don't want users to use swap-space,
    // because it would be a frustrating user-experience.
    //
    double mem_cache_size_min = 0.0;
    double mem_cache_size_soft_min = mem_cache_size_min;
    double mem_cache_size_soft_max = 32.0;
    double mem_cache_size_default = 1.0;
    m_mem_cache_size_attr = nAttr.create(
        "memoryCacheSizeGigabytes", "cchszgb",
        MFnNumericData::kDouble, mem_cache_size_default);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(mem_cache_size_min));
    CHECK_MSTATUS(nAttr.setSoftMin(mem_cache_size_soft_min));
    CHECK_MSTATUS(nAttr.setSoftMax(mem_cache_size_soft_max));

    // Display Cache Diagnostics:
    //
    //   - Amount of RAM in use.
    //   - Amount of RAM available.
    //   - Cache hits.
    //   - Cache misses.
    //   - Evictions
    //   - Insertions
    //   - Number of images.

    // Disk Cache Base Directory.
    //
    MFnStringData base_dir_string_data;
    MObject base_dir_string_data_obj = base_dir_string_data.create("${TEMP}");
    m_disk_cache_base_dir_attr = tAttr.create(
            "diskCacheBaseDir", "dskcchbsdr",
            MFnData::kString, base_dir_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(true));

    // Add Attributes
    CHECK_MSTATUS(MPxNode::addAttribute(m_color_space_name_linear_attr));
    CHECK_MSTATUS(MPxNode::addAttribute(m_ocio_path_enable_attr));
    CHECK_MSTATUS(MPxNode::addAttribute(m_ocio_path_attr));
    CHECK_MSTATUS(MPxNode::addAttribute(m_mem_cache_enable_attr));
    CHECK_MSTATUS(MPxNode::addAttribute(m_mem_cache_size_attr));
    CHECK_MSTATUS(MPxNode::addAttribute(m_disk_cache_base_dir_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
