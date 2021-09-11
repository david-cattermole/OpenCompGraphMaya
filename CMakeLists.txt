# Copyright (C) 2020 David Cattermole.
#
# This file is part of OpenCompGraphMaya.
#
# OpenCompGraphMaya is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# OpenCompGraphMaya is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCompGraphMaya.  If not, see <https://www.gnu.org/licenses/>.
# ---------------------------------------------------------------------
#
# OpenCompGraphMaya build script.
#
cmake_minimum_required(VERSION 3.0.0)

# Project configuration.
project(OpenCompGraphMaya
        VERSION 0.1.0)

set(PROJECT_HOMEPAGE_URL "https://github.com/david-cattermole/OpenCompGraphMaya")
set(PROJECT_DESCRIPTION "Compositing Graph nodes inside Autodesk Maya.")
set(PROJECT_AUTHOR "David Cattermole and others (see AUTHORS.txt file)")
set(PROJECT_COPYRIGHT "2020, 2021, David Cattermole.")

# Add custom modules to the module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Maya SDK
set(MAYA_VERSION "2018" CACHE STRING "Maya version number")
set(MAYA_LOCATION "/usr/autodesk/maya${MAYA_VERSION}/" CACHE PATH "Maya install directory")
set(DEVKIT_LOCATION "/usr/autodesk/maya${MAYA_VERSION}/" CACHE PATH "Maya development kit (devkit) directory")
set(MAYA_INCLUDE_PATH "/usr/autodesk/maya${MAYA_VERSION}/include" CACHE PATH "Maya include directory")
set(MAYA_LIB_PATH "/usr/autodesk/maya${MAYA_VERSION}/lib" CACHE PATH "Maya library directory")

add_subdirectory(modules)
add_subdirectory(mel)
add_subdirectory(python)
add_subdirectory(shader)
add_subdirectory(include)
add_subdirectory(src)

# Install misc files
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
  ${CMAKE_CURRENT_SOURCE_DIR}/INSTALL.md
  DESTINATION "${MODULE_FULL_NAME}/")

# Package Configuration.
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_AUTHOR}")
set(CPACK_PACKAGE_CONTACT "cattermole91@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "${MODULE_FULL_NAME}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
# Put the packages into the source directory, so that when we run
# another build, it's not removed as part of the clean up.
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/packages")
if (WIN32 AND NOT UNIX)
  # .zip file for Windows.
  set(CPACK_GENERATOR "ZIP")
elseif (UNIX AND NOT WIN32)
  # .tar.gz for Linux and MacOS
  set(CPACK_GENERATOR "TGZ")
endif ()

# Use CPack (part of CMake) to generate the packages...
# If 'CPack' is not included, the 'packages' target will not be generated.
include(CPack)
