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
#include <maya/MDagMessage.h>
#include <maya/MSelectionContext.h>
#include <maya/MGlobal.h>
#include <maya/MEvaluationNode.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MUuid.h>

// Maya Viewport 2.0
#include <maya/MDrawRegistry.h>
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>
#include <maya/MHWGeometry.h>
#include <maya/MHWGeometryUtilities.h>

// OCG
#include "opencompgraph.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {
namespace image_plane {

class ShapeNode : public MPxLocatorNode {
public:
    ShapeNode();
    ~ShapeNode() override;

    void postConstructor();

    MStatus compute(const MPlug &plug, MDataBlock &data) override;

    // void draw(M3dView &view, const MDagPath &path,
    //           M3dView::DisplayStyle style,
    //           M3dView::DisplayStatus status) override;

    bool isBounded() const override;
    MBoundingBox boundingBox() const override;
    MSelectionMask getShapeSelectionMask() const override;
    bool excludeAsLocator() const;
    MStatus preEvaluation(
        const MDGContext& context,
        const MEvaluationNode& evaluationNode) override;
    MStatus postEvaluation(
        const MDGContext& context,
        const MEvaluationNode& evaluationNode,
        PostEvaluationType evalType) override;
    static void *creator();
    static MStatus initialize();

    static MString nodeName();

    // Attribute MObjects
    static MObject m_camera_attr;
    static MObject m_in_stream_attr;
    //
    static MObject m_display_mode_attr;
    static MObject m_display_color_attr;
    static MObject m_display_alpha_attr;
    static MObject m_display_saturation_attr;
    static MObject m_display_exposure_attr;
    static MObject m_display_gamma_attr;
    static MObject m_display_soft_clip_attr;
    static MObject m_display_use_draw_depth_attr;
    static MObject m_display_draw_depth_attr;
    //
    static MObject m_card_depth_attr;
    static MObject m_card_size_x_attr;
    static MObject m_card_size_y_attr;
    static MObject m_card_res_x_attr;
    static MObject m_card_res_y_attr;
    //
    static MObject m_color_space_name_attr;
    static MObject m_lut_edge_size_attr;
    //
    static MObject m_cache_option_attr;
    static MObject m_cache_pixel_data_type_attr;
    static MObject m_cache_crop_on_format_attr;
    //
    static MObject m_disk_cache_enable_attr;
    static MObject m_disk_cache_file_path_attr;
    //
    static MObject m_time_attr;
    static MObject m_out_stream_attr;

    // Node Constants.
    static MTypeId m_id;
    static MString m_draw_db_classification;
    static MString m_draw_registrant_id;
    static MString m_selection_type_name;
    static MString m_display_filter_name;
    static MString m_display_filter_label;

    // Output OCG node
    ocg::Node m_out_stream_node;

    // Unique id for the node.
    MUuid m_node_uuid;
};

} // namespace image_plane
} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_IMAGE_PLANE_SHAPE_H
