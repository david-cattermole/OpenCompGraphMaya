# Copyright (C) 2021 David Cattermole.
#
# This file is part of OpenCompGraph.
#
# OpenCompGraph is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# OpenCompGraph is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCompGraph.  If not, see <https://www.gnu.org/licenses/>.
#
"""
Render Window.
"""

from __future__ import print_function

import maya.cmds


def _run(node_name):
    maya.cmds.ocgExecute()
    return


def main():
    print("OCG Render Window")
    _run("node_name_here")
    pass
