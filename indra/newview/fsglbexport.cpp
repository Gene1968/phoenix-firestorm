/**
 * @file fsglbexport.cpp
 * @brief ShareStorm: GLB export (uses same options as Collada floater; geometry path mirrors daeexport).
 *
 * ShareStorm-only settings (see app_settings/settings.xml + skins/.../floater_export_glb.xml):
 *   FSGLBExportYUp, FSGLBExportDoubleSided, FSGLBExportBlendTextured, FSGLBExportFlipTextureV
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "fsglbexport.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llcallbacklist.h"
#include "lldir.h"
#include "llfilepicker.h"
#include "llviewermenufile.h" // LLFilePickerReplyThread
#include "llimagej2c.h"
#include "llimagepng.h"
#include "llimagetga.h"
#include "llnotificationsutil.h"
#include "llselectmgr.h"
#include "lltexturecache.h"
#include "lltexturectrl.h"
#include "lltinygltfhelper.h"
#include "lluri.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewertexturelist.h"
#include "llvolume.h"
#include "fsexportperms.h"
#include "llapr.h"
#include "llviewernetwork.h"

#include "loextras.h"

namespace
{
static constexpr F32 TEXTURE_DOWNLOAD_TIMEOUT = 60.f;
static constexpr S32 EXPANDED_WIDTH = 500;
static constexpr S32 COLLAPSED_WIDTH = 250;

static const LLUUID LL_TEXTURE_TRANSPARENT("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903");
static const LLUUID LL_TEXTURE_BLANK("5748decc-f629-461c-9a36-a35a221fe21f");

static const std::string image_format_ext[] = { "tga", "png", "j2c" };
enum image_format_type
{
    ft_tga,
    ft_png,
    ft_j2c
};

class v4adapt
{
private:
    LLStrider<LLVector4a> mV4aStrider;

public:
    v4adapt(LLVector4a* vp) { mV4aStrider = vp; }
    inline LLVector3 operator[](const unsigned int i) { return LLVector3((F32*)&mV4aStrider[i]); }
};

static bool fs_read_file_binary(const std::string& path, std::vector<unsigned char>& out)
{
    S32 sz = LLAPRFile::size(path);
    if (sz <= 0)
    {
        return false;
    }
    out.resize((size_t)sz);
    return LLAPRFile::readEx(path, out.data(), 0, sz, nullptr) == sz;
}

static size_t append_buffer_aligned(std::vector<unsigned char>& blob, const void* data, size_t len, size_t align = 4)
{
    while (blob.size() % align)
    {
        blob.push_back(0);
    }
    size_t off = blob.size();
    const U8* p = static_cast<const U8*>(data);
    blob.insert(blob.end(), p, p + len);
    return off;
}

static S32 push_buffer_view(tinygltf::Model& model, S32 buffer_idx, size_t byte_offset, size_t byte_length, int target)
{
    tinygltf::BufferView bv;
    bv.buffer = buffer_idx;
    bv.byteOffset = byte_offset;
    bv.byteLength = byte_length;
    if (target)
    {
        bv.target = target;
    }
    model.bufferViews.push_back(bv);
    return (S32)model.bufferViews.size() - 1;
}

static S32 push_accessor(tinygltf::Model& model, S32 buffer_view, S32 component_type, S32 type, size_t count, size_t byte_offset = 0)
{
    tinygltf::Accessor acc;
    acc.bufferView = buffer_view;
    acc.byteOffset = byte_offset;
    acc.componentType = component_type;
    acc.count = (int)count;
    acc.type = type;
    model.accessors.push_back(acc);
    return (S32)model.accessors.size() - 1;
}

} // namespace

// ---------------- FSGlbSaver (mirrors DAESaver in daeexport.cpp) ----------------

void FSGlbSaver::add(const LLViewerObject* prim, const std::string name)
{
    mObjects.push_back(std::pair<LLViewerObject*, std::string>((LLViewerObject*)prim, name));
}

void FSGlbSaver::updateTextureInfo()
{
    mTextures.clear();
    mTextureNames.clear();

    for (obj_info_t::iterator obj_iter = mObjects.begin(); obj_iter != mObjects.end(); ++obj_iter)
    {
        LLViewerObject* obj = obj_iter->first;
        S32 num_faces = obj->getVolume()->getNumVolumeFaces();
        for (S32 face_num = 0; face_num < num_faces; ++face_num)
        {
            LLTextureEntry* te = obj->getTE(face_num);
            const LLUUID id = te->getID();

            if (std::find(mTextures.begin(), mTextures.end(), id) != mTextures.end())
            {
                continue;
            }

            mTextures.push_back(id);
            bool exportable = true;
            LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(id);
            std::string tex_name;
            std::string description;
            if (LLGridManager::getInstance()->isInSecondLife())
            {
                if (imagep->mComment.find("a") != imagep->mComment.end())
                {
                    if (LLUUID(imagep->mComment["a"]) == gAgentID)
                    {
                        exportable = true;
                    }
                }
            }
            if (exportable)
            {
                FSExportPermsCheck::canExportAsset(id, &tex_name, &description);
            }
            else
            {
                exportable = FSExportPermsCheck::canExportAsset(id, &tex_name, &description);
            }

            if (id != LL_TEXTURE_BLANK && exportable)
            {
                std::string safe_name = gDirUtilp->getScrubbedFileName(tex_name);
                std::replace(safe_name.begin(), safe_name.end(), ' ', '_');
                mTextureNames.push_back(safe_name);
            }
            else
            {
                mTextureNames.push_back(std::string());
            }
        }
    }
}

bool FSGlbSaver::skipFace(LLTextureEntry* te)
{
    return (gSavedSettings.getBOOL("DAEExportSkipTransparent")
            && (te->getColor().mV[3] < 0.01f || te->getID() == LL_TEXTURE_TRANSPARENT));
}

FSGlbSaver::MaterialInfo FSGlbSaver::getMaterial(LLTextureEntry* te)
{
    if (gSavedSettings.getBOOL("DAEExportConsolidateMaterials"))
    {
        for (S32 i = 0; i < (S32)mAllMaterials.size(); i++)
        {
            if (mAllMaterials[i].matches(te))
            {
                return mAllMaterials[i];
            }
        }
    }

    MaterialInfo ret;
    ret.textureID = te->getID();
    ret.color = te->getColor();
    ret.name = llformat("Material%d", (int)mAllMaterials.size());
    mAllMaterials.push_back(ret);
    return mAllMaterials[mAllMaterials.size() - 1];
}

void FSGlbSaver::getMaterials(LLViewerObject* obj, material_list_t* ret)
{
    S32 num_faces = obj->getVolume()->getNumVolumeFaces();
    for (S32 face_num = 0; face_num < num_faces; ++face_num)
    {
        LLTextureEntry* te = obj->getTE(face_num);

        if (skipFace(te))
        {
            continue;
        }

        MaterialInfo mat = getMaterial(te);
        if (!gSavedSettings.getBOOL("DAEExportConsolidateMaterials")
            || std::find(ret->begin(), ret->end(), mat) == ret->end())
        {
            ret->push_back(mat);
        }
    }
}

void FSGlbSaver::getFacesWithMaterial(LLViewerObject* obj, MaterialInfo& mat, int_list_t* ret)
{
    S32 num_faces = obj->getVolume()->getNumVolumeFaces();
    for (S32 face_num = 0; face_num < num_faces; ++face_num)
    {
        if (mat == getMaterial(obj->getTE(face_num)))
        {
            ret->push_back(face_num);
        }
    }
}

void FSGlbSaver::transformTexCoord(S32 num_vert, LLVector2* coord, LLVector3* positions, LLVector3* normals, LLTextureEntry* te, LLVector3 scale)
{
    F32 cosineAngle = cos(te->getRotation());
    F32 sinAngle = sin(te->getRotation());

    for (S32 ii = 0; ii < num_vert; ii++)
    {
        if (LLTextureEntry::TEX_GEN_PLANAR == te->getTexGen())
        {
            LLVector3 normal = normals[ii];
            LLVector3 pos = positions[ii];
            LLVector3 binormal;
            F32 d = normal * LLVector3::x_axis;
            if (d >= 0.5f || d <= -0.5f)
            {
                binormal = LLVector3::y_axis;
                if (normal.mV[0] < 0)
                {
                    binormal *= -1.0f;
                }
            }
            else
            {
                binormal = LLVector3::x_axis;
                if (normal.mV[1] > 0)
                {
                    binormal *= -1.0f;
                }
            }
            LLVector3 tangent = binormal % normal;
            LLVector3 scaledPos = pos.scaledVec(scale);
            coord[ii].mV[0] = 1.f + ((binormal * scaledPos) * 2.f - 0.5f);
            coord[ii].mV[1] = -((tangent * scaledPos) * 2.f - 0.5f);
        }

        F32 repeatU;
        F32 repeatV;
        te->getScale(&repeatU, &repeatV);
        F32 tX = coord[ii].mV[0] - 0.5f;
        F32 tY = coord[ii].mV[1] - 0.5f;

        F32 offsetU;
        F32 offsetV;
        te->getOffset(&offsetU, &offsetV);

        coord[ii].mV[0] = (tX * cosineAngle + tY * sinAngle) * repeatU + offsetU + 0.5f;
        coord[ii].mV[1] = (-tX * sinAngle + tY * cosineAngle) * repeatV + offsetV + 0.5f;
    }
}

bool FSGlbSaver::saveGLB(const std::string& filename)
{
    mAllMaterials.clear();
    mTotalNumMaterials = 0;

    tinygltf::Model model;
    model.asset.version = "2.0";
    std::string gen = LLVersionInfo::getInstance()->getChannelAndVersion();
    if (lolistorm_check_flag(LO_ANONYMIZE_EXPORTS))
    {
        gen = "Viewer";
    }
    model.asset.generator = gen;

    std::vector<unsigned char> bin_blob;
    std::vector<int> scene_root_nodes;

    S32 prim_nr = 0;
    for (obj_info_t::iterator obj_iter = mObjects.begin(); obj_iter != mObjects.end(); ++obj_iter)
    {
        LLViewerObject* obj = obj_iter->first;

        std::string prim_name = llformat("prim%d", prim_nr++);

        std::vector<F32> position_data;
        std::vector<F32> normal_data;
        std::vector<F32> uv_data;
        bool applyTexCoord = gSavedSettings.getBOOL("DAEExportTextureParams");

        S32 num_faces = obj->getVolume()->getNumVolumeFaces();
        for (S32 face_num = 0; face_num < num_faces; face_num++)
        {
            if (skipFace(obj->getTE(face_num)))
            {
                continue;
            }

            const LLVolumeFace* face = (LLVolumeFace*)&obj->getVolume()->getVolumeFace(face_num);

            v4adapt verts(face->mPositions);
            v4adapt norms(face->mNormals);

            LLVector2* newCoord = nullptr;

            if (applyTexCoord)
            {
                newCoord = new LLVector2[face->mNumVertices];
                LLVector3* newPos = new LLVector3[face->mNumVertices];
                LLVector3* newNormal = new LLVector3[face->mNumVertices];
                for (S32 i = 0; i < face->mNumVertices; i++)
                {
                    newPos[i] = verts[i];
                    newNormal[i] = norms[i];
                    newCoord[i] = face->mTexCoords[i];
                }
                transformTexCoord(face->mNumVertices, newCoord, newPos, newNormal, obj->getTE(face_num), obj->getScale());
                delete[] newPos;
                delete[] newNormal;
            }

            for (S32 i = 0; i < face->mNumVertices; i++)
            {
                LLVector3 v = verts[i];
                LLVector3 n = norms[i];

                position_data.push_back(v.mV[VX]);
                position_data.push_back(v.mV[VY]);
                position_data.push_back(v.mV[VZ]);

                normal_data.push_back(n.mV[VX]);
                normal_data.push_back(n.mV[VY]);
                normal_data.push_back(n.mV[VZ]);

                const LLVector2 uv = applyTexCoord ? newCoord[i] : face->mTexCoords[i];
                F32 u_tex = uv.mV[VX];
                F32 v_tex = uv.mV[VY];
                // <ShareStorm> GLB: optional V flip (SL / OpenGL-style texcoords vs many glTF importers)
                if (gSavedSettings.getBOOL("FSGLBExportFlipTextureV"))
                {
                    v_tex = 1.f - v_tex;
                }
                // </ShareStorm>
                uv_data.push_back(u_tex);
                uv_data.push_back(v_tex);
            }

            if (applyTexCoord)
            {
                delete[] newCoord;
            }
        }

        if (position_data.empty())
        {
            continue;
        }

        material_list_t obj_materials;
        getMaterials(obj, &obj_materials);

        tinygltf::Mesh mesh;
        mesh.name = prim_name;

        const size_t pos_byte_len = position_data.size() * sizeof(F32);
        const size_t norm_byte_len = normal_data.size() * sizeof(F32);
        const size_t uv_byte_len = uv_data.size() * sizeof(F32);
        const size_t vert_count = position_data.size() / 3;

        const size_t pos_off = append_buffer_aligned(bin_blob, position_data.data(), pos_byte_len);
        const size_t norm_off = append_buffer_aligned(bin_blob, normal_data.data(), norm_byte_len);
        const size_t uv_off = append_buffer_aligned(bin_blob, uv_data.data(), uv_byte_len);

        const int pos_bv = push_buffer_view(model, 0, pos_off, pos_byte_len, TINYGLTF_TARGET_ARRAY_BUFFER);
        const int norm_bv = push_buffer_view(model, 0, norm_off, norm_byte_len, TINYGLTF_TARGET_ARRAY_BUFFER);
        const int uv_bv = push_buffer_view(model, 0, uv_off, uv_byte_len, TINYGLTF_TARGET_ARRAY_BUFFER);

        const int pos_acc = push_accessor(model, pos_bv, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, vert_count);
        const int norm_acc = push_accessor(model, norm_bv, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, vert_count);
        const int uv_acc = push_accessor(model, uv_bv, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2, vert_count);

        const bool use_u32 = vert_count > 65535;

        auto add_primitive = [&](int_list_t* faces_to_include, const std::string& mat_name) {
            size_t index_offset = 0;
            std::vector<U32> indices;
            for (S32 face_num = 0; face_num < num_faces; face_num++)
            {
                if (skipFace(obj->getTE(face_num)))
                {
                    continue;
                }

                const LLVolumeFace* face = (LLVolumeFace*)&obj->getVolume()->getVolumeFace(face_num);

                if (faces_to_include == nullptr
                    || (std::find(faces_to_include->begin(), faces_to_include->end(), face_num) != faces_to_include->end()))
                {
                    for (S32 i = 0; i < face->mNumIndices; i++)
                    {
                        U32 index = (U32)(index_offset + face->mIndices[i]);
                        indices.push_back(index);
                    }
                }
                index_offset += face->mNumVertices;
            }

            if (indices.empty())
            {
                return;
            }

            size_t idx_off;
            size_t idx_byte_len;
            int idx_comp;
            if (use_u32)
            {
                idx_off = append_buffer_aligned(bin_blob, indices.data(), indices.size() * sizeof(U32));
                idx_byte_len = indices.size() * sizeof(U32);
                idx_comp = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
            }
            else
            {
                std::vector<U16> idx16(indices.size());
                for (size_t i = 0; i < indices.size(); i++)
                {
                    idx16[i] = (U16)indices[i];
                }
                idx_off = append_buffer_aligned(bin_blob, idx16.data(), idx16.size() * sizeof(U16));
                idx_byte_len = idx16.size() * sizeof(U16);
                idx_comp = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
            }

            const int idx_bv = push_buffer_view(model, 0, idx_off, idx_byte_len, TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
            const int idx_acc = push_accessor(model, idx_bv, idx_comp, TINYGLTF_TYPE_SCALAR, indices.size());

            S32 mat_idx = -1;
            for (S32 mi = 0; mi < (S32)mAllMaterials.size(); mi++)
            {
                if (mAllMaterials[(size_t)mi].name == mat_name)
                {
                    mat_idx = mi;
                    break;
                }
            }

            tinygltf::Primitive prim;
            prim.attributes["POSITION"] = pos_acc;
            prim.attributes["NORMAL"] = norm_acc;
            prim.attributes["TEXCOORD_0"] = uv_acc;
            prim.indices = idx_acc;
            prim.mode = TINYGLTF_MODE_TRIANGLES;
            if (mat_idx >= 0)
            {
                prim.material = mat_idx;
            }
            mesh.primitives.push_back(prim);
        };

        if (gSavedSettings.getBOOL("DAEExportConsolidateMaterials"))
        {
            for (S32 om = 0; om < (S32)obj_materials.size(); om++)
            {
                int_list_t faces;
                getFacesWithMaterial(obj, obj_materials[(size_t)om], &faces);
                add_primitive(&faces, obj_materials[(size_t)om].name);
            }
        }
        else
        {
            S32 mat_nr = 0;
            for (S32 face_num = 0; face_num < num_faces; face_num++)
            {
                if (skipFace(obj->getTE(face_num)))
                {
                    continue;
                }
                int_list_t faces;
                faces.push_back(face_num);
                std::string mn = obj_materials[(size_t)mat_nr++].name;
                add_primitive(&faces, mn);
            }
        }

        if (mesh.primitives.empty())
        {
            continue;
        }

        const int mesh_idx = (S32)model.meshes.size();
        model.meshes.push_back(mesh);

        LLXform srt;
        srt.setScale(obj->getScale());
        srt.setPosition(obj->getRenderPosition() + mOffset);
        srt.setRotation(obj->getRenderRotation());
        LLMatrix4 m4;
        srt.getLocalMat4(m4);

        LLMatrix4 world_y(m4);
        if (gSavedSettings.getBOOL("FSGLBExportYUp"))
        {
            LLMatrix4 fix;
            // +90° X (was −90°): matches desired glTF +Y-up orientation for this pipeline
            fix.initRotation(F_PI * 0.5f, LLVector4(1.f, 0.f, 0.f, 0.f));
            // m4math.h has matrix*matrix operator* commented out; use operator*=
            world_y = fix;
            world_y *= m4;
        }

        const int node_idx = (S32)model.nodes.size();
        tinygltf::Node node;
        node.name = prim_name;
        node.mesh = mesh_idx;
        for (int c = 0; c < 4; c++)
        {
            for (int r = 0; r < 4; r++)
            {
                node.matrix.push_back(world_y.mMatrix[r][c]);
            }
        }
        model.nodes.push_back(node);
        scene_root_nodes.push_back(node_idx);
    }

    if (scene_root_nodes.empty())
    {
        LL_WARNS("export") << "GLB export: no mesh data" << LL_ENDL;
        return false;
    }

    tinygltf::Buffer buffer;
    buffer.data = std::move(bin_blob);

    const std::string dir = gDirUtilp->getDirName(filename);
    const std::string delim = gDirUtilp->getDirDelimiter();
    const bool export_textures = gSavedSettings.getBOOL("DAEExportTextures");

    for (S32 mi = 0; mi < (S32)mAllMaterials.size(); mi++)
    {
        tinygltf::Material gl_mat;
        LLColor4 color = mAllMaterials[(size_t)mi].color;
        const F32 tint_a = color.mV[VALPHA];
        gl_mat.name = mAllMaterials[(size_t)mi].name;
        if (tint_a < 1.f - 1e-5f)
        {
            gl_mat.name += "_tintA";
            gl_mat.name += llformat("%.3f", tint_a);
        }

        // Length-4 vector so tinygltf always writes baseColorFactor (incl. SL "Texture transparency %" → tint alpha).
        {
            std::vector<double>& bcf = gl_mat.pbrMetallicRoughness.baseColorFactor;
            bcf.resize(4);
            bcf[0] = (double)color.mV[VRED];
            bcf[1] = (double)color.mV[VGREEN];
            bcf[2] = (double)color.mV[VBLUE];
            bcf[3] = (double)tint_a;
        }
        gl_mat.pbrMetallicRoughness.metallicFactor = 0.0;
        gl_mat.pbrMetallicRoughness.roughnessFactor = 1.0;

        bool embedded_base_color_tex = false;

        if (export_textures)
        {
            S32 tex_i = -1;
            for (S32 i = 0; i < (S32)mTextures.size(); i++)
            {
                if (mAllMaterials[(size_t)mi].textureID == mTextures[(size_t)i])
                {
                    tex_i = i;
                    break;
                }
            }

            if (tex_i >= 0 && !mTextureNames[(size_t)tex_i].empty())
            {
                std::string path = dir + delim + mTextureNames[(size_t)tex_i] + "." + mImageFormat;
                std::vector<unsigned char> img_bytes;
                std::string mime;
                if (mImageFormat == "png" && fs_read_file_binary(path, img_bytes))
                {
                    mime = "image/png";
                }
                else if (mImageFormat == "tga" && fs_read_file_binary(path, img_bytes))
                {
                    mime = "image/x-tga";
                }

                if (!img_bytes.empty())
                {
                    embedded_base_color_tex = true;
                    size_t img_off = append_buffer_aligned(buffer.data, img_bytes.data(), img_bytes.size(), 4);
                    tinygltf::BufferView ibv;
                    ibv.buffer = 0;
                    ibv.byteOffset = img_off;
                    ibv.byteLength = img_bytes.size();
                    model.bufferViews.push_back(ibv);
                    const int ibv_idx = (S32)model.bufferViews.size() - 1;

                    tinygltf::Image img;
                    img.mimeType = mime;
                    img.bufferView = ibv_idx;
                    model.images.push_back(img);
                    const int img_idx = (S32)model.images.size() - 1;

                    tinygltf::Texture tex;
                    tex.source = img_idx;
                    model.textures.push_back(tex);
                    const int tex_idx = (S32)model.textures.size() - 1;

                    gl_mat.pbrMetallicRoughness.baseColorTexture.index = tex_idx;
                    gl_mat.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
                }
            }
        }

        // OPAQUE mode ignores factor alpha in many DCC/importers — use BLEND whenever tint is not fully opaque.
        const bool tint_not_opaque = (tint_a < 1.f - 1e-5f);
        const bool blend_textured = gSavedSettings.getBOOL("FSGLBExportBlendTextured");
        if (tint_not_opaque || (embedded_base_color_tex && blend_textured))
        {
            gl_mat.alphaMode = "BLEND";
        }
        else
        {
            gl_mat.alphaMode = "OPAQUE";
        }

        if (gSavedSettings.getBOOL("FSGLBExportDoubleSided"))
        {
            gl_mat.doubleSided = true;
        }

        model.materials.push_back(gl_mat);
    }

    model.buffers.push_back(buffer);

    tinygltf::Scene scene;
    scene.nodes = scene_root_nodes;
    model.scenes.push_back(scene);
    model.defaultScene = 0;

    return LLTinyGLTFHelper::saveModel(filename, model);
}

// ---------------- FSFloaterExportGLB ----------------

FSFloaterExportGLB::FSFloaterExportGLB(const LLSD& key)
    : LLFloater(key),
      mCurrentObjectID(LLUUID::null),
      mDirty(true)
{
    mCommitCallbackRegistrar.add("FSGlbExport.TextureExport", boost::bind(&FSFloaterExportGLB::onTextureExportCheck, this));
}

FSFloaterExportGLB::~FSFloaterExportGLB()
{
    if (gIdleCallbacks.containsFunction(CacheReadResponder::saveTexturesWorker, this))
    {
        gIdleCallbacks.deleteFunction(CacheReadResponder::saveTexturesWorker, this);
    }
}

bool FSFloaterExportGLB::postBuild()
{
    mTitleProgress = getString("texture_progress");
    mTexturePanel = getChild<LLPanel>("textures_panel");
    childSetAction("export_btn", boost::bind(&FSFloaterExportGLB::onClickExport, this));
    LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&FSFloaterExportGLB::updateSelection, this));

    return true;
}

void FSFloaterExportGLB::draw()
{
    if (mDirty)
    {
        refresh();
        mDirty = false;
    }
    LLFloater::draw();
}

void FSFloaterExportGLB::dirty() { mDirty = true; }

void FSFloaterExportGLB::refresh()
{
    addSelectedObjects();
    onTextureExportCheck();
    addTexturePreview();
    updateUI();
}

void FSFloaterExportGLB::onOpen(const LLSD& key)
{
    LLObjectSelectionHandle object_selection = LLSelectMgr::getInstance()->getSelection();
    if (!(object_selection->getPrimaryObject()))
    {
        closeFloater();
        return;
    }
    mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
    refresh();
}

void FSFloaterExportGLB::updateTitleProgress()
{
    LLSD args;
    args["OBJECT"] = mObjectName;
    args["COUNT"] = llformat("%d", (int)mTexturesToSave.size());
    mTitleProgress.setArgs(args);
    setTitle(mTitleProgress);
}

void FSFloaterExportGLB::updateUI()
{
    childSetTextArg("NameText", "[NAME]", mObjectName);
    childSetTextArg("exportable_prims", "[COUNT]", llformat("%d", mIncluded));
    childSetTextArg("exportable_prims", "[TOTAL]", llformat("%d", mTotal));
    childSetTextArg("exportable_textures", "[COUNT]", llformat("%d", mNumExportableTextures));
    childSetTextArg("exportable_textures", "[TOTAL]", llformat("%d", mNumTextures));

    LLUIString title = getString("floater_title");
    title.setArg("[OBJECT]", mObjectName);
    setTitle(title);
    childSetEnabled("export_textures_check", mNumExportableTextures);
    childSetEnabled("export_btn", mIncluded);
}

void FSFloaterExportGLB::onClickExport()
{
    LLFilePickerReplyThread::startPicker(boost::bind(&FSFloaterExportGLB::onExportFileSelected, this, _1),
                                         LLFilePicker::FFSAVE_GLTF,
                                         LLDir::getScrubbedFileName(mObjectName + ".glb"));
}

void FSFloaterExportGLB::onExportFileSelected(const std::vector<std::string>& filenames)
{
    mFilename = filenames[0];

    if (gSavedSettings.getBOOL("DAEExportTextures"))
    {
        saveTextures();
    }
    else
    {
        onTexturesSaved();
    }
}

void FSFloaterExportGLB::onTextureExportCheck()
{
    bool show_tex_panel = (gSavedSettings.getBOOL("DAEExportTextures") && mNumExportableTextures);

    getChild<LLPanel>("tex_layout_panel")->setVisible(show_tex_panel);
    if (show_tex_panel)
    {
        reshape(EXPANDED_WIDTH, getRect().getHeight());
    }
    else
    {
        reshape(COLLAPSED_WIDTH, getRect().getHeight());
    }
}

void FSFloaterExportGLB::onTexturesSaved()
{
    bool success = mSaver.saveGLB(mFilename);
    LLSD args;
    args["OBJECT"] = mObjectName;
    args["FILENAME"] = mFilename;
    if (success)
    {
        LL_INFOS() << "GLB export successful" << LL_ENDL;
        LLNotificationsUtil::add("ExportGLBSuccess", args);
    }
    else
    {
        LL_WARNS() << "GLB export failed" << LL_ENDL;
        LLNotificationsUtil::add("ExportGLBFailure", args);
    }
    closeFloater();
}

void FSFloaterExportGLB::addSelectedObjects()
{
    mTotal = 0;
    mIncluded = 0;
    mNumTextures = 0;
    mNumExportableTextures = 0;
    mSaver.mObjects.clear();
    mSaver.mTextures.clear();
    mSaver.mTextureNames.clear();
    if (mObjectSelection)
    {
        LLSelectNode* node = mObjectSelection->getFirstRootNode();
        if (node)
        {
            mCurrentObjectID = node->getObject()->getID();
            mSaver.mOffset = -mObjectSelection->getFirstRootObject()->getRenderPosition();
            mObjectName = node->mName;

            for (LLObjectSelection::iterator iter = mObjectSelection->begin(); iter != mObjectSelection->end(); ++iter)
            {
                mTotal++;
                LLSelectNode* sn = *iter;
                mIncluded++;
                mSaver.add(sn->getObject(), sn->mName);
            }

            if (mSaver.mObjects.empty())
            {
                return;
            }
        }
        else
        {
            mObjectName = "";
        }
        mSaver.updateTextureInfo();
        mNumTextures = static_cast<S32>(mSaver.mTextures.size());
        mNumExportableTextures = getNumExportableTextures();
    }
}

void FSFloaterExportGLB::updateSelection()
{
    LLObjectSelectionHandle object_selection = LLSelectMgr::getInstance()->getSelection();
    LLSelectNode* node = object_selection->getFirstRootNode();

    if (node && !node->mValid && node->getObject()->getID() == mCurrentObjectID)
    {
        return;
    }

    mObjectSelection = object_selection;
    dirty();
    refresh();
}

S32 FSFloaterExportGLB::getNumExportableTextures() const
{
    S32 res = 0;
    for (FSGlbSaver::string_list_t::const_iterator t = mSaver.mTextureNames.begin(); t != mSaver.mTextureNames.end(); ++t)
    {
        if (!t->empty())
        {
            ++res;
        }
    }

    return res;
}

void FSFloaterExportGLB::addTexturePreview()
{
    S32 num_text = mNumExportableTextures;
    if (num_text == 0)
    {
        return;
    }
    S32 img_width = 100;
    S32 img_height = img_width + 15;
    S32 panel_height = (num_text / 2 + 1) * (img_height) + 10;
    mTexturePanel->deleteAllChildren();
    mTexturePanel->reshape(230, panel_height);
    S32 img_nr = 0;
    for (S32 i = 0; i < mSaver.mTextures.size(); i++)
    {
        if (mSaver.mTextureNames[(size_t)i].empty())
        {
            continue;
        }
        S32 left = 8 + (img_nr % 2) * (img_width + 13);
        S32 bottom = panel_height - (10 + (img_nr / 2 + 1) * (img_height));
        LLRect r(left, bottom + img_height, left + img_width, bottom);
        LLTextureCtrl::Params p;
        p.rect(r);
        p.layout("topleft");
        p.name(mSaver.mTextureNames[(size_t)i]);
        p.image_id(mSaver.mTextures[(size_t)i]);
        p.tool_tip(mSaver.mTextureNames[(size_t)i]);
        LLTextureCtrl* texture_block = LLUICtrlFactory::create<LLTextureCtrl>(p);
        mTexturePanel->addChild(texture_block);
        img_nr++;
    }
}

void FSFloaterExportGLB::saveTextures()
{
    mTexturesToSave.clear();
    for (S32 i = 0; i < mSaver.mTextures.size(); i++)
    {
        if (mSaver.mTextureNames[(size_t)i].empty())
        {
            continue;
        }

        mTexturesToSave[mSaver.mTextures[(size_t)i]] = mSaver.mTextureNames[(size_t)i];
    }

    mSaver.mImageFormat = image_format_ext[gSavedSettings.getS32("DAEExportTexturesFormat")];

    LL_DEBUGS("export") << "GLB: starting texture save" << LL_ENDL;
    mTimer.start();
    mTimer.setTimerExpirySec(TEXTURE_DOWNLOAD_TIMEOUT);
    updateTitleProgress();
    gIdleCallbacks.addFunction(CacheReadResponder::saveTexturesWorker, this);
}

FSFloaterExportGLB::CacheReadResponder::CacheReadResponder(const LLUUID& id, LLImageFormatted* image, std::string name, S32 img_type)
    : mFormattedImage(image), mID(id), mName(name), mImageType(img_type)
{
    setImage(image);
}

void FSFloaterExportGLB::CacheReadResponder::setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, bool imagelocal)
{
    if (imageformat == IMG_CODEC_TGA && mFormattedImage->getCodec() == IMG_CODEC_J2C)
    {
        LL_WARNS("export") << "GLB: texture " << mID << " TGA in cache, skipping." << LL_ENDL;
        mFormattedImage = nullptr;
        mImageSize = 0;
        return;
    }

    if (mFormattedImage.notNull())
    {
        if (mFormattedImage->getCodec() == imageformat)
        {
            mFormattedImage->appendData(data, datasize);
        }
        else
        {
            LL_WARNS("export") << "GLB: texture " << mID << " wrong format." << LL_ENDL;
            mFormattedImage = nullptr;
            mImageSize = 0;
            return;
        }
    }
    else
    {
        mFormattedImage = LLImageFormatted::createFromType(imageformat);
        mFormattedImage->setData(data, datasize);
    }
    mImageSize = imagesize;
    mImageLocal = imagelocal;
}

void FSFloaterExportGLB::CacheReadResponder::completed(bool success)
{
    if (success && mFormattedImage.notNull() && mImageSize > 0)
    {
        bool ok = false;

        if (mImageType == ft_j2c)
        {
            mName += "." + mFormattedImage->getExtension();
            ok = mFormattedImage->save(mName);
        }
        else
        {
            if (mFormattedImage->updateData()
                && ((mFormattedImage->getWidth() * mFormattedImage->getHeight() * mFormattedImage->getComponents()) != 0))
            {
                mFormattedImage->setDiscardLevel(0);

                LLPointer<LLImageRaw> raw = new LLImageRaw;
                raw->resize(mFormattedImage->getWidth(), mFormattedImage->getHeight(), mFormattedImage->getComponents());

                if (mFormattedImage->decode(raw, 0))
                {
                    LLPointer<LLImageFormatted> img = nullptr;
                    switch (mImageType)
                    {
                    case ft_tga:
                        img = new LLImageTGA;
                        break;
                    case ft_png:
                        img = new LLImagePNG;
                        break;
                    default:
                        break;
                    }

                    if (img.notNull())
                    {
                        if (img->encode(raw, 0))
                        {
                            mName += "." + img->getExtension();
                            ok = img->save(mName);
                        }
                    }
                }
            }
        }

        if (ok)
        {
            LL_DEBUGS("export") << "GLB saved texture " << mName << LL_ENDL;
        }
        else
        {
            LL_WARNS("export") << "GLB FAILED texture " << mID << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS("export") << "GLB FAILED texture " << mID << LL_ENDL;
    }
}

void FSFloaterExportGLB::CacheReadResponder::saveTexturesWorker(void* data)
{
    FSFloaterExportGLB* me = (FSFloaterExportGLB*)data;
    if (me->mTexturesToSave.size() == 0)
    {
        LL_DEBUGS("export") << "GLB textures done" << LL_ENDL;
        me->updateTitleProgress();
        gIdleCallbacks.deleteFunction(saveTexturesWorker, me);
        me->mTimer.stop();
        me->onTexturesSaved();
        return;
    }

    LLUUID id = me->mTexturesToSave.begin()->first;
    LLViewerTexture* imagep = LLViewerTextureManager::findFetchedTexture(id, TEX_LIST_STANDARD);
    if (!imagep)
    {
        me->mTexturesToSave.erase(id);
        me->updateTitleProgress();
        me->mTimer.reset();
        me->mTimer.setTimerExpirySec(TEXTURE_DOWNLOAD_TIMEOUT);
    }
    else
    {
        if (imagep->getDiscardLevel() == 0)
        {
            LL_DEBUGS("export") << "GLB saving texture " << id << LL_ENDL;
            LLImageFormatted* img = new LLImageJ2C;
            S32 img_type = gSavedSettings.getS32("DAEExportTexturesFormat");
            std::string name = gDirUtilp->getDirName(me->mFilename);
            name += gDirUtilp->getDirDelimiter() + me->mTexturesToSave[id];
            CacheReadResponder* responder = new CacheReadResponder(id, img, name, img_type);
            S32 texture_size
                = LLImageJ2C::calcDataSizeJ2C(imagep->getFullWidth(), imagep->getFullHeight(), imagep->getComponents(), 0);
            LLAppViewer::getTextureCache()->readFromCache(id, 0, texture_size, responder);
            me->mTexturesToSave.erase(id);
            me->updateTitleProgress();
            me->mTimer.reset();
            me->mTimer.setTimerExpirySec(TEXTURE_DOWNLOAD_TIMEOUT);
        }
        else if (me->mTimer.hasExpired())
        {
            LL_WARNS("export") << "GLB timed out texture " << id << LL_ENDL;
            me->mTexturesToSave.erase(id);
            me->updateTitleProgress();
            me->mTimer.reset();
            me->mTimer.setTimerExpirySec(TEXTURE_DOWNLOAD_TIMEOUT);
        }
    }
}
