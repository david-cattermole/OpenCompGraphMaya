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
    file_path3 = "C:/Users/catte/dev/robotArm/imageSequence/robotArm.####.png"
    file_path4 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/stills/DSC05345.jpg"
    file_path5 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/vancouver_jpg/vancouver.####.jpg"
    file_path6 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/color_bars_3840x2160_exr/color_bars.####.exr"
    file_path7 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/color_bars_3840x2160_jpg/color_bars.####.jpg"
    file_path8 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/color_bars_3840x2160_png/color_bars.####.png"
    file_path9 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/openexr-images/Beachball/multipart.####.exr"
    file_path10 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/openexr-images/Beachball/singlepart.####.exr"
    file_path11 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/openexr-images/DisplayWindow/t##.exr"

    file_paths = [
        (file_path1, 1, 1),
        (file_path2, 1, 1),
        (file_path3, 991, 1501),
        (file_path4, 1, 1),
        (file_path5, 1, 10),
        (file_path6, 901, 1501),
        (file_path7, 901, 1501),
        (file_path8, 901, 1501),
        (file_path9, 1, 10),
        (file_path10, 1, 10),
        (file_path11, 1, 20),
    ]
    file_path, start_frame, end_frame = random.choice(file_paths)
    return os.path.abspath(file_path), start_frame, end_frame


def test_a():
    """
    An image plane without any inputs must not fail.
    """
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    node = maya.cmds.createNode('ocgImagePlane')
    maya.cmds.connectAttr('time1.outTime', node + ".time")
    return


def test_b():
    """
    Image reading and color grading using color matrix.
    """
    read_node = maya.cmds.createNode('ocgImageRead')
    grade_node = maya.cmds.createNode('ocgColorGrade')
    image_plane = maya.cmds.createNode('ocgImagePlane')

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', image_plane + '.inStream')

    file_path, start_frame, end_frame = _get_random_file_path()
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

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', lens_node + '.inStream')
    maya.cmds.connectAttr(lens_node + '.outStream', image_plane + '.inStream')

    file_path, start_frame, end_frame = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    maya.cmds.setAttr(grade_node + '.multiplyR', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyG', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyB', random.random() * 2)
    maya.cmds.setAttr(grade_node + '.multiplyA', 1.0)
    maya.cmds.setAttr(lens_node + '.distortion', random.uniform(-1.0, 1.0))
    return


def test_d():
    """Read an image, transform the image and view it in
    an image plane.
    """
    read_node = maya.cmds.createNode('ocgImageRead')
    tfm_node = maya.cmds.createNode('ocgImageTransform')
    image_plane = maya.cmds.createNode('ocgImagePlane')

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_node + '.outStream', tfm_node + '.inStream')
    maya.cmds.connectAttr(tfm_node + '.outStream', image_plane + '.inStream')

    file_path, start_frame, end_frame = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    maya.cmds.setAttr(tfm_node + '.translateX', random.uniform(-1.0, 1.0))
    maya.cmds.setAttr(tfm_node + '.translateY', random.uniform(-1.0, 1.0))
    maya.cmds.setAttr(tfm_node + '.rotate', random.random() * 45.0)
    maya.cmds.setAttr(tfm_node + '.scaleX', random.uniform(0.5, 2.0))
    maya.cmds.setAttr(tfm_node + '.scaleY', random.uniform(0.5, 2.0))
    return


def test_e():
    """Read an image, transform the image and view it in
    an image plane.
    """
    read_node = maya.cmds.createNode('ocgImageRead')
    image_plane = maya.cmds.createNode('ocgImagePlane')

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_node + '.outStream', image_plane + '.inStream')

    file_path, start_frame, end_frame = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    return


def test_f():
    """Read an image, grade the colors and then distort it and view it in
    an image plane, WITH a camera.
    """
    read_node = maya.cmds.createNode('ocgImageRead')
    grade_node = maya.cmds.createNode('ocgColorGrade')
    lens_node = maya.cmds.createNode('ocgLensDistort')
    tfm_node = maya.cmds.createNode('ocgImageTransform')
    cam_tfm = maya.cmds.createNode('transform')
    cam_shp = maya.cmds.createNode('camera', parent=cam_tfm)
    image_plane_tfm = maya.cmds.createNode('transform', parent=cam_tfm)
    image_plane = maya.cmds.createNode('ocgImagePlane', parent=image_plane_tfm)

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', lens_node + '.inStream')
    maya.cmds.connectAttr(lens_node + '.outStream', tfm_node + '.inStream')
    maya.cmds.connectAttr(tfm_node + '.outStream', image_plane + '.inStream')
    maya.cmds.connectAttr(cam_shp + '.message', image_plane + '.camera')

    file_path, start_frame, end_frame = _get_random_file_path()
    maya.cmds.setAttr(read_node + '.filePath', file_path, type='string')
    maya.cmds.setAttr(grade_node + '.multiplyR', random.uniform(0.1, 2.0))
    maya.cmds.setAttr(grade_node + '.multiplyG', random.uniform(0.1, 2.0))
    maya.cmds.setAttr(grade_node + '.multiplyB', random.uniform(0.1, 2.0))
    maya.cmds.setAttr(grade_node + '.multiplyA', 1.0)
    maya.cmds.setAttr(lens_node + '.distortion', random.uniform(-0.5, 0.5))
    maya.cmds.setAttr(tfm_node + '.translateX', random.uniform(-0.2, 0.2))
    maya.cmds.setAttr(tfm_node + '.translateY', random.uniform(-0.2, 0.2))
    maya.cmds.setAttr(tfm_node + '.rotate', random.random() * 10.0)
    maya.cmds.setAttr(tfm_node + '.scaleX', random.uniform(0.5, 2.0))
    maya.cmds.setAttr(tfm_node + '.scaleY', random.uniform(0.5, 2.0))
    maya.cmds.setAttr(cam_shp + '.displayCameraFrustum', 1)

    maya.cmds.playbackOptions(min=start_frame, max=end_frame)
    return


