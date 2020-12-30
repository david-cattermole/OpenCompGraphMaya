"""
A test function to be run inside Maya to ensure the ogsImagePlane is
working as expected.
"""


import os
import random
import maya.cmds

def main():
    maya.cmds.loadPlugin('OpenCompGraphMaya')
    node = maya.cmds.createNode('ocgImagePlane')

    # Set random file path
    file_path1 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/checker_8bit_rgba_3840x2160.png"
    file_path2 = "C:/Users/catte/dev/OpenCompGraphMaya/src/OpenCompGraph/tests/data/oiio-images/tahoe-gps.jpg"
    file_path = random.choice([file_path1, file_path2])
    maya.cmds.setAttr(node + '.filePath', os.path.abspath(file_path1), type='string')
