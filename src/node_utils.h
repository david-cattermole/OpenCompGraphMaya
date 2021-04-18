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
 * Node utilities.
 */

// Maya
#include <maya/MObject.h>
#include <maya/MDataBlock.h>
#include <maya/MString.h>

namespace open_comp_graph_maya {
namespace utils {

bool get_attr_value_bool(MDataBlock &data_block, MObject &attr);

int16_t get_attr_value_short(MDataBlock &data_block, MObject &attr);

int32_t get_attr_value_int(MDataBlock &data_block, MObject &attr);

float get_attr_value_float(MDataBlock &data_block, MObject &attr);

MString get_attr_value_string(MDataBlock &data_block, MObject &attr);

} // namespace utils
} // namespace open_comp_graph_maya
