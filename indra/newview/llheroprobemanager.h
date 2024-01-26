/**
 * @file llheroprobemanager.h
 * @brief LLHeroProbeManager class declaration
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#pragma once

#include "llreflectionmap.h"
#include "llrendertarget.h"
#include "llcubemaparray.h"
#include "llcubemap.h"
#include "lldrawable.h"

class LLSpatialGroup;
class LLViewerObject;

// number of reflection probes to keep in vram
#define LL_MAX_HERO_PROBE_COUNT 2

class alignas(16) LLHeroProbeManager
{
    LL_ALIGN_NEW
public:
    enum class DetailLevel 
    {
        STATIC_ONLY = 0,
        STATIC_AND_DYNAMIC,
        REALTIME = 2
    };

    // allocate an environment map of the given resolution 
    LLHeroProbeManager();

    // release any GL state 
    void cleanup();

    // maintain reflection probes
    void update();

    // debug display, called from llspatialpartition if reflection
    // probe debug display is active
    void renderDebug();

    // call once at startup to allocate cubemap arrays
    void initReflectionMaps();

    // perform occlusion culling on all active reflection probes
    void doOcclusion();

    void registerHeroDrawable(LLVOVolume* drawablep);
    void unregisterHeroDrawable(LLVOVolume* drawablep);

    bool isViableMirror(LLFace* face) const;

    bool isMirrorPass() const { return mRenderingMirror; }

    LLPlane currentMirrorClip() const { return mCurrentClipPlane; }
    
private:
    friend class LLPipeline;
    
    // update UBO used for rendering (call only once per render pipe flush)
    void updateUniforms();

    // bind UBO used for rendering
    void setUniforms();

    // render target for cube snapshots
    // used to generate mipmaps without doing a copy-to-texture
    LLRenderTarget mRenderTarget;
    
    LLRenderTarget mHeroRenderTarget;

    std::vector<LLRenderTarget> mMipChain;

    // storage for reflection probe radiance maps (plus two scratch space cubemaps)
    LLPointer<LLCubeMapArray> mTexture;

    // vertex buffer for pushing verts to filter shaders
    LLPointer<LLVertexBuffer> mVertexBuffer;

    LLPlane mCurrentClipPlane;

    // update the specified face of the specified probe
    void updateProbeFace(LLReflectionMap* probe, U32 face, F32 near_clip);
    
    // list of active reflection maps
    std::vector<LLPointer<LLReflectionMap> > mProbes;

    // handle to UBO
    U32 mUBO = 0;

    // list of maps being used for rendering
    std::vector<LLReflectionMap*> mReflectionMaps;

    LLReflectionMap* mUpdatingProbe = nullptr;

    LLPointer<LLReflectionMap> mDefaultProbe;  // default reflection probe to fall back to for pixels with no probe influences (should always be at cube index 0)

    // number of reflection probes to use for rendering
    U32 mReflectionProbeCount;

    // resolution of reflection probes
    U32 mProbeResolution = 1024;
    
    // maximum LoD of reflection probes (mip levels - 1)
    F32 mMaxProbeLOD = 6.f;
    
    F32 mHeroProbeStrength = 1.f;
    bool mIsInTransition = false;

    // if true, reset all probe render state on the next update (for teleports and sky changes)
    bool mReset = false;

    bool mRenderingMirror = false;
    
    std::set<LLVOVolume*>               mHeroVOList;
    LLVOVolume*                         mNearestHero;
    LLFace*                             mCurrentFace;
};
