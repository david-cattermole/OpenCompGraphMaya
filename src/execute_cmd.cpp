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
 * Execute OCG nodes in the Maya node network, used to render write
 * nodes.
 *
 * Command for running ocgExecute.
 *
 * Example usage (MEL):
 *
 *   ocgExecute
 *       -frameStart 1001
 *       -frameEnd 1101
 *       -dryRun false
 *       "myNodeName1";
 *
 */

// STL
#include <vector>
#include <cmath>
#include <cassert>

// Maya
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MTime.h>
#include <maya/MTimeArray.h>
#include <maya/MMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MUuid.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MAnimControl.h>
#include <maya/MComputation.h>

// OCG
#include "opencompgraph.h"

// OCG Maya
#include <opencompgraphmaya/node_type_ids.h>
#include "logger.h"
#include "global_cache.h"
#include "graph_data.h"
#include "graph_execute.h"
#include "node_utils.h"

#include "execute_cmd.h"

namespace ocg = open_comp_graph;
namespace ocgm_graph = open_comp_graph_maya::graph;
namespace ocgm_cache = open_comp_graph_maya::cache;

namespace open_comp_graph_maya {

// Command arguments:
#define DRY_RUN_FLAG            "-dr"
#define DRY_RUN_FLAG_LONG       "-dryRun"

// Frame range values
#define FRAME_START_FLAG        "-fs"
#define FRAME_START_FLAG_LONG   "-frameStart"
#define FRAME_END_FLAG          "-fe"
#define FRAME_END_FLAG_LONG     "-frameEnd"


ExecuteCmd::~ExecuteCmd() {}

void *ExecuteCmd::creator() {
    return new ExecuteCmd();
}

MString ExecuteCmd::cmdName() {
    return MString(OCGM_EXECUTE_CMD_NAME);
}


bool ExecuteCmd::hasSyntax() const {
    return true;
}


bool ExecuteCmd::isUndoable() const {
    return false;
}


MSyntax ExecuteCmd::newSyntax() {
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    syntax.useSelectionAsDefault(true);

    unsigned int minNumObjects = 1;
    syntax.setObjectType(MSyntax::kSelectionList, minNumObjects);

    syntax.addFlag(DRY_RUN_FLAG, DRY_RUN_FLAG_LONG, MSyntax::kBoolean);
    syntax.addFlag(FRAME_START_FLAG, FRAME_START_FLAG_LONG, MSyntax::kLong);
    syntax.addFlag(FRAME_END_FLAG, FRAME_END_FLAG_LONG, MSyntax::kLong);
    return syntax;
}


MStatus ExecuteCmd::parseArgs(const MArgList &args) {
    MStatus status = MStatus::kSuccess;
    auto log = log::get_logger();

    MArgDatabase argData(syntax(), args, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = argData.getObjects(m_nodes);
    if (status != MStatus::kSuccess) {
        log->error("Error parsing {} command arguments.",
                   OCGM_EXECUTE_CMD_NAME);
        return status;
    }

    // CHECK_MSTATUS_AND_RETURN_IT(status);
    if (m_nodes.length() == 0) {
        // log->error("Error parsing {} command arguments.",
        //            OCGM_EXECUTE_CMD_NAME);
        status = MStatus::kFailure;
        status.perror("No objects given!");
        return status;
    }

    // Dry Run flag
    m_dry_run = false;
    bool dryRunFlagIsSet = argData.isFlagSet(DRY_RUN_FLAG, &status);
    if (dryRunFlagIsSet == true) {
         status = argData.getFlagArgument(DRY_RUN_FLAG, 0, m_dry_run);
         CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    MTime start_time = MAnimControl::minTime();
    MTime end_time = MAnimControl::maxTime();

    // Frame Start flag
    m_frame_start = static_cast<uint32_t>(start_time.as(MTime::uiUnit()));
    bool frameStartFlagIsSet = argData.isFlagSet(FRAME_START_FLAG, &status);
    if (frameStartFlagIsSet == true) {
         status = argData.getFlagArgument(FRAME_START_FLAG, 0, m_frame_start);
         CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // Frame End flag
    m_frame_end = static_cast<uint32_t>(end_time.as(MTime::uiUnit()));
    bool frameEndFlagIsSet = argData.isFlagSet(FRAME_END_FLAG, &status);
    if (frameEndFlagIsSet == true) {
        status = argData.getFlagArgument(FRAME_END_FLAG, 0, m_frame_end);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // Check frame range is valid.
    if (m_frame_end < m_frame_start) {
        log->error(
            "{}: Start frame ({}) is greater than the end frame ({}).",
            OCGM_EXECUTE_CMD_NAME, m_frame_start, m_frame_end);
        status = MStatus::kFailure;
        status.perror("Start frame is greater than end frame.");
        return status;
    }
    auto num_frames = (m_frame_end - m_frame_start) + 1;
    if (num_frames == 0) {
        log->error(
            "{}: Number of frames to execute is zero: Start frame = {}, End frame {}.",
            OCGM_EXECUTE_CMD_NAME, m_frame_start, m_frame_end);
        status = MStatus::kFailure;
        status.perror("Number of frames to execute is zero.");
        return status;
    }

    return status;
}


MStatus ExecuteCmd::doIt(const MArgList &args) {
    MStatus status = MStatus::kSuccess;
    auto log = log::get_logger();

    // Command Outputs
    MStringArray outResult;

    // Read all the flag arguments.
    status = ExecuteCmd::parseArgs(args);
    if (status != MStatus::kSuccess) {
        log->error(
            "{}: Error parsing command arguments.",
            OCGM_EXECUTE_CMD_NAME);
        return status;
    }

    // Get OCG Nodes to be executed.
    auto shared_graph = get_shared_graph();
    std::vector<ocg::Node> ocg_nodes;
    for (auto i = 0; i < m_nodes.length(); ++i) {
        MPlug stream_plug;

        MObject node_obj;
        status = m_nodes.getDependNode(i, node_obj);
        if (status != MStatus::kSuccess) {
            log->error(
                "{}: Error parsing command arguments.",
                OCGM_EXECUTE_CMD_NAME);
            continue;
        }

        MFnDependencyNode dep(node_obj, &status);
        CHECK_MSTATUS(status);
        if (status != MStatus::kSuccess) {
            log->error(
                "{}: Could not find OCG stream attribute to execute on node.",
                OCGM_EXECUTE_CMD_NAME);
            continue;
        }
        log->info(
            "{}: Found node: {}",
            OCGM_EXECUTE_CMD_NAME, dep.name().asChar());

        // Get plug object from the user given node (or plug).
        status = m_nodes.getPlug(i, stream_plug);
        if (status != MStatus::kSuccess || stream_plug.isNull()) {
            log->info(
                "{}: Searching for stream plug.",
                OCGM_EXECUTE_CMD_NAME);

            MString plug_name("outStream");
            stream_plug = dep.findPlug(plug_name, true, &status);
            CHECK_MSTATUS(status);
            if (status != MStatus::kSuccess || stream_plug.isNull()) {
                log->error(
                    "{}: Could not find OCG stream attribute to execute on node.",
                    OCGM_EXECUTE_CMD_NAME);
                continue;
            }
        }
        log->info(
            "{}: Found node plug: {}",
            OCGM_EXECUTE_CMD_NAME, stream_plug.name().asChar());

        ocg::Node stream_node;
        status = open_comp_graph_maya::utils::get_plug_ocg_stream_value(
            stream_plug,
            shared_graph,
            stream_node);
        CHECK_MSTATUS(status);
        log->info(
            "{}: Got node: {}",
            OCGM_EXECUTE_CMD_NAME,
            stream_node.get_id());

        auto exists = shared_graph->node_exists(stream_node);
        if (!exists) {
            log->warn(
                "{}: Node does not exist, skipping: {}",
                OCGM_EXECUTE_CMD_NAME,
                stream_node.get_id());
            continue;
        }

        // TODO: Check if the stream node found is valid for
        // executing.
        auto node_valid = true;
        if (!node_valid) {
            log->warn(
                "{}: Node cannot be executed, skipping: {}",
                OCGM_EXECUTE_CMD_NAME,
                stream_node.get_id());
            continue;
        }

        ocg_nodes.push_back(stream_node);
    }

    if (ocg_nodes.size() == 0) {
        log->error(
            "{}: No OCG nodes found for execution.",
            OCGM_EXECUTE_CMD_NAME);
        return MS::kFailure;
    }

    if (m_dry_run) {
        log->debug(
            "{}: Dry run enabled, stopping before executing nodes..",
            OCGM_EXECUTE_CMD_NAME);
        return MS::kSuccess;
    }

    // Evaluate the OCG Graph.
    //
    // TODO: Pause the viewport so it doesn't update.
    //
    // TODO: Run MEL "waitCursor -state on" and "off" when executing
    // to inform the user something is happening.
    //
    MComputation computation;
    computation.beginComputation(true);
    auto num_frames = (m_frame_end - m_frame_start) + 1;
    auto num_node_frames = ocg_nodes.size() * num_frames;
    auto execute_count = 0;
    computation.setProgressRange(0, num_node_frames);

    auto shared_cache = ocgm_cache::get_shared_cache();
    for (auto ocg_node : ocg_nodes) {
        for (auto frame = m_frame_start; frame <= m_frame_end; ++frame) {
            double execute_frame = static_cast<double>(frame);
            log->debug("ocgExecute: execute_frame={}", execute_frame);
            log->info(
                "{}: Executing Node {} on Frame {}.",
                OCGM_EXECUTE_CMD_NAME,
                ocg_node.get_id(),
                execute_frame);

            // TODO: Add a callback that can be run by Maya to
            // periodically check the status of the executing.
            //
            MGlobal::viewFrame(execute_frame);
            auto exec_status = ocgm_graph::execute_ocg_graph(
                ocg_node,
                execute_frame,
                shared_graph,
                shared_cache);

            if (exec_status != ocg::ExecuteStatus::kSuccess) {
                log->warn("Execute failed!");
            } else {
                log->info("Execute finished with success.");
                // TODO: Print statistics to the log.

                // // std::cout << "Graph as string:\n"
                // //           << graph.data_debug_string();
                // // exec_status == ocg::ExecuteStatus::kSuccess

                // auto stream_data = graph.output_stream();
                // auto state_id = stream_data.state_id();
                // auto hash = stream_data.hash();
                // // std::cout << "state_id=" << state_id << '\n';
                // // std::cout << "hash=" << hash << '\n';

                // auto pixel_buffer = stream_data1.pixel_buffer();
                // std::cout << "pixel_buffer"
                //           << " data=" << &pixel_buffer
                //           << " size=" << pixel_buffer.size() << '\n';

                // auto width = stream_data.pixel_width();
                // auto height = stream_data.pixel_height();
                // auto num_channels = stream_data.pixel_num_channels();
                // std::cout << "width=" << width << '\n';
                // std::cout << "height=" << height << '\n';
                // std::cout << "num_channels="
                //     << static_cast<uint32_t>(num_channels) << '\n';
            }
            if (computation.isInterruptRequested()) {
                break;
            }
            computation.setProgress(execute_count);
            execute_count += 1;
        }
    }
    computation.endComputation();

    return status;
}

} // namespace open_comp_graph_maya
