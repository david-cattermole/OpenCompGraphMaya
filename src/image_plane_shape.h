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
 * Image plane shape node.
 */

#ifndef OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H
#define OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H

// Maya
#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MDistance.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MFnDagNode.h>
#include <maya/MSelectionContext.h>
#include <maya/MDagMessage.h>

// Maya Viewport 2.0
#include <maya/MDrawRegistry.h>
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

namespace open_comp_graph_maya{

// Shape Data
static const float shape_vertices[6][3] = {
    // First Triangle - counter clockwise direction.
    {-1.0, -1.0, 0.0}, // Lower left
    {1.0, 1.0, 0.0},   // Upper right
    {-1.0, 1.0, 0.0},  // Upper left

    // Second Triangle - counter clockwise direction.
    {-1.0, -1.0, 0.0}, // Lower left
    {1.0, -1.0, 0.0},  // Lower Right
    {1.0, 1.0, 0.0}};  // Upper right
static const int shape_vertices_count = 6;

static const float shape_uvs[6][2] = {
   // First Triangle - counter clockwise direction.
   {0.0, 0.0}, // Lower left
   {1.0, 1.0}, // Upper right
   {0.0, 1.0}, // Upper left

   // Second Triangle - counter clockwise direction.
   {0.0, 0.0},  // Lower left
   {1.0, 0.0},  // Lower Right
   {1.0, 1.0}}; // Upper right
static const int shape_uvs_count = 6;

class ImagePlaneShape : public MPxLocatorNode {
public:
    ImagePlaneShape();
    ~ImagePlaneShape() override;

    MStatus compute(const MPlug &plug, MDataBlock &data) override;

    // void draw(M3dView &view, const MDagPath &path,
    //           M3dView::DisplayStyle style,
    //           M3dView::DisplayStatus status) override;

    bool isBounded() const override;
    MBoundingBox boundingBox() const override;
    MSelectionMask getShapeSelectionMask() const override;
    static void *creator();
    static MStatus initialize();

    static MString nodeName();

    // Attribute MObjects
    static MObject m_size_attr;
    static MObject m_file_path_attr;
    static MObject m_exposure_attr;
    static MObject m_time_attr;

    // Node Constants.
    static MTypeId m_id;
    static MString m_draw_db_classification;
    static MString m_draw_registrant_id;
    static MString m_selection_type_name;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H
