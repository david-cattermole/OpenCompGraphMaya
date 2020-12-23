@ECHO OFF
SETLOCAL
::
:: Copyright (C) 2020 David Cattermole.
::
:: This file is part of OpenCompGraphMaya.
::
:: OpenCompGraphMaya is free software: you can redistribute it and/or modify it
:: under the terms of the GNU Lesser General Public License as
:: published by the Free Software Foundation, either version 3 of the
:: License, or (at your option) any later version.
::
:: OpenCompGraphMaya is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU Lesser General Public License for more details.
::
:: You should have received a copy of the GNU Lesser General Public License
:: along with OpenCompGraphMaya.  If not, see <https://www.gnu.org/licenses/>.
:: ---------------------------------------------------------------------
::
:: Builds the OpenCompGraphMaya project.

:: Maya directories
::
:: If you're not using Maya 2018 or have a non-standard install location,
:: set these variables here.
::
:: Note: Do not enclose the MAYA_VERSION in quotes, it will
::       lead to tears.
SET MAYA_VERSION=2018
SET MAYA_LOCATION="C:\Program Files\Autodesk\Maya2018"

:: Clear all build information before re-compiling.
:: Turn this off when wanting to make small changes and recompile.
SET FRESH_BUILD=1

:: Where to install the module?
::
:: Note: In Windows 8 and 10, "My Documents" is no longer visible,
::       however files copying to "My Documents" automatically go
::       to the "Documents" directory.
::
:: The "$HOME/maya/2018/modules" directory is automatically searched
:: for Maya module (.mod) files. Therefore we can install directly.
::
SET INSTALL_MODULE_DIR="%USERPROFILE%\My Documents\maya\%MAYA_VERSION%\modules"

:: Build ZIP Package.
:: For developer use. Make ZIP packages ready to distribute to others.
SET BUILD_PACKAGE=1


:: Do not edit below, unless you know what you're doing.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: What type of build? "Release" or "Debug"?
SET BUILD_TYPE=Release

:: To Generate a Visual Studio 'Solution' file, change the '0' to a '1'.
SET GENERATE_SOLUTION=0

:: The root of this project.
SET PROJECT_ROOT=%CD%
ECHO OpenCompGraphMaya Root: %PROJECT_ROOT%


rem :: The root of the OpenCompGraph project.
rem SET OCG_ROOT=%PROJECT_ROOT%\src\OpenCompGraph\
rem ECHO OpenCompGroup Root: %OCG_ROOT%
rem CHDIR %OCG_ROOT%

rem :: Install directory
rem SET INSTALL_DIR="%OCG_ROOT%\install"

rem :: Where to find the Rust libraries and headers.
rem SET RUST_BUILD_DIR="%OCG_ROOT%\target\release"
rem SET RUST_INCLUDE_DIR="%OCG_ROOT%\include"

rem :: Build Rust
rem ::
rem :: Assumes 'cxxbridge-cmd' and 'cbindgen' is installed.
rem cxxbridge --header --output "%OCG_ROOT%\include\rust\cxx.h"
rem cbindgen --config cbindgen.toml ^
rem          --crate opencompgraph ^
rem          --output "%OCG_ROOT%\include\opencompgraph\_cbindgen.h"
rem cargo build --release

rem :: Return back project root directory.
rem CHDIR %PROJECT_ROOT%


:: Build plugin
MKDIR build_windows64_maya%MAYA_VERSION%_%BUILD_TYPE%
CHDIR build_windows64_maya%MAYA_VERSION%_%BUILD_TYPE%
IF "%FRESH_BUILD%"=="1" (
    DEL /S /Q *
    FOR /D %%G in ("*") DO RMDIR /S /Q "%%~nxG"
)

IF "%GENERATE_SOLUTION%"=="1" (

REM To Generate a Visual Studio 'Solution' file
    cmake -G "Visual Studio 14 2015 Win64" -T "v140" ^
        -DRUST_BUILD_DIR=%RUST_BUILD_DIR% ^
        -DRUST_INCLUDE_DIR=%RUST_INCLUDE_DIR% ^
        -DMAYA_VERSION=%MAYA_VERSION% ^
        -DMAYA_LOCATION=%MAYA_LOCATION% ^
        -DMAYA_VERSION=%MAYA_VERSION% ^
        ..

) ELSE (

    cmake -G "NMake Makefiles" ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
        -DCMAKE_INSTALL_PREFIX=%INSTALL_MODULE_DIR% ^
        -DRUST_BUILD_DIR=%RUST_BUILD_DIR% ^
        -DRUST_INCLUDE_DIR=%RUST_INCLUDE_DIR% ^
        -DMAYA_LOCATION=%MAYA_LOCATION% ^
        -DMAYA_VERSION=%MAYA_VERSION% ^
        ..

    nmake /F Makefile clean
    nmake /F Makefile all

REM Comment this line out to stop the automatic install into the home directory.
    nmake /F Makefile install

REM Run tests
    IF "%RUN_TESTS%"=="1" (
        nmake /F Makefile test
    )

REM Create a .zip package.
IF "%BUILD_PACKAGE%"=="1" (
       nmake /F Makefile package
   )

)

:: Return back project root directory.
CHDIR "%PROJECT_ROOT%"
