// Maya
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagMessage.h>
#include <maya/MSelectionContext.h>
#include <maya/MStreamUtils.h>

// Maya Viewport 2.0
#include <maya/MPxSubSceneOverride.h>
#include <maya/MShaderManager.h>

// STL
#include <unordered_map>

// OCG Maya
#include "shader.h"
#include "subSceneOverride.h"
#include "shape.h"

namespace open_comp_graph_maya {

ImagePlaneSubSceneOverride::ImagePlaneSubSceneOverride(const MObject &obj)
        : MHWRender::MPxSubSceneOverride(obj), fLocatorNode(obj),
          fMultiplier(0.0f),
          fIsInstanceMode(false),
          fAreUIDrawablesDirty(true),
          fPositionBuffer(nullptr),
          fWireIndexBuffer(nullptr),
          fShadedIndexBuffer(nullptr),
          fInstanceAddedCbId(0),
          fInstanceRemovedCbId(0) {
    MDagPath dagPath;
    if (MDagPath::getAPathTo(obj, dagPath)) {
        fInstanceAddedCbId = MDagMessage::addInstanceAddedDagPathCallback(
            dagPath, InstanceChangedCallback, this);

        fInstanceRemovedCbId = MDagMessage::addInstanceRemovedDagPathCallback(
            dagPath, InstanceChangedCallback, this);
    }
}

ImagePlaneSubSceneOverride::~ImagePlaneSubSceneOverride() {
    deleteGeometryBuffers();

    if (fInstanceAddedCbId != 0) {
        MMessage::removeCallback(fInstanceAddedCbId);
        fInstanceAddedCbId = 0;
    }

    if (fInstanceRemovedCbId != 0) {
        MMessage::removeCallback(fInstanceRemovedCbId);
        fInstanceRemovedCbId = 0;
    }
}

/* static */
void ImagePlaneSubSceneOverride::InstanceChangedCallback(
    MDagPath &/*child*/,
    MDagPath &/*parent*/,
    void *clientData) {
    ImagePlaneSubSceneOverride *ovr = static_cast<ImagePlaneSubSceneOverride *>(clientData);
    if (ovr) {
        ovr->fInstanceDagPaths.clear();
    }
}

void ImagePlaneSubSceneOverride::update(
    MHWRender::MSubSceneContainer &container,
    const MHWRender::MFrameContext &/*frameContext*/) {
    std::uint32_t numInstances = fInstanceDagPaths.length();
    if (numInstances == 0) {
        if (!MDagPath::getAllPathsTo(fLocatorNode, fInstanceDagPaths)) {
            MStreamUtils::stdErrorStream() << "ImagePlaneSubSceneOverride: Failed to get all DAG paths.\n";
            return;
        }
        numInstances = fInstanceDagPaths.length();
    }

    if (numInstances == 0) {
        return;
    }

    // MHWRender::MShaderInstance *shader = get3dSolidShader(
    //     MHWRender::MGeometryUtilities::wireframeColor(
    //         fInstanceDagPaths[0]));
    MHWRender::MShaderInstance *shader = getImagePlaneShader();
    if (!shader) {
        MStreamUtils::stdErrorStream()
            << "ImagePlaneSubSceneOverride: Failed to get a shader.\n";
        return;
    }

    MPlug plug(fLocatorNode, ImagePlaneShape::size);
    float newMultiplier = 1.0f;
    if (!plug.isNull()) {
        MDistance sizeVal;
        if (plug.getValue(sizeVal)) {
            newMultiplier = (float) sizeVal.asCentimeters();
        }
    }

    bool updateGeometry = (container.count() == 0);
    if (fMultiplier != newMultiplier) {
        fMultiplier = newMultiplier;
        updateGeometry = true;
    }
    if (updateGeometry) {
        rebuildGeometryBuffers();
    }

    bool anyInstanceChanged = false;
    std::uint32_t numVisibleInstances = 0;
    const std::uint32_t componentsPerColor = 4; // RGBA color

    MMatrixArray instanceMatrixArray(numInstances);
    MFloatArray instanceColorArray(
        static_cast<std::uint32_t>(numInstances * componentsPerColor));

    // If expecting large numbers of instances, walking through all
    // the instances every frame to look for changes is not efficient
    // enough. Monitoring change events and changing only the required
    // instances should be done instead.
    for (uint32_t i = 0; i < numInstances; i++) {
        const MDagPath &instance = fInstanceDagPaths[i];
        if (instance.isValid() && instance.isVisible()) {
            InstanceInfo instanceInfo(
                instance.inclusiveMatrix(),
                MHWRender::MGeometryUtilities::wireframeColor(instance));

            InstanceInfoMap::iterator iter = fInstanceInfoCache.find(i);
            if (iter == fInstanceInfoCache.end() ||
                iter->second.fColor != instanceInfo.fColor ||
                !iter->second.fMatrix.isEquivalent(instanceInfo.fMatrix)) {
                if (!fAreUIDrawablesDirty &&
                    (iter == fInstanceInfoCache.end() ||
                     !iter->second.fMatrix.isEquivalent(
                         instanceInfo.fMatrix))) {
                    fAreUIDrawablesDirty = true;
                }
                anyInstanceChanged = true;
                fInstanceInfoCache[i] = instanceInfo;
            }

            instanceMatrixArray[numVisibleInstances] = instanceInfo.fMatrix;

            uint32_t idx = numVisibleInstances * componentsPerColor;
            instanceColorArray[idx + 0] = instanceInfo.fColor.r;
            instanceColorArray[idx + 1] = instanceInfo.fColor.g;
            instanceColorArray[idx + 2] = instanceInfo.fColor.b;
            instanceColorArray[idx + 3] = instanceInfo.fColor.a;

            numVisibleInstances++;
        } else {
            InstanceInfoMap::iterator iter = fInstanceInfoCache.find(i);
            if (iter != fInstanceInfoCache.end()) {
                fInstanceInfoCache.erase(i);

                anyInstanceChanged = true;
                fAreUIDrawablesDirty = true;
            }
        }
    }

    // Shrink to fit
    instanceMatrixArray.setLength(numVisibleInstances);
    instanceColorArray.setLength(numVisibleInstances * componentsPerColor);

    std::uint32_t numInstanceInfo = static_cast<uint32_t>(fInstanceInfoCache.size());
    if (numInstanceInfo != numVisibleInstances) {
        for (unsigned int i = numVisibleInstances; i < numInstanceInfo; i++) {
            fInstanceInfoCache.erase(i);
        }
        anyInstanceChanged = true;
        fAreUIDrawablesDirty = true;
    }

    bool itemsChanged = false;
    MHWRender::MRenderItem *wireItem = container.find(wireframeItemName_);
    if (!wireItem) {
        wireItem = MHWRender::MRenderItem::Create(
            wireframeItemName_,
            MHWRender::MRenderItem::DecorationItem,
            MHWRender::MGeometry::kLines);
        wireItem->setDrawMode(MHWRender::MGeometry::kWireframe);
        wireItem->depthPriority(5);
        container.add(wireItem);
        itemsChanged = true;
    }

    MHWRender::MRenderItem *shadedItem = container.find(shadedItemName_);
    if (!shadedItem) {
        shadedItem = MHWRender::MRenderItem::Create(shadedItemName_,
                                                    MHWRender::MRenderItem::DecorationItem,
                                                    MHWRender::MGeometry::kTriangles);
        shadedItem->setDrawMode((MHWRender::MGeometry::DrawMode)
                                (MHWRender::MGeometry::kShaded |
                                 MHWRender::MGeometry::kTextured));
        container.add(shadedItem);

        itemsChanged = true;
    }

    if (itemsChanged || anyInstanceChanged) {
        wireItem->setShader(shader);
        shadedItem->setShader(shader);
    }

    if (itemsChanged || updateGeometry) {
        MStatus status;
        MFnDagNode node(fLocatorNode, &status);

        ImagePlaneShape *fp = status ? dynamic_cast<ImagePlaneShape *>(node.userNode()) : nullptr;
        MBoundingBox *bounds = fp ? new MBoundingBox(fp->boundingBox()) : nullptr;

        MHWRender::MVertexBufferArray vertexBuffers;
        vertexBuffers.addBuffer("positions", fPositionBuffer);
        setGeometryForRenderItem(
                *wireItem, vertexBuffers, *fWireIndexBuffer, bounds);
        setGeometryForRenderItem(
                *shadedItem, vertexBuffers, *fShadedIndexBuffer, bounds);
        if (bounds) {
            delete bounds;
        }
    }

    if (itemsChanged || anyInstanceChanged) {
        if (!fIsInstanceMode && numInstances == 1) {
            // For multiple copies (not multiple instances), subscene
            // consolidation is enabled for static scenario, mainly to
            // improve tumbling performance.
            wireItem->setWantSubSceneConsolidation(true);
            shadedItem->setWantSubSceneConsolidation(true);

            // When not dealing with multiple instances, don't convert
            // the render items into instanced mode.  Set the matrices
            // on them directly.
            MMatrix &objToWorld = instanceMatrixArray[0];
            wireItem->setMatrix(&objToWorld);
            shadedItem->setMatrix(&objToWorld);
        } else {
            // For multiple instances, subscene conslidation should be
            // turned off so that the GPU instancing can kick in.
            wireItem->setWantSubSceneConsolidation(false);
            shadedItem->setWantSubSceneConsolidation(false);

            // If we have DAG instances of this shape then use the
            // MPxSubSceneOverride instance transform API to set up
            // instance copies of the render items.  This will be
            // faster to render than creating render items for each
            // instance, especially for large numbers of instances.
            // Note this has to happen after the geometry and shaders
            // are set, otherwise it will fail.
            setInstanceTransformArray(*wireItem, instanceMatrixArray);
            setInstanceTransformArray(*shadedItem, instanceMatrixArray);
            setInstanceTransformArray(*texturedItem, instanceMatrixArray);
            setExtraInstanceData(*wireItem, colorParameterName_, instanceColorArray);
            setExtraInstanceData(*shadedItem, colorParameterName_, instanceColorArray);

            // Once we change the render items into instance rendering
            // they can't be changed back without being deleted and
            // re-created.  So if instances are deleted to leave only
            // one remaining, just keep treating them the instance
            // way.
            fIsInstanceMode = true;
        }
    }
}

void ImagePlaneSubSceneOverride::addUIDrawables(
    MHWRender::MUIDrawManager &drawManager,
    const MHWRender::MFrameContext &/*frameContext*/) {
    MPoint pos(0.0, 0.0, 0.0);
    MColor textColor(0.1f, 0.8f, 0.8f, 1.0f);
    MString text("Open Comp Graph Maya");

    drawManager.beginDrawable();

    drawManager.setColor(textColor);
    drawManager.setFontSize(MHWRender::MUIDrawManager::kSmallFontSize);

    // MUIDrawManager assumes the object space of the original instance. If there
    // are multiple instances, each text needs to be drawn in the origin of each
    // instance, so we need to transform the coordinates from each instance's
    // object space to the original instance's object space.
    MMatrix worldInverse0 = fInstanceInfoCache[0].fMatrix.inverse();
    for (auto it = fInstanceInfoCache.begin();
         it != fInstanceInfoCache.end(); it++) {

        drawManager.text((pos * it->second.fMatrix) * worldInverse0,
                         text, MHWRender::MUIDrawManager::kCenter);
    }

    drawManager.endDrawable();

    fAreUIDrawablesDirty = false;
}

// NOTE: This will be unneeded in Maya 2019+.
bool ImagePlaneSubSceneOverride::getSelectionPath(
    const MHWRender::MRenderItem &/*renderItem*/,
    MDagPath &dagPath) const {
    if (fInstanceDagPaths.length() == 0) return false;

    // Return the first DAG path because there is no instancing in this case.
    return MDagPath::getAPathTo(fInstanceDagPaths[0].transform(), dagPath);
}

bool ImagePlaneSubSceneOverride::getInstancedSelectionPath(
    const MHWRender::MRenderItem &/*renderItem*/,
    const MHWRender::MIntersection &intersection,
    MDagPath &dagPath) const {
    unsigned int numInstances = fInstanceDagPaths.length();
    if (numInstances == 0) return false;

    // The instance ID starts from 1 for the first DAG path. We use instanceID-1
    // as the index to DAG path array returned by MFnDagNode::getAllPaths().
    int instanceId = intersection.instanceID();
    if (instanceId > (int) numInstances) return false;

    // Return the first DAG path in case of no instancing.
    if (numInstances == 1 || instanceId == -1) {
        instanceId = 1;
    }

    return MDagPath::getAPathTo(fInstanceDagPaths[instanceId - 1].transform(),
                                dagPath);
}

void ImagePlaneSubSceneOverride::rebuildGeometryBuffers() {
    ImagePlaneSubSceneOverride::deleteGeometryBuffers();

    // VertexBuffer for positions. We concatenate the shapeVerticesB and
    // shapeVerticesA positions into a single vertex buffer.  The index
    // buffers will decide which positions will be selected for each
    // render items.
    const MHWRender::MVertexBufferDescriptor vbDesc(
        "",
        MHWRender::MGeometry::kPosition,
        MHWRender::MGeometry::kFloat,
        3);
    fPositionBuffer = new MHWRender::MVertexBuffer(vbDesc);
    if (fPositionBuffer) {
        float *positions = (float *) fPositionBuffer->acquire(shapeVerticesCountA + shapeVerticesCountB, true);
        if (positions) {
            int verticesPointerOffset = 0;
            for (int currentVertex = 0;
                 currentVertex < shapeVerticesCountA + shapeVerticesCountB;
                 ++currentVertex) {
                if (currentVertex < shapeVerticesCountB) {
                    int shapeBVtx = currentVertex;
                    float x = shapeVerticesB[shapeBVtx][0] * fMultiplier;
                    float y = shapeVerticesB[shapeBVtx][1] * fMultiplier;
                    float z = shapeVerticesB[shapeBVtx][2] * fMultiplier;
                    positions[verticesPointerOffset++] = x;
                    positions[verticesPointerOffset++] = y;
                    positions[verticesPointerOffset++] = z;
                } else {
                    int shapeAVtx = currentVertex - shapeVerticesCountB;
                    float x = shapeVerticesA[shapeAVtx][0] * fMultiplier;
                    float y = shapeVerticesA[shapeAVtx][1] * fMultiplier;
                    float z = shapeVerticesA[shapeAVtx][2] * fMultiplier;
                    positions[verticesPointerOffset++] = x;
                    positions[verticesPointerOffset++] = y;
                    positions[verticesPointerOffset++] = z;
                }
            }

            fPositionBuffer->commit(positions);
        }
    }

    // IndexBuffer for the wireframe item
    fWireIndexBuffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (fWireIndexBuffer) {
        int primitiveIndex = 0;
        int startIndex = 0;
        int numPrimitive = (shapeVerticesCountB + shapeVerticesCountA) - 2;
        int numIndex = numPrimitive * 2;

        unsigned int *indices = (unsigned int *) fWireIndexBuffer->acquire(
            numIndex, true);
        if (indices) {
            for (int i = 0; i < numIndex;) {
                if (i < (shapeVerticesCountB - 1) * 2) {
                    startIndex = 0;
                    primitiveIndex = i / 2;
                } else {
                    startIndex = shapeVerticesCountB;
                    primitiveIndex = i / 2 - shapeVerticesCountB + 1;
                }
                indices[i++] = startIndex + primitiveIndex;
                indices[i++] = startIndex + primitiveIndex + 1;
            }

            fWireIndexBuffer->commit(indices);
        }
    }

    // IndexBuffer for the shaded item
    fShadedIndexBuffer = new MHWRender::MIndexBuffer(
        MHWRender::MGeometry::kUnsignedInt32);
    if (fShadedIndexBuffer) {
        int primitiveIndex = 0;
        int startIndex = 0;
        int numPrimitive = shapeVerticesCountB + shapeVerticesCountA - 4;
        int numIndex = numPrimitive * 3;

        unsigned int *indices = (unsigned int *) fShadedIndexBuffer->acquire(
            numIndex, true);
        if (indices) {
            for (int i = 0; i < numIndex;) {
                if (i < (shapeVerticesCountB - 2) * 3) {
                    startIndex = 0;
                    primitiveIndex = i / 3;
                } else {
                    startIndex = shapeVerticesCountB;
                    primitiveIndex = i / 3 - shapeVerticesCountB + 2;
                }
                indices[i++] = startIndex;
                indices[i++] = startIndex + primitiveIndex + 1;
                indices[i++] = startIndex + primitiveIndex + 2;
            }

            fShadedIndexBuffer->commit(indices);
        }
    }
}

void ImagePlaneSubSceneOverride::deleteGeometryBuffers() {
    if (fPositionBuffer) {
        delete fPositionBuffer;
        fPositionBuffer = nullptr;
    }

    if (fWireIndexBuffer) {
        delete fWireIndexBuffer;
        fWireIndexBuffer = nullptr;
    }

    if (fShadedIndexBuffer) {
        delete fShadedIndexBuffer;
        fShadedIndexBuffer = nullptr;
    }
}

} // namespace open_comp_graph_maya
