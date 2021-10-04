# Copyright (C) 2021 David Cattermole.
#
# This file is part of OpenCompGraphMaya.
#
# OpenCompGraphMaya is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# OpenCompGraphMaya is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCompGraphMaya.  If not, see <https://www.gnu.org/licenses/>.
#
"""
Render Window, used to give users the options to execute an OCG
graph from Maya.

Example of how to test window:

import OpenCompGraphMaya.render_window as rndr_win
reload(rndr_win)
rndr_win.open_window()

"""

from __future__ import print_function

import abc

import maya.cmds


PLUGIN_NAME = 'OpenCompGraphMaya'


def load_plugin():
    maya.cmds.loadPlugin(PLUGIN_NAME, quiet=True)


def _get_selected_nodes_string():
    node_names = maya.cmds.ls(
        selection=True,
        dagObjects=False) or []
    node_names_str = ','.join(node_names)
    node_names_str = node_names_str.strip(',')
    return node_names_str


def _run(node_names, start_frame, end_frame):
    assert isinstance(node_names, list)
    assert isinstance(start_frame, int)
    assert isinstance(end_frame, int)
    load_plugin()

    # Command arguments.
    kwargs = {
        'frameStart': start_frame,
        'frameEnd': end_frame,
    }

    # Ensure the values are valid.
    try:
        valid = maya.cmds.ocgExecute(
            node_names,
            dryRun=True,
            **kwargs
        )
    except RuntimeError:
        msg = 'Node execution failed.'
        maya.cmds.error(msg)
        raise

    # Do the rendering...
    #
    # TODO: Pause / hide the viewport so it doesn't update while we
    # render.
    maya.cmds.ocgExecute(
        node_names,
        dryRun=False,
        **kwargs
    )
    return True


def _show_exception(exception):
    title = 'Render Failed!'
    button_text = 'Ok'
    msg = 'Error:\n' + str(exception)
    maya.cmds.confirmDialog(
        title=title,
        message=msg,
        button=[button_text],
        defaultButton=button_text,
        cancelButton=button_text,
        dismissString=button_text)


class BaseWindow(object):
    __metaclass__ = abc.ABCMeta

    title = 'Base Window Title'
    name = 'BaseWindow'
    width = 500
    height = 1
    instance = None

    def __init__(self):
        if maya.cmds.window(self.name, query=True, exists=True):
            maya.cmds.deleteUI(self.name, window=True)

        if maya.cmds.windowPref(self.name, exists=True):
            maya.cmds.windowPref(self.name, remove=True)

        self.window_object = maya.cmds.window(
            self.name,
            w=self.width,
            h=self.height,
            title=self.title,
        )

        maya.cmds.setParent(self.instance)
        self.setup_ui(self.instance)
        self.create_connections(self.instance)
        self.populate_ui(self.instance)
        return

    def set_window_title(self, text):
        maya.cmds.window(
            self.window_object,
            edit=True,
            title=text)

    @classmethod
    def close_window(cls):
        if cls.instance is not None:
            cmd = "maya.cmds.deleteUI('{0}', window=True)"
            cmd = cmd.format(cls.instance.window_object)
            maya.cmds.evalDeferred(cmd)
        return

    @classmethod
    def open_window(cls):
        if cls.instance is None:
            cls.instance = cls()
        maya.cmds.showWindow(cls.instance.window_object)

    @abc.abstractmethod
    def setup_ui(self, parent):
        pass

    @abc.abstractmethod
    def create_connections(self, parent):
        pass

    @abc.abstractmethod
    def populate_ui(self, parent):
        pass


class RenderWindow(BaseWindow):

    title = 'OCG Render Window'
    name = 'ocgm_render_window'

    def setup_ui(self, parent):
        maya.cmds.window(
            self.window_object,
            edit=True,
            menuBar=False,
            resizeToFitChildren=True,
            sizeable=False,
            maximizeButton=False,
            minimizeButton=True
        )

        # self.main = maya.cmds.columnLayout()
        self.main = maya.cmds.rowColumnLayout(
            columnWidth=[ (1, self.width) ],
            numberOfColumns=1 )

        self.space1 = maya.cmds.text(label='')

        self.node_names_field = maya.cmds.textFieldButtonGrp(
            label='Node Name(s)',
            text='',
            placeholderText='<node names here>',
            columnWidth3=(self.width * 0.2,
                          self.width * 0.55,
                          self.width * 0.25),
            buttonLabel='Get Selection')

        self.space2 = maya.cmds.text(label='')

        self.frame_range_field = maya.cmds.intFieldGrp(
            numberOfFields=2,
            label='Start/End Frame')

        self.space3 = maya.cmds.text(label='')
        self.space4 = maya.cmds.text(label='')

        button_width = self.width * 0.2
        space_width = self.width - (button_width * 2)
        self.buttons_layout = maya.cmds.rowLayout(
            numberOfColumns=3,
            columnWidth3=(space_width,
                          button_width,
                          button_width),
        )
        self.button_space = maya.cmds.text(label='')
        self.ok_button = maya.cmds.button(label='Render', width=button_width)
        self.close_button = maya.cmds.button(label='Close', width=button_width)
        maya.cmds.setParent('..')

        maya.cmds.setParent(parent)

    def get_node_names_text(self):
        text = maya.cmds.textFieldButtonGrp(
            self.node_names_field,
            query=True,
            text=True)
        return text

    def node_names_button_clicked(self):
        text = _get_selected_nodes_string()
        self.set_node_names_text(text)
        return

    def set_node_names_text(self, text):
        maya.cmds.textFieldButtonGrp(
            self.node_names_field,
            edit=True,
            text=text)
        return

    def get_node_names(self):
        text = self.get_node_names_text()
        node_names = text.split(',')
        node_names = [x.strip() for x in node_names]
        node_names = [x for x in node_names if maya.cmds.objExists(x)]
        return node_names

    def get_start_frame(self):
        value = maya.cmds.intFieldGrp(
            self.frame_range_field,
            query=True,
            value1=True)
        return value

    def get_end_frame(self):
        value = maya.cmds.intFieldGrp(
            self.frame_range_field,
            query=True,
            value2=True)
        return value

    def create_connections(self, parent):
        func = lambda: self.node_names_button_clicked()
        maya.cmds.textFieldButtonGrp(
            self.node_names_field,
            edit=True,
            buttonCommand=func)

        func = lambda x: self.run()
        maya.cmds.button(self.ok_button, edit=True, command=func)

        func = lambda x: RenderWindow.close_window()
        maya.cmds.button(self.close_button, edit=True, command=func)

    def populate_ui(self, parent):
        text = _get_selected_nodes_string()
        self.set_node_names_text(text)

        start_frame = int(maya.cmds.playbackOptions(query=True, minTime=True))
        end_frame = int(maya.cmds.playbackOptions(query=True, maxTime=True))
        maya.cmds.intFieldGrp(
            self.frame_range_field,
            edit=True,
            value1=start_frame,
            value2=end_frame)
        return

    def run(self):
        node_names = self.get_node_names()
        if len(node_names) == 0:
            msg = 'No valid node names given to render.'
            maya.cmds.error(msg)
        start_frame = self.get_start_frame()
        end_frame = self.get_end_frame()
        try:
            _run(node_names, start_frame, end_frame)
        except Exception as e:
            _show_exception(e)
        return


def open_window():
    win = RenderWindow.open_window()
    return win


def main():
    print("OCG Render...")
    open_window()
