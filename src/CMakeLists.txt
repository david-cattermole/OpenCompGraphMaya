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
# Building the C++ code for OpenCompGraphMaya.
#

set(OpenCompGraph_INSTALL_PREFIX "${MODULE_FULL_NAME}/" CACHE STRING "")
add_subdirectory(OpenCompGraph)

# Source
set(SOURCE_FILES
  image_plane_shape.cpp
  image_plane_sub_scene_override.cpp
  plugin_main.cpp
  )

# Find external packages
find_package(Maya REQUIRED)

# # Treat warnings as errors.
# if(MSVC)
#   add_compile_options(/W4 /WX)
# else()
#   add_compile_options(-W -Wall -Werror -Wpedantic)
# endif()

# Compile Flags.
#
# Release flags come from the Autodesk Maya build scripts (and
# Visual Studio project files).
if (MSVC)
  # For Visual Studio 11 2012
  set(CMAKE_CXX_FLAGS "")  # Zero out the C++ flags, we have complete control.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS /Zc:wchar_t /Zi /fp:precise /Zc:forScope /GR /Gd /EHsc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OSWin_\" /D \"WIN32\" /D \"_WINDOWS\" /D \"_USRDLL\" /D \"NT_PLUGIN\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"_HAS_ITERATOR_DEBUGGING=0\" /D \"_SECURE_SCL=0\" /D \"_SECURE_SCL_THROWS=0\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"_SECURE_SCL_DEPRECATE=0\" /D \"_CRT_SECURE_NO_DEPRECATE\" /D \"TBB_USE_DEBUG=0\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"__TBB_LIB_NAME=tbb.lib\" /D \"Bits64_\" /D \"_WINDLL\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"NT_PLUGIN\" /D \"REQUIRE_IOSTREAM\"")

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /D \"_DEBUG\"")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Gm /Od /RTC1")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Ob0 /GR /GL /Oi /Gy /Zi /EHsc")

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /D \"NDEBUG\"")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /Gy /Gm- /O2 /Ob1 /GF")

  # Must add the plug-in entry/exit points otherwise
  # the plug-in won't load.
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /export:initializePlugin /export:uninitializePlugin")
elseif (APPLE)

  # For MacOS with Clang (which is the supported compiler for Maya
  # 2018+)
  set(CMAKE_CXX_FLAGS "")  # Zero out the C++ flags, we have complete control.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x  -stdlib=libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-multichar -Wno-comment -Wno-sign-compare")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-reorder")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth-35 -fno-gnu-keywords")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funsigned-char -fpascal-strings") #  -pthread
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCC_GNU_ -DOSMac_ -DOSMacOSX_ -DOSMac_MachO_ -D_LANGUAGE_C_PLUS_PLUS -mmacosx-version-min=10.8")

  # Special MacOS linking stuff
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -headerpad_max_install_names")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework System")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework SystemConfiguration")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework CoreServices")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework Carbon")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework Cocoa")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework ApplicationServices")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework IOKit")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -bundle")

  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fPIC -fno-strict-aliasing -m64")
else ()
  # For Linux with GCC
  set(CMAKE_CXX_FLAGS "")  # Zero out the C++ flags, we have complete control.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-multichar -Wno-comment -Wno-sign-compare")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funsigned-char -pthread -fopenmp")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBits64_ -DUNIX -D_BOOL -DLINUX -DFUNCPROTO -D_GNU_SOURCE -DLINUX_64 -DREQUIRE_IOSTREAM")
  # '-ftemplate-depth-27', rather than '25' is required to compile under GCC 4.8.5.
  # '-ftemplate-depth-35', rather than '25' is required to compile under GCC 5.5.x.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-reorder -ftemplate-depth-35 -fno-gnu-keywords")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fPIC -fno-strict-aliasing -m64")
endif ()


# 'OpenCompGraphMaya' maya plugin library
if (APPLE)
  add_library(OpenCompGraphMaya MODULE ${SOURCE_FILES})
else ()
  add_library(OpenCompGraphMaya SHARED ${SOURCE_FILES})
endif ()
target_include_directories(OpenCompGraphMaya
  PRIVATE ../include
  PRIVATE .
  PUBLIC ${MAYA_INCLUDE_DIRS}
  )
target_link_libraries(OpenCompGraphMaya
  PUBLIC
  ${MAYA_OpenMaya_LIBRARY}
  ${MAYA_OpenMayaUI_LIBRARY}
  ${MAYA_OpenMayaRender_LIBRARY}
  ${MAYA_Foundation_LIBRARY}
  opencompgraph  # Target the OpenCompGraph git submodule.
  )

# On Linux the 'm' library is required.
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_link_libraries(OpenCompGraphMaya PUBLIC m)
endif ()

if (MSVC)
  target_link_libraries(OpenCompGraphMaya PUBLIC opengl32)
endif ()

# no 'lib' prefix to Maya plugin .so files
set_target_properties(OpenCompGraphMaya PROPERTIES PREFIX "")

# On Linux, the Maya Plug-In file extension is '.so'.
# On MacOS, the Maya Plug-In file extension is '.bundle'.
# On Windows, the Maya Plug-In file extension is '.mll', not '.dll'.
set(MAYA_PLUGIN_SUFFIX ".so")
if (APPLE)
  set(MAYA_PLUGIN_SUFFIX ".bundle")
elseif (WIN32)
  set(MAYA_PLUGIN_SUFFIX ".mll")
else()
  message(FATAL_ERROR "Only Linux, MacOS and Windows are supported.")
endif ()
set_target_properties(OpenCompGraphMaya PROPERTIES SUFFIX ${MAYA_PLUGIN_SUFFIX})

# Output to the Module plug-ins directory.
#
# On Windows, the Plug-In is treated as a 'RUNTIME' type,
# on Linux, it's a 'LIBRARY' type.
set_target_properties(OpenCompGraphMaya PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${MODULE_FULL_NAME}"
  LIBRARY_OUTPUT_DIRECTORY "${MODULE_FULL_NAME}"
  ARCHIVE_OUTPUT_DIRECTORY "${MODULE_FULL_NAME}")

# Install the Plug-In.
install(TARGETS OpenCompGraphMaya
  RUNTIME DESTINATION "${MODULE_FULL_NAME}/plug-ins"
  LIBRARY DESTINATION "${MODULE_FULL_NAME}/plug-ins")
