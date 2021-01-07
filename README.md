# Open Comp Graph For Autodesk Maya

*Open Comp Graph Maya* (or *OCG Maya* for short) is currently in
development. When completed, the project will allow artists to create
simple image processing composting node graphs inside of Autodesk
Maya's DG Network, and view the results inside Maya's Viewport 2.0.

*Open Comp Graph Maya* is a plug-in using the
[Open Comp Graph](https://github.com/david-cattermole/OpenCompGraph/)
library (currently in development).

*OCG Maya* consists of the following node types:

| Node Type      | Description                                      | Status              |
|----------------|--------------------------------------------------|---------------------|
| ocgImagePlane  | Displays the output of OCG to the Maya Viewport. | In progress         |
| ocgImageRead   | Reads an image from disk.                        | In progress         |
| ocgImageWrite  | Write an image to disk.                          | Not implemented yet |
| ocgImageMerge  | Merge two images together.                       | Not implemented yet |
| ocgColorGrade  | Perform colour grading to an image.              | In progress         |
| ocgLensDistort | Deform an image using brownian lens distortion.  | In progress         |
| ocgNull        | An empty image operation - null operation.       | Not implemented yet |
| ocgSwitch      | Switch between multiple node inputs.             | Not implemented yet |
|                |                                                  |                     |

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

## Bugs or Issues?

All issues are listed in the
[issues page](https://github.com/david-cattermole/OpenCompGraphMaya/issues)
on the project page. If you have found a bug, please submit an issue and we will
try to address it as soon as possible.
