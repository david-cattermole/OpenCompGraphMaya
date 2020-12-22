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
 * Main Maya plugin entry point.
 */

#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MPxTransform.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MDrawRegistry.h>

#include <OpenCompGraphMaya/buildConstant.h>  // Build-Time constant values.
#include <OpenCompGraphMaya/nodeTypeIds.h>
#include <imagePlane/shader.h>
#include <imagePlane/shape.h>
#include <imagePlane/subSceneOverride.h>

namespace ocgm = open_comp_graph_maya;

#undef PLUGIN_COMPANY  // Maya API defines this, we override it.
#define PLUGIN_COMPANY PROJECT_NAME
#define PLUGIN_VERSION PROJECT_VERSION

// Register the plug-in with Maya.
MStatus initializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, PLUGIN_VERSION, "Any");

    status = plugin.registerNode(
        ocgm::ImagePlaneShape::nodeName(),
        ocgm::ImagePlaneShape::m_id,
        &ocgm::ImagePlaneShape::creator,
        &ocgm::ImagePlaneShape::initialize,
        MPxNode::kLocatorNode,
        &ocgm::ImagePlaneShape::drawDbClassification);
    if (!status) {
        status.perror("registerNode");
        return status;
    }

    status = MHWRender::MDrawRegistry::registerSubSceneOverrideCreator(
        ocgm::ImagePlaneShape::drawDbClassification,
        ocgm::ImagePlaneShape::drawRegistrantId,
        ocgm::ImagePlaneSubSceneOverride::Creator);
    if (!status) {
        status.perror("registerSubSceneOverrideCreator");
        return status;
    }

    // Register a custom selection mask with priority 2 (same as locators by default).
    MSelectionMask::registerSelectionType(ocgm::ImagePlaneShape::selectionTypeName, 2);
    MString mel_cmd = "selectType -byName \"";
    mel_cmd += ocgm::ImagePlaneShape::selectionTypeName;
    mel_cmd += "\" 1";
    status = MGlobal::executeCommand(mel_cmd);

    return status;
}

// Deregister the plug-in from Maya.
MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj);

    status = MHWRender::MDrawRegistry::deregisterSubSceneOverrideCreator(
        ocgm::ImagePlaneShape::drawDbClassification,
        ocgm::ImagePlaneShape::drawRegistrantId);
    if (!status) {
        status.perror("deregisterSubSceneOverrideCreator");
        return status;
    }

    // status = ocgm::releaseShaders();
    status = ocgm::releaseImagePlaneShader();
    if (!status) {
        status.perror("releaseShaders");
        return status;
    }

    status = plugin.deregisterNode(ocgm::ImagePlaneShape::m_id);
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }

    // Deregister custom selection mask
    MSelectionMask::deregisterSelectionType(ocgm::ImagePlaneShape::selectionTypeName);
    return status;
}
