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
 *
 */

#include <spdlog/logger.h>

namespace open_comp_graph_maya {
namespace log {

// Start up all logging needed for OCG Maya.
void initialize();

void deinitialize();

void set_level(const char *level_name);

std::shared_ptr<spdlog::logger> get_logger();

} // namespace log
} // namespace open_comp_graph_maya
