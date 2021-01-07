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
    file_path5 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/vancouver.0001.jpg"

    file_paths = [
        file_path1,
        file_path2,
        file_path3,
        file_path4,
        file_path5,
    ]
    file_path = random.choice(file_paths)
    return os.path.abspath(file_path)


def test_a():
    """
    An image plane without any inputs must not fail.
    """
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    node = maya.cmds.createNode('ocgImagePlane')

    # Set random file path
    file_path = _get_random_file_path()
    maya.cmds.setAttr(node + '.filePath', file_path, type='string')
    return


def test_b():
    """
    Image reading and color grading using color matrix.
    """
    read_node = maya.cmds.createNode('ocgImageRead')
    grade_node = maya.cmds.createNode('ocgColorGrade')
    image_plane = maya.cmds.createNode('ocgImagePlane')

    maya.cmds.connectAttr(read_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', image_plane + '.inStream')

    file_path = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    maya.cmds.setAttr(grade_node + '.multiplyR', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyG', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyB', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyA', 1.0)
    return


def test_c():
    """Read an image, grade the colors and then distort it and view it in
    an image plane.
    """
    read_node = maya.cmds.createNode('ocgImageRead')
    grade_node = maya.cmds.createNode('ocgColorGrade')
    lens_node = maya.cmds.createNode('ocgLensDistort')
    image_plane = maya.cmds.createNode('ocgImagePlane')

    maya.cmds.connectAttr(read_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', lens_node + '.inStream')
    maya.cmds.connectAttr(lens_node + '.outStream', image_plane + '.inStream')

    file_path = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    maya.cmds.setAttr(grade_node + '.multiplyR', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyG', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyB', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyA', 1.0)
    maya.cmds.setAttr(lens_node + '.k1', random.uniform(-1.0, 1.0))
    return


def main():
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    test_a()
    test_b()
    test_c()


main()
