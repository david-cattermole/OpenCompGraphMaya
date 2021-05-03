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
 * This file lists the Node Type IDs used in the OpenCompGraphMaya project
 * and other projects by David Cattermole.
 *
 */

#ifndef OPENCOMPGRAPHMAYA_NODE_TYPE_IDS_H
#define OPENCOMPGRAPHMAYA_NODE_TYPE_IDS_H

#define MM_MARKER_SCALE_TYPE_ID 0x0012F180
#define MM_REPROJECTION_TYPE_ID 0x0012F181
#define MM_MARKER_GROUP_TRANSFORM_TYPE_ID 0x0012F182
#define CAMERA_INFERNO_TYPE_ID 0x0012F183
#define MM_LENS_DATA_TYPE_ID 0x0012F184
#define MM_LENS_DATA_TYPE_NAME "MMLensData"
#define MM_LENS_DEFORMER_TYPE_ID 0x0012F185
#define MM_LENS_EVALUATE_TYPE_ID 0x0012F186
#define MM_LENS_MODEL_BASIC_TYPE_ID 0x0012F187
#define MM_LENS_MODEL_TOGGLE_TYPE_ID 0x0012F188
#define MM_MARKER_TRANSFORM_TYPE_ID 0x0012F189
#define MM_MARKER_TRANSFORM_MATRIX_TYPE_ID 0x0012F18A

// Set to '1' to use the 'MPxSubSceneOverride' implementation.
#define OCG_USE_SUB_SCENE_OVERRIDE 0

#define OCGM_IMAGE_PLANE_SHAPE_TYPE_ID 0x0012F18B
#define OCGM_IMAGE_PLANE_SHAPE_TYPE_NAME "ocgImagePlane"
#if OCG_USE_SUB_SCENE_OVERRIDE == 1
#define OCGM_IMAGE_PLANE_DRAW_CLASSIFY "drawdb/subscene/OpenCompGraph/ocgImagePlane"
#else
#define OCGM_IMAGE_PLANE_DRAW_CLASSIFY "drawdb/geometry/OpenCompGraph/ocgImagePlane"
#endif
#define OCGM_IMAGE_PLANE_DRAW_REGISTRANT_ID "ocgImagePlaneSubSceneOverride"
#define OCGM_IMAGE_PLANE_SHAPE_SELECTION_TYPE_NAME "ocgImagePlaneSelection"
#define OCGM_IMAGE_PLANE_SHAPE_DISPLAY_FILTER_NAME "ocgImagePlaneDisplayFilter"
#define OCGM_IMAGE_PLANE_SHAPE_DISPLAY_FILTER_LABEL "OCG ImagePlane"

#define MM_MARKER_BUNDLE_SHAPE_TYPE_ID 0x0012F18C
#define OCGM_GRAPH_DATA_TYPE_ID 0x0012F18D
#define OCGM_GRAPH_DATA_TYPE_NAME "ocgGraphData"
#define OCGM_IMAGE_READ_TYPE_ID 0x0012F18E
#define OCGM_IMAGE_READ_TYPE_NAME "ocgImageRead"
#define OCGM_COLOR_GRADE_TYPE_ID 0x0012F18F
#define OCGM_COLOR_GRADE_TYPE_NAME "ocgColorGrade"
#define OCGM_LENS_DISTORT_TYPE_ID 0x0012F190
#define OCGM_LENS_DISTORT_TYPE_NAME "ocgLensDistort"
#define OCGM_IMAGE_TRANSFORM_TYPE_ID 0x0012F191
#define OCGM_IMAGE_TRANSFORM_TYPE_NAME "ocgImageTransform"
#define OCGM_IMAGE_MERGE_TYPE_ID 0x0012F192
#define OCGM_IMAGE_MERGE_TYPE_NAME "ocgImageMerge"

#define OCGM_IMAGE_CROP_TYPE_ID 0x0012F198
#define OCGM_IMAGE_CROP_TYPE_NAME "ocgImageCrop"

#endif // OPENCOMPGRAPHMAYA_NODE_TYPE_IDS_H
