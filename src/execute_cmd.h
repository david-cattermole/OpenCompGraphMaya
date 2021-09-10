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
 * Header for 'ocgExecute' Maya command.
 */

#ifndef OPENCOMPGRAPHMAYA_EXECUTE_CMD_H
#define OPENCOMPGRAPHMAYA_EXECUTE_CMD_H


// STL
#include <cmath>

// Maya
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>

#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>

#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <maya/MPoint.h>
#include <maya/MTimeArray.h>

// OCG
#include "opencompgraph.h"

namespace open_comp_graph_maya {

class ExecuteCmd : public MPxCommand {
public:

    ExecuteCmd()
            : m_nodes()
            , m_dry_run(false)
            , m_frame_start(1)
            , m_frame_end(1) {};

    virtual ~ExecuteCmd();

    virtual bool hasSyntax() const;
    static MSyntax newSyntax();

    virtual MStatus doIt(const MArgList &args);

    virtual bool isUndoable() const;

    static void *creator();

    static MString cmdName();

private:
    MStatus parseArgs( const MArgList& args );

    MSelectionList m_nodes;
    bool m_dry_run;
    uint32_t m_frame_start;
    uint32_t m_frame_end;
};

} // namespace open_comp_graph_maya

#endif // OPENCOMPGRAPHMAYA_EXECUTE_CMD_H
