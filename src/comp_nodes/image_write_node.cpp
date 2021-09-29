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
 * Write an image from a file path.
 */

// Maya
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MUuid.h>

// STL
#include <cstring>
#include <cmath>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "logger.h"
#include "graph_data.h"
#include "image_write_node.h"
#include "node_utils.h"

namespace ocg = open_comp_graph;

namespace open_comp_graph_maya {

// Precompute index for enum.
const int32_t kDataTypeFloat32 = static_cast<int32_t>(ocg::DataType::kFloat32);
const int32_t kDataTypeHalf16 = static_cast<int32_t>(ocg::DataType::kHalf16);
const int32_t kDataTypeUInt8 = static_cast<int32_t>(ocg::DataType::kUInt8);
const int32_t kDataTypeUInt16 = static_cast<int32_t>(ocg::DataType::kUInt16);
const int32_t kDataTypeUnknown = static_cast<int32_t>(ocg::DataType::kUnknown);

const int32_t kCropOnWriteAuto = static_cast<int32_t>(ocg::CropOnWrite::kAuto);
const int32_t kCropOnWriteEnable = static_cast<int32_t>(ocg::CropOnWrite::kEnable);
const int32_t kCropOnWriteDisable = static_cast<int32_t>(ocg::CropOnWrite::kDisable);

const int32_t kExrCompressDefault = static_cast<int32_t>(ocg::ExrCompression::kDefault);
const int32_t kExrCompressNone = static_cast<int32_t>(ocg::ExrCompression::kNone);
const int32_t kExrCompressRle = static_cast<int32_t>(ocg::ExrCompression::kRle);
const int32_t kExrCompressZip = static_cast<int32_t>(ocg::ExrCompression::kZip);
const int32_t kExrCompressZipScanline = static_cast<int32_t>(ocg::ExrCompression::kZipScanline);
const int32_t kExrCompressPiz = static_cast<int32_t>(ocg::ExrCompression::kPiz);
const int32_t kExrCompressPxr24 = static_cast<int32_t>(ocg::ExrCompression::kPxr24);
const int32_t kExrCompressB44 = static_cast<int32_t>(ocg::ExrCompression::kB44);
const int32_t kExrCompressB44a = static_cast<int32_t>(ocg::ExrCompression::kB44a);
const int32_t kExrCompressDwaa = static_cast<int32_t>(ocg::ExrCompression::kDwaa);
const int32_t kExrCompressDwab = static_cast<int32_t>(ocg::ExrCompression::kDwab);

const int32_t kJpegChromaSubSamplingDefault = static_cast<int32_t>(ocg::JpegChromaSubSampling::kDefault);
const int32_t kJpegChromaSubSampling444 = static_cast<int32_t>(ocg::JpegChromaSubSampling::kNone444);
const int32_t kJpegChromaSubSampling422 = static_cast<int32_t>(ocg::JpegChromaSubSampling::kSample422);
const int32_t kJpegChromaSubSampling420 = static_cast<int32_t>(ocg::JpegChromaSubSampling::kSample420);
const int32_t kJpegChromaSubSampling421 = static_cast<int32_t>(ocg::JpegChromaSubSampling::kSample421);

MTypeId ImageWriteNode::m_id(OCGM_IMAGE_WRITE_TYPE_ID);

// Input Attributes
MObject ImageWriteNode::m_in_stream_attr;
MObject ImageWriteNode::m_enable_attr;
MObject ImageWriteNode::m_file_path_attr;
MObject ImageWriteNode::m_crop_on_write_attr;
MObject ImageWriteNode::m_pixel_data_type_attr;
MObject ImageWriteNode::m_exr_compression_attr;
MObject ImageWriteNode::m_exr_dwa_compression_level_attr;
MObject ImageWriteNode::m_png_compression_level_attr;
MObject ImageWriteNode::m_jpeg_compression_level_attr;
MObject ImageWriteNode::m_jpeg_chroma_sub_sampling_attr;
MObject ImageWriteNode::m_jpeg_progressive_attr;

// Output Attributes
MObject ImageWriteNode::m_out_stream_attr;

ImageWriteNode::ImageWriteNode()
        : m_ocg_node(ocg::Node(ocg::NodeType::kNull, 0)) {}

ImageWriteNode::~ImageWriteNode() {}

MString ImageWriteNode::nodeName() {
    return MString(OCGM_IMAGE_WRITE_TYPE_NAME);
}

MStatus ImageWriteNode::updateOcgNodes(
        MDataBlock &data,
        std::shared_ptr<ocg::Graph> &shared_graph,
        std::vector<ocg::Node> input_ocg_nodes,
        ocg::Node &output_ocg_node) {
    MStatus status = MS::kSuccess;
    auto log = log::get_logger();
    if (input_ocg_nodes.size() != 1) {
        return MS::kFailure;
    }

    bool exists = shared_graph->node_exists(m_ocg_node);
    if (!exists) {
        MString node_name = "write";
        auto node_hash = utils::generate_unique_node_hash(
            m_node_uuid,
            node_name);
        m_ocg_node = shared_graph->create_node(
            ocg::NodeType::kWriteImage,
            node_hash);
    }
    auto input_ocg_node = input_ocg_nodes[0];

    uint8_t input_num = 0;
    status = utils::join_ocg_nodes(
        shared_graph,
        input_ocg_node,
        m_ocg_node,
        input_num);
    CHECK_MSTATUS(status);

    if (m_ocg_node.get_id() != 0) {
        // Set the output node
        output_ocg_node = m_ocg_node;

        // Enable Attribute toggle
        bool enable = utils::get_attr_value_bool(data, m_enable_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "enable", static_cast<int32_t>(enable));

        // File Path Attribute
        MString file_path = utils::get_attr_value_string(data, m_file_path_attr);
        shared_graph->set_node_attr_str(
            m_ocg_node, "file_path", file_path.asChar());

        // Crop-on-Write Attribute
        int16_t crop_on_write = utils::get_attr_value_short(
            data, m_crop_on_write_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "crop_on_write",
            static_cast<int32_t>(crop_on_write));

        // Pixel Data Type Attribute
        int16_t pixel_data_type = utils::get_attr_value_short(
            data, m_pixel_data_type_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "pixel_data_type",
            static_cast<int32_t>(pixel_data_type));

        // EXR Compression Attribute
        int16_t exr_compression = utils::get_attr_value_short(
            data, m_exr_compression_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "exr_compression",
            static_cast<int32_t>(exr_compression));

        // EXR DWA Compression Level Attribute
        int32_t exr_dwa_compression_level = utils::get_attr_value_int(
            data, m_exr_dwa_compression_level_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "exr_dwa_compression_level",
            exr_dwa_compression_level);

        // PNG Compression Level Attribute
        int32_t png_compression_level = utils::get_attr_value_int(
            data, m_png_compression_level_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "png_compression_level",
            exr_dwa_compression_level);

        // JPEG Compression Level Attribute
        int32_t jpeg_compression_level = utils::get_attr_value_int(
            data, m_jpeg_compression_level_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "jpeg_compression_level",
            exr_dwa_compression_level);

        // JPEG Sub-Sampling Attribute
        int16_t jpeg_subsampling = utils::get_attr_value_short(
            data, m_jpeg_chroma_sub_sampling_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "jpeg_subsampling",
            static_cast<int32_t>(jpeg_subsampling));

        // JPEG Progressive Attribute toggle
        bool jpeg_progressive = utils::get_attr_value_bool(
            data, m_jpeg_progressive_attr);
        shared_graph->set_node_attr_i32(
            m_ocg_node, "jpeg_progressive",
            static_cast<int32_t>(jpeg_progressive));
    }

    return status;
}

MStatus ImageWriteNode::compute(const MPlug &plug, MDataBlock &data) {
    MObjectArray in_attr_array;
    in_attr_array.append(m_in_stream_attr);
    return computeOcgStream(
        plug, data,
        in_attr_array,
        m_out_stream_attr);
}

void *ImageWriteNode::creator() {
    return (new ImageWriteNode());
}

MStatus ImageWriteNode::initialize() {
    MStatus status;
    MFnUnitAttribute    uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute   tAttr;
    MFnEnumAttribute    eAttr;

    // Create empty string data to be used as attribute default
    // (string) value.
    MFnStringData empty_string_data;
    MObject empty_string_data_obj = empty_string_data.create("");

    // File Path Attribute
    m_file_path_attr = tAttr.create(
            "filePath", "fp",
            MFnData::kString, empty_string_data_obj);
    CHECK_MSTATUS(tAttr.setStorable(true));
    CHECK_MSTATUS(tAttr.setUsedAsFilename(false));

    // Crop on Write
    m_crop_on_write_attr = eAttr.create(
        "cropOnWrite", "crpnwrt",
        kCropOnWriteAuto);
    CHECK_MSTATUS(eAttr.addField("auto", kCropOnWriteAuto));
    CHECK_MSTATUS(eAttr.addField("enable", kCropOnWriteEnable));
    CHECK_MSTATUS(eAttr.addField("disable", kCropOnWriteDisable));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // Pixel Data Type
    m_pixel_data_type_attr = eAttr.create(
        "pixelDataType", "pxldtyp",
        kDataTypeUnknown);
    CHECK_MSTATUS(eAttr.addField("auto", kDataTypeUnknown));
    CHECK_MSTATUS(eAttr.addField("float32", kDataTypeFloat32));
    CHECK_MSTATUS(eAttr.addField("half16", kDataTypeHalf16));
    CHECK_MSTATUS(eAttr.addField("uint8", kDataTypeUInt8));
    CHECK_MSTATUS(eAttr.addField("uint16", kDataTypeUInt16));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // EXR Compression mode
    m_exr_compression_attr = eAttr.create(
        "exrCompression", "exrcmprs",
        kExrCompressDefault);
    CHECK_MSTATUS(eAttr.addField("default", kExrCompressDefault));
    CHECK_MSTATUS(eAttr.addField("none", kExrCompressNone));
    CHECK_MSTATUS(eAttr.addField("rle", kExrCompressRle));
    CHECK_MSTATUS(eAttr.addField("zip", kExrCompressZip));
    CHECK_MSTATUS(eAttr.addField("zipScanline", kExrCompressZipScanline));
    CHECK_MSTATUS(eAttr.addField("piz", kExrCompressPiz));
    CHECK_MSTATUS(eAttr.addField("pxr24", kExrCompressPxr24));
    CHECK_MSTATUS(eAttr.addField("b44", kExrCompressB44));
    CHECK_MSTATUS(eAttr.addField("b44a", kExrCompressB44a));
    CHECK_MSTATUS(eAttr.addField("dwaa", kExrCompressDwaa));
    CHECK_MSTATUS(eAttr.addField("dwab", kExrCompressDwab));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // EXR DWA Compression Level
    m_exr_dwa_compression_level_attr = nAttr.create(
        "exrDwaCompressionLevel", "exrdwacmprslvl",
        MFnNumericData::kInt, 45);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(0));
    CHECK_MSTATUS(nAttr.setSoftMax(300));

