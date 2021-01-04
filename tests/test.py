"""
A test function to be run inside Maya to ensure the ogsImagePlane is
working as expected.
"""

import os
import random
import maya.cmds


def _get_random_file_path():
    file_path1 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/checker_8bit_rgba_3840x2160.png"
    file_path2 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/oiio-images/tahoe-gps.jpg"
    file_path3 = "C:/Users/catte/dev/robotArm/imageSequence/robotArm.1001.png"
    file_path4 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/DSC05345.jpg"
    file_paths = [
        file_path1,
        file_path2,
        file_path3,
        file_path4,
    ]
    file_path = random.choice(file_paths)
    return os.path.abspath(file_path)


def test_a():
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    node = maya.cmds.createNode('ocgImagePlane')

    # Set random file path
    file_path = _get_random_file_path()
    maya.cmds.setAttr(node + '.filePath', file_path, type='string')
    return


def test_b():
    read_node = maya.cmds.createNode('ocgImageRead')
    grade_node = maya.cmds.createNode('ocgColorGrade')
    image_plane = maya.cmds.createNode('ocgImagePlane')

    maya.cmds.connectAttr(read_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', image_plane + '.inStream')

    file_path = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    maya.cmds.setAttr(grade_node + '.multiply', 1.0)
    return


def main():
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    test_a()
    test_b()


# main()
