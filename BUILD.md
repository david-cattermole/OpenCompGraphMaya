# Building

This document is a work in progress and is missing a lot of details and examples.
It is provided for basic information and will be finished at a later date.

On Windows (in a Visual Studio 20XX "Cross Tools" Command Prompt):
```
$ cd path/to/folder
$ git clone --recursive https://github.com/david-cattermole/OpenCompGraphMaya.git
$ cd OpenCompGraphMaya
$ build_OpenCompGraphMaya_windows64_maya2018.bat
```

# Dependencies

* [CMake 3.0+](https://cmake.org/)
* C++11 Compiler (see [Maya Development Build Environment](#maya-development-build-environment))
* [Rust 1.48.0 or higher](https://www.rust-lang.org/) (stable-branch)
  * [cxx 1.0.x](https://github.com/dtolnay/cxx)
* Maya 2018+ Development Kit (devkit)
* [spdlog 1.8.2](https://github.com/gabime/spdlog)

# Maya Development Build Environment

* Linux build environment for...
  * [Maya 2018](https://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__files_Setting_up_your_build_environment_Linux_environments_32bit_and_64bit_htm)
  * [Maya 2019](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=__developer_Maya_SDK_MERGED_Setting_up_your_build_Linux_environment_html)
  * [Maya 2020](https://help.autodesk.com/view/MAYAUL/2020/ENU/?guid=__developer_Maya_SDK_MERGED_Setting_up_your_build_Linux_environment_html)
* MacOS build environment for...
  * [Maya 2018](https://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__files_Setting_up_your_build_environment_Mac_OS_X_environment_htm)
  * [Maya 2019](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=__developer_Maya_SDK_MERGED_Setting_up_your_build_Mac_OS_X_environment_html)
  * [Maya 2020](https://help.autodesk.com/view/MAYAUL/2020/ENU/?guid=__developer_Maya_SDK_MERGED_Setting_up_your_build_Mac_OS_X_environment_html)
* Windows (64-bit) build environment for...
  * [Maya 2018](https://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__files_Setting_up_your_build_env_Windows_env_32bit_and_64bit_htm)
  * [Maya 2019](https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=__developer_Maya_SDK_MERGED_Setting_up_your_build_Windows_environment_64_bit_html)
  * [Maya 2020](https://help.autodesk.com/view/MAYAUL/2020/ENU/?guid=__developer_Maya_SDK_MERGED_Setting_up_your_build_Windows_environment_64_bit_html)