    // PNG Compression Level
    m_png_compression_level_attr = nAttr.create(
        "pngCompressionLevel", "pngcmprslvl",
        MFnNumericData::kInt, 6);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(0));
    CHECK_MSTATUS(nAttr.setMax(9));

    // JPEG Compression Level
    m_jpeg_compression_level_attr = nAttr.create(
        "jpegCompressionLevel", "jpegcmprslvl",
        MFnNumericData::kInt, 90);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setMin(1));
    CHECK_MSTATUS(nAttr.setMax(100));

    // JPEG Chroma Sub-Sampling
    m_jpeg_chroma_sub_sampling_attr = eAttr.create(
        "jpegChromaSubSampling", "jpgchrmssmp",
        kJpegChromaSubSamplingDefault);
    CHECK_MSTATUS(eAttr.addField("default", kJpegChromaSubSamplingDefault));
    CHECK_MSTATUS(eAttr.addField("4:4:4", kJpegChromaSubSampling444));
    CHECK_MSTATUS(eAttr.addField("4:2:2", kJpegChromaSubSampling422));
    CHECK_MSTATUS(eAttr.addField("4:2:0", kJpegChromaSubSampling420));
    CHECK_MSTATUS(eAttr.addField("4:2:1", kJpegChromaSubSampling421));
    CHECK_MSTATUS(eAttr.setStorable(true));

    // JPEG Progressive
    m_jpeg_progressive_attr = nAttr.create(
        "jpegProgressive", "jpgprgs",
        MFnNumericData::kBoolean, false);
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setKeyable(true));

    // Create Common Attributes
    CHECK_MSTATUS(create_enable_attribute(m_enable_attr));
    CHECK_MSTATUS(create_input_stream_attribute(m_in_stream_attr));
    CHECK_MSTATUS(create_output_stream_attribute(m_out_stream_attr));

    // Add Attributes
    CHECK_MSTATUS(addAttribute(m_enable_attr));
    CHECK_MSTATUS(addAttribute(m_file_path_attr));
    CHECK_MSTATUS(addAttribute(m_crop_on_write_attr));
    CHECK_MSTATUS(addAttribute(m_pixel_data_type_attr));
    CHECK_MSTATUS(addAttribute(m_exr_compression_attr));
    CHECK_MSTATUS(addAttribute(m_exr_dwa_compression_level_attr));
    CHECK_MSTATUS(addAttribute(m_png_compression_level_attr));
    CHECK_MSTATUS(addAttribute(m_jpeg_compression_level_attr));
    CHECK_MSTATUS(addAttribute(m_jpeg_chroma_sub_sampling_attr));
    CHECK_MSTATUS(addAttribute(m_jpeg_progressive_attr));
    CHECK_MSTATUS(addAttribute(m_in_stream_attr));
    CHECK_MSTATUS(addAttribute(m_out_stream_attr));

    // Attribute Affects
    CHECK_MSTATUS(attributeAffects(m_enable_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_in_stream_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_file_path_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_crop_on_write_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_pixel_data_type_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_exr_compression_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_exr_dwa_compression_level_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_png_compression_level_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_jpeg_compression_level_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_jpeg_chroma_sub_sampling_attr, m_out_stream_attr));
    CHECK_MSTATUS(attributeAffects(m_jpeg_progressive_attr, m_out_stream_attr));

    return MS::kSuccess;
}

} // namespace open_comp_graph_maya
