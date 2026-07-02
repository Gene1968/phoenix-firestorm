/**
 * @file fsglbexport.h
 * @brief ShareStorm: export selected mesh linkset to binary glTF (.glb).
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * $/LicenseInfo$
 */

#ifndef FS_GLB_EXPORT_H
#define FS_GLB_EXPORT_H

#include "llfloater.h"
#include "lltexturecache.h"
#include "lltextureentry.h"

class LLViewerObject;
class LLObjectSelection;

/// Geometry/material collector mirroring DAESaver (daeexport) for GLB output.
class FSGlbSaver
{
public:
    struct MaterialInfo
    {
        LLUUID textureID;
        LLColor4 color;
        std::string name;

        bool matches(LLTextureEntry* te) const
        {
            return (textureID == te->getID()) && (color == te->getColor());
        }

        bool operator==(const MaterialInfo& rhs) const
        {
            return (textureID == rhs.textureID) && (color == rhs.color) && (name == rhs.name);
        }

        bool operator!=(const MaterialInfo& rhs) const { return !(*this == rhs); }

        MaterialInfo() = default;
        MaterialInfo(const MaterialInfo& rhs) = default;
        MaterialInfo& operator=(const MaterialInfo& rhs) = default;
    };

    typedef std::vector<std::pair<LLViewerObject*, std::string>> obj_info_t;
    typedef std::vector<LLUUID> id_list_t;
    typedef std::vector<std::string> string_list_t;
    typedef std::vector<S32> int_list_t;
    typedef std::vector<MaterialInfo> material_list_t;

    material_list_t mAllMaterials;
    id_list_t mTextures;
    string_list_t mTextureNames;
    obj_info_t mObjects;
    LLVector3 mOffset;
    std::string mImageFormat;
    S32 mTotalNumMaterials{ 0 };

    void updateTextureInfo();
    void add(const LLViewerObject* prim, const std::string name);
    bool saveGLB(const std::string& filename);

private:
    void transformTexCoord(S32 num_vert, LLVector2* coord, LLVector3* positions, LLVector3* normals, LLTextureEntry* te, LLVector3 scale);
    bool skipFace(LLTextureEntry* te);
    MaterialInfo getMaterial(LLTextureEntry* te);
    void getMaterials(LLViewerObject* obj, material_list_t* ret);
    void getFacesWithMaterial(LLViewerObject* obj, MaterialInfo& mat, int_list_t* ret);
};

/// Same workflow as ColladaExportFloater (texture download, DAE settings), writes .glb at the end.
class FSFloaterExportGLB : public LLFloater
{
public:
    FSFloaterExportGLB(const LLSD& key);
    bool postBuild() override;
    void updateSelection();

protected:
    void onTexturesSaved();

    LLSafeHandle<LLObjectSelection> mObjectSelection;
    LLTimer mTimer;
    typedef std::map<LLUUID, std::string> texture_list_t;
    texture_list_t mTexturesToSave;
    std::string mFilename;

private:
    ~FSFloaterExportGLB();
    void draw() override;
    void onOpen(const LLSD& key) override;
    void refresh() override;
    void dirty();
    void onClickExport();
    void onClickMakeCopy();
    void onExportFileSelected(const std::vector<std::string>& filenames);
    void onTextureExportCheck();
    void saveTextures();
    void addSelectedObjects();
    void addTexturePreview();
    void updateTitleProgress();
    void updateUI();
    S32 getNumExportableTextures() const;

    FSGlbSaver mSaver;
    S32 mTotal{ 0 };
    S32 mIncluded{ 0 };
    S32 mNumTextures{ 0 };
    S32 mNumExportableTextures{ 0 };
    std::string mObjectName;
    LLUIString mTitleProgress;
    LLPanel* mTexturePanel{ nullptr };
    LLUUID mCurrentObjectID;
    bool mDirty{ true };

    class CacheReadResponder : public LLTextureCache::ReadResponder
    {
        friend class FSFloaterExportGLB;

    private:
        LLPointer<LLImageFormatted> mFormattedImage;
        LLUUID mID;
        std::string mName;
        S32 mImageType;

    public:
        CacheReadResponder(const LLUUID& id, LLImageFormatted* image, std::string name, S32 img_type);

        void setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, bool imagelocal) override;
        void completed(bool success) override;
        static void saveTexturesWorker(void* data);
    };
};

#endif // FS_GLB_EXPORT_H
