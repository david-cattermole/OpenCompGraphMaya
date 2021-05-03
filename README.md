# Open Comp Graph For Autodesk Maya

**Open Comp Graph Maya** (or **OCG Maya** - pronounced *Ohh-See-Gee
Maya*). OCG Maya will allow artists to create simple image processing
composting node graphs inside of Autodesk Maya's DG Network, and view
the results inside Maya's Viewport 2.0. Optionally, it will be
possible to write out image data from Maya

**OCG Maya** is focused on real-time image caching, playback speed,
efficent graph evaluation and support for real-time lens distortion
previews.

## Road Map

This project is currently in development and should not be used in
production for any reason.

*Open Comp Graph Maya* is a plug-in using the
[Open Comp Graph](https://github.com/david-cattermole/OpenCompGraph/)
library (currently in development).

*OCG Maya* consists of the following node types:

| Node Type        | Description                                      | Status              |
|------------------|--------------------------------------------------|---------------------|
| ocgImagePlane    | Displays the output of OCG to the Maya Viewport. | In progress         |
| ocgImageRead     | Reads an image from disk.                        | In progress         |
| ocgImageWrite    | Write an image to disk.                          | Not implemented yet |
| ocgImageMerge    | Merge two images together.                       | In progress         |
| ocgImageCrop     | Crop an image to a sub-window.                   | In progress         |
| ocgImageReformat | Change the resolution of images.                 | Not implemented yet |
| ocgImageRetime   | Perform time-based frame blending                | Not implemented yet |
| ocgImageKeyer    | Create a green-screen matte.                     | Not implemented yet |
| ocgColorGrade    | Perform colour grading to an image.              | In progress         |
| ocgLensDistort   | Deform an image using brownian lens distortion.  | In progress         |
| ocgNull          | An empty image operation - null operation.       | Not implemented yet |
| ocgSwitch        | Switch between multiple node inputs.             | Not implemented yet |

Below is a brief description on some of the features planed and being
worked on.

| Feature Description                                                  |        Status |
|:---------------------------------------------------------------------|--------------:|
| Draw live image on a flat plane.                                     |          Done |
| Draw live image on a camera image plane.                             |          Done |
| Draw live image on a sphere.                                         | To be started |
| Draw live image on a cylinder.                                       | To be started |
| Draw live image on a cube.                                           | To be started |
| Import lens distortion values from Nuke.                             | To be started |
| Import lens distortion values from 3DEqualizer.                      | To be started |
| Export lens distortion values to Nuke.                               | To be started |
| Export lens distortion values to 3DEqualizer.                        | To be started |
| Python API to save, and load OCG graph networks as Maya nodes.       | To be started |
| Python API to create and manipulate OCG Maya nodes.                  | To be started |
| Write node graph to disk to bake and speed up read speeds.           | To be started |
| Control OCG caching preferences from Maya nodes.                     | To be started |
| Display which frames are cached in the Maya timeline (with a color). | To be started |
| Render the image plane natively in Software renderers (e.g. Arnold)  | To be started |
| Transform node matching image space, with effect of deformer         |          Done |

## Installation / Building

To install, follow the instructions in
[INSTALL.md](https://github.com/david-cattermole/OpenCompGraphMaya/blob/master/INSTALL.md).
To build (compile) the plug-in follow the steps in
[BUILD.md](https://github.com/david-cattermole/OpenCompGraphMaya/blob/master/BUILD.md).

## License

*Open Comp Graph Maya* (OCG Maya) is licensed under the
[Lesser GNU Public License v3.0](https://github.com/david-cattermole/OpenCompGraphMaya/blob/master/LICENSE)
or *LGPL-3.0* for short.
This means the project is Free Open Source Software, and will always
stay Free Open Source Software:
[TL;DR](https://www.tldrlegal.com/l/lgpl-3.0).

Please read the *LICENSE* (text) file for details.

## Bugs, Questions or Issues?

All issues are listed in the
[issues page](https://github.com/david-cattermole/OpenCompGraphMaya/issues)
on the project page. If you have found a bug, please submit an issue and we will
try to address it as soon as possible.