def test_g():
    """Read 2 images, grade the colors and then distort, and merge images
    together and view it in an image plane, WITH a camera.
    """
    read_a_node = maya.cmds.createNode('ocgImageRead')
    read_b_node = maya.cmds.createNode('ocgImageRead')
    merge_node = maya.cmds.createNode('ocgImageMerge')
    grade_node = maya.cmds.createNode('ocgColorGrade')
    lens_node = maya.cmds.createNode('ocgLensDistort')
    tfm_node = maya.cmds.createNode('ocgImageTransform')
    cam_tfm = maya.cmds.createNode('transform')
    cam_shp = maya.cmds.createNode('camera', parent=cam_tfm)
    image_plane_tfm = maya.cmds.createNode('transform', parent=cam_tfm)
    image_plane = maya.cmds.createNode('ocgImagePlane', parent=image_plane_tfm)

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_a_node + '.outStream', merge_node + '.inStreamA')
    maya.cmds.connectAttr(read_b_node + '.outStream', grade_node + '.inStream')
    maya.cmds.connectAttr(grade_node + '.outStream', lens_node + '.inStream')
    maya.cmds.connectAttr(lens_node + '.outStream', tfm_node + '.inStream')
    maya.cmds.connectAttr(tfm_node + '.outStream', merge_node + '.inStreamB')
    maya.cmds.connectAttr(merge_node + '.outStream', image_plane + '.inStream')
    maya.cmds.connectAttr(cam_shp + '.message', image_plane + '.camera')

    file_path_a, start_frame_a, end_frame_a = _get_random_file_path()
    file_path_b, start_frame_b, end_frame_b = _get_random_file_path()
    start_frame = min(start_frame_a, start_frame_b)
    end_frame = max(end_frame_a, end_frame_b)

    maya.cmds.setAttr(read_a_node + '.filePath', file_path_a, type='string')
    maya.cmds.setAttr(read_b_node + '.filePath', file_path_b, type='string')
    maya.cmds.setAttr(grade_node + '.multiplyR', random.uniform(0.1, 2.0))
    maya.cmds.setAttr(grade_node + '.multiplyG', random.uniform(0.1, 2.0))
    maya.cmds.setAttr(grade_node + '.multiplyB', random.uniform(0.1, 2.0))
    maya.cmds.setAttr(grade_node + '.multiplyA', 1.0)
    maya.cmds.setAttr(lens_node + '.distortion', random.uniform(-0.5, 0.5))
    maya.cmds.setAttr(tfm_node + '.translateX', random.uniform(-0.2, 0.2))
    maya.cmds.setAttr(tfm_node + '.translateY', random.uniform(-0.2, 0.2))
    maya.cmds.setAttr(tfm_node + '.rotate', random.random() * 10.0)
    maya.cmds.setAttr(tfm_node + '.scaleX', random.uniform(0.5, 2.0))
    maya.cmds.setAttr(tfm_node + '.scaleY', random.uniform(0.5, 2.0))
    maya.cmds.setAttr(cam_shp + '.displayCameraFrustum', 1)

    maya.cmds.playbackOptions(min=start_frame, max=end_frame)
    return


def test_h():
    """Read 2 images, merge them together and view it in an image plane,
    WITH a camera.

    """
    read_a_node = maya.cmds.createNode('ocgImageRead')
    read_b_node = maya.cmds.createNode('ocgImageRead')
    merge_node = maya.cmds.createNode('ocgImageMerge')
    cam_tfm = maya.cmds.createNode('transform')
    cam_shp = maya.cmds.createNode('camera', parent=cam_tfm)
    image_plane_tfm = maya.cmds.createNode('transform', parent=cam_tfm)
    image_plane = maya.cmds.createNode('ocgImagePlane', parent=image_plane_tfm)

    maya.cmds.connectAttr('time1.outTime', image_plane + ".time")
    maya.cmds.connectAttr(read_a_node + '.outStream', merge_node + '.inStreamA')
    maya.cmds.connectAttr(read_b_node + '.outStream', merge_node + '.inStreamB')
    maya.cmds.connectAttr(merge_node + '.outStream', image_plane + '.inStream')
    maya.cmds.connectAttr(cam_shp + '.message', image_plane + '.camera')

    file_path_a, start_frame_a, end_frame_a = _get_random_file_path()
    file_path_b, start_frame_b, end_frame_b = _get_random_file_path()
    start_frame = min(start_frame_a, start_frame_b)
    end_frame = max(end_frame_a, end_frame_b)

    maya.cmds.setAttr(read_a_node + '.filePath', file_path_a, type='string')
    maya.cmds.setAttr(read_b_node + '.filePath', file_path_b, type='string')
    maya.cmds.setAttr(cam_shp + '.displayCameraFrustum', 1)

    maya.cmds.playbackOptions(min=start_frame, max=end_frame)
    return


def main():
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    test_a()
    test_b()
    test_c()
    test_d()
    test_e()
    test_f()
    test_g()
    test_h()


main()
