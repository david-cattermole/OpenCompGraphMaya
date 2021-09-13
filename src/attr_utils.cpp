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
 * Attribute utilities.
 */

// Maya
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>

#include "attr_utils.h"

namespace open_comp_graph_maya {
namespace utils {

MStatus create_node_disk_cache_attributes(
        MObject &enable_attr,
        MObject &file_path_attr) {
    MStatus status = MS::kFailure;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    // Enable
    enable_attr = nAttr.create(
        "diskCacheEnable", "dskchenb",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // File Path
    MFnStringData empty_string_data;
    MObject empty_string_data_obj = empty_string_data.create("");
    file_path_attr = tAttr.create(
            "diskCacheFilePath", "dskchflpth",
            MFnData::kString, empty_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(true));

    return MS::kSuccess;
}

} // namespace utils
} // namespace open_comp_graph_maya
