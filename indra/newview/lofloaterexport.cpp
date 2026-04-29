// LOstorm: Shoutout to whoever originally wrote this some time last century

#include "lltextureentry.h"
#include "llviewerprecompiledheaders.h"
#include "lofloaterexport.h"

#include "llagentdata.h"
#include "llappviewer.h"
#include "llavatarappearancedefines.h"
#include "llfilepicker.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"
#include "llselectmgr.h"
#include "lltextbox.h"
#include "llviewerobjectlist.h"
#include "llviewerpartsource.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llvoavatar.h"
#include "llwearable.h"

using namespace LLAvatarAppearanceDefines;

namespace
{

class LLExportable
{
    enum EXPORTABLE_TYPE
    {
        EXPORTABLE_OBJECT,
        EXPORTABLE_WEARABLE
    };

public:
    LLExportable(LLViewerObject* object, std::string name, std::map<U32, std::pair<std::string, std::string>>& primNameMap);
    LLExportable(LLVOAvatar* avatar, LLWearableType::EType type, std::map<U32, std::pair<std::string, std::string>>& primNameMap);

    LLSD asLLSD();

    EXPORTABLE_TYPE mType;
    LLWearableType::EType mWearableType;
    LLViewerObject* mObject;
    LLVOAvatar* mAvatar;
    std::map<U32, std::pair<std::string, std::string>>& mPrimNameMap;
};

LLExportable::LLExportable(LLViewerObject* object, std::string name, std::map<U32, std::pair<std::string, std::string>>& primNameMap)
    : mObject(object),
      mType(EXPORTABLE_OBJECT),
      mPrimNameMap(primNameMap)
{
}

LLExportable::LLExportable(LLVOAvatar* avatar, LLWearableType::EType type, std::map<U32, std::pair<std::string, std::string>>& primNameMap)
    : mAvatar(avatar),
      mType(EXPORTABLE_WEARABLE),
      mWearableType(type),
      mPrimNameMap(primNameMap)
{
}

/*
    CURRENT STATE OF THIS EXPORT FEATURE:

    The point of this feature is for compatibility with tools which process traditional XML exports.
    Specifically, it is written to meet the expectations of MeshFix v2.0.4

    This XML format lacks a few features that exist in Firestorm OXP exports:
        - Exporter metadata (exporter name, grid name, client name, export date, etc.)
        - The "linkset" array, for reliable ordering when re-linking during import
        - The "asset" map, containing embedded textures, materials, notecards, and scripts
        - The "inventory" map, containing a list of assets in the object's inventory
        - Object permission masks
        - The object properties "flags", "material", "clickaction", "sale_info", "touch_name", and "sit_name"
            - "flags" is a full set of object flags (partially redundant with phantom/physical)
            - "material" is the prim material that controls sound effects (e.g. LL_MCODE_WOOD)
            - ... and the rest are relatively self explanatory
        - The optional "particle" map
            - See LLPartSysData::asLLSD()
        - The optional "ExtraPhysics" map
            - (Density, Friction, GravityMultiplier, PhysicsShapeType and Restitution)
        - The optional "extended_mesh", "render_material" and "reflection_probe" maps
            - (Custom LOstorm additions, not supported in Vanilla OXP)
        - The optional texture property "gltf_override"
            - This contains some of the PBR data
            - This has to be stripped out, as MeshFix chokes on it :(
*/
LLSD LLExportable::asLLSD()
{
    if (mType == EXPORTABLE_OBJECT)
    {
        std::list<LLViewerObject*> prims;

        prims.push_back(mObject);

        for (LLViewerObject* child : mObject->getChildren())
        {
            if (child->getPCode() < LL_PCODE_APP)
                prims.push_back(child);
        }

        LLSD llsd;

        for (LLViewerObject* object : prims)
        {
            LLSD prim_llsd;

            prim_llsd["type"] = object->isMesh() ? "mesh" : "prim";

            if (!object->isRoot() && !object->getSubParent()->isAvatar())
                prim_llsd["parent"] = llformat("%d", object->getSubParent()->getLocalID());
            else if (object->getSubParent() && object->getSubParent()->isAvatar())
                prim_llsd["attach"] = ATTACHMENT_ID_FROM_STATE(object->getAttachmentState());

            // Transforms
            prim_llsd["position"] = object->getPosition().getValue();
            prim_llsd["scale"] = object->getScale().getValue();
            prim_llsd["rotation"] = ll_sd_from_quaternion(object->getRotation());

            // Flags
            prim_llsd["phantom"] = object->flagPhantom() ? 1 : 0;
            prim_llsd["physical"] = object->flagUsePhysics() ? 1 : 0;

            // Volume params (path, profile and sculpt)
            LLVolumeParams params = object->getVolume()->getParams();
            prim_llsd["volume"] = params.asLLSD();

            // Flexible
            if (object->isFlexible())
            {
                LLFlexibleObjectData* flex = (LLFlexibleObjectData*)object->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
                prim_llsd["flexible"] = flex->asLLSD();
            }

            // Light
            if (object->getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT))
            {
                LLLightParams* light = (LLLightParams*)object->getParameterEntry(LLNetworkData::PARAMS_LIGHT);
                prim_llsd["light"] = light->asLLSD();
            }

            // Sculpt
            if (object->isSculpted())
            {
                LLSculptParams* sculpt = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
                prim_llsd["sculpt"] = sculpt->asLLSD();
            }

            // Textures
            LLSD te_llsd;
            U8 te_count = object->getNumTEs();
            for (U8 i = 0; i < te_count; i++)
            {
                LLTextureEntry* te = object->getTE(i);
                llassert(te != NULL);
                LLSD llsd = te->asLLSD();

                // Omit this field, as it breaks MeshFix
                llsd.erase("gltf_override");

                // Insert optional additional (non-PBR) material properties
                LLMaterialPtr mat_ptr = te->getMaterialParams();

                if (!mat_ptr.isNull())
                {
                    const LLMaterial& mat = *mat_ptr;

                    llsd["normid"] = mat.getNormalID();
                    llsd["normrot"] = mat.getNormalRotation();
                    llsd["norm_offsets"] = mat.getNormalOffsetX();
                    llsd["norm_offsett"] = mat.getNormalOffsetY();
                    llsd["norm_scales"] = mat.getNormalRepeatX();
                    llsd["norm_scalet"] = mat.getNormalRepeatY();

                    llsd["specid"] = mat.getSpecularID();
                    llsd["specrot"] = mat.getSpecularRotation();
                    llsd["spec_offsets"] = mat.getSpecularOffsetX();
                    llsd["spec_offsett"] = mat.getSpecularOffsetY();
                    llsd["spec_scales"] = mat.getSpecularRepeatX();
                    llsd["spec_scalet"] = mat.getSpecularRepeatY();
                    llsd["spec_color"] = ll_sd_from_color4(mat.getSpecularLightColor());
                    llsd["gloss"] = (S32)mat.getSpecularLightExponent();

                    llsd["env"] = (S32)mat.getEnvironmentIntensity();
                    llsd["alpha_mode"] = (S32)mat.getDiffuseAlphaMode();
                    llsd["cutoff"] = (S32)mat.getAlphaMaskCutoff();
                }

                // MeshFix wants 0 for default and 1 for planar
                llsd["texgen"] = te->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR ? 1 : 0;

                te_llsd.append(llsd);
            }
            prim_llsd["textures"] = te_llsd;

            auto pos = mPrimNameMap.find(object->getLocalID());
            if (pos != mPrimNameMap.end())
            {
                prim_llsd["name"] = mPrimNameMap[object->getLocalID()].first;
                prim_llsd["description"] = mPrimNameMap[object->getLocalID()].second;
            }
            else
            {
                prim_llsd["name"] = "Object";
                prim_llsd["description"] = "";
            }

            llsd[llformat("%d", object->getLocalID())] = prim_llsd;
        }

        return llsd;
    }
    else if (mType == EXPORTABLE_WEARABLE)
    {
        LLSD item_sd; // map for wearable

        item_sd["type"] = "wearable";

        S32 type_s32 = (S32)mWearableType;
        std::string wearable_name = LLWearableType::getInstance()->getTypeName(mWearableType);

        item_sd["name"] = mAvatar->getFullname() + " " + wearable_name;
        item_sd["wearabletype"] = type_s32;

        LLSD param_map;

        for (LLVisualParam* param = mAvatar->getFirstVisualParam(); param; param = mAvatar->getNextVisualParam())
        {
            LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
            // XXX: This is missing color tints, since it only accepts integer parameters
            if ((viewer_param->getWearableType() == type_s32) &&
                (viewer_param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE))
            {
                param_map[llformat("%d", viewer_param->getID())] = viewer_param->getWeight();
            }
        }

        item_sd["params"] = param_map;

        LLSD textures_map;

        for (S32 te = 0; te < TEX_NUM_INDICES; te++)
        {
            if (LLAvatarAppearance::getDictionary()->getTEWearableType((ETextureIndex)te) == mWearableType)
            {
                LLViewerTexture* te_image = mAvatar->getTEImage(te);
                if (te_image)
                {
                    textures_map[llformat("%d", te)] = te_image->getID();
                }
            }
        }

        item_sd["textures"] = textures_map;

        LLSD result;
        result[LLUUID::generateNewID().asString()] = item_sd;
        return result;
    }
    return LLSD();
}

}

LOFloaterExport::LOFloaterExport(const LLSD& key)
    : LLFloater(key)
{
}

LOFloaterExport::~LOFloaterExport()
{
}

bool LOFloaterExport::postBuild()
{
    childSetAction("select_all_btn", onClickSelectAll, this);
    childSetAction("select_objects_btn", onClickSelectObjects, this);
    childSetAction("select_meshes_btn", onClickSelectMeshes, this);
    childSetAction("select_wearables_btn", onClickSelectWearables, this);

    childSetAction("save_as_btn", onClickSaveAs, this);

    repopulate();
    return TRUE;
}

// llappviewermenu.cpp
LLVOAvatar* find_avatar_from_object(LLViewerObject* object);

void LOFloaterExport::repopulate()
{
    LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
    LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

    if (!selection) return;

    if (!mExportables.empty())
    {
        mPrimList.clear();
        mPrimNameMap.clear();
        mExportables.clear();
        list->clearRows();
    }

    // Populate prim name map
    for (auto iter = selection->valid_begin(); iter != selection->valid_end(); iter++)
    {
        LLSelectNode* nodep = *iter;
        LLViewerObject* objectp = nodep->getObject();
        U32 localid = objectp->getLocalID();
        std::string name = nodep->mName;
        mPrimNameMap[localid] = std::pair<std::string, std::string>(name, nodep->mDescription);
    }

    std::unordered_set<LLViewerObject*> avatars;

    // Ensure the selected avatar is added, even if no objects are selected
    LLVOAvatar* selectedAvatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
    if (selectedAvatar)
        avatars.insert(selectedAvatar);

    for (auto iter = selection->valid_root_begin(); iter != selection->valid_root_end(); iter++)
    {
        LLSelectNode* nodep = *iter;
        LLViewerObject* objectp = nodep->getObject();
        std::string objectp_id = llformat("%d", objectp->getLocalID());

        if (list->getItemIndex(objectp->getID()) == -1)
        {
            bool is_attachment = false;
            bool is_valid_root = true;
            LLViewerObject* parentp = objectp->getSubParent();
            if (parentp)
            {
                if (!parentp->isAvatar())
                {
                    // parent is a prim I guess
                    is_valid_root = false;
                }
                else
                {
                    // parent is an avatar
                    is_attachment = true;
                    avatars.insert(parentp);
                }
            }

            if (objectp->getPCode() >= LL_PCODE_APP)
                is_valid_root = false;

            bool is_avatar = objectp->isAvatar();

            if (is_valid_root)
            {
                LLSD element;
                element["id"] = objectp->getID();

                LLSD& check_column = element["columns"][LIST_CHECKED];
                check_column["column"] = "checked";
                check_column["type"] = "checkbox";
                check_column["value"] = true;

                LLSD& type_column = element["columns"][LIST_TYPE];
                type_column["column"] = "type";
                type_column["type"] = "icon";
                type_column["value"] = objectp->isMesh() ? "Inv_Mesh" : "Inv_Object";

                LLSD& name_column = element["columns"][LIST_NAME];
                name_column["column"] = "name";
                if (is_attachment)
                {
                    std::string attachPointName = LLAvatarAppearance::getAttachmentPointName(ATTACHMENT_ID_FROM_STATE(objectp->getAttachmentState()));
                    name_column["value"] = nodep->mName + " (worn on " + utf8str_tolower(attachPointName) + ")";
                }
                else
                {
                    name_column["value"] = nodep->mName;
                }

                LLSD& avatarid_column = element["columns"][LIST_AVATARID];
                avatarid_column["column"] = "avatarid";
                if (is_attachment)
                    avatarid_column["value"] = parentp->getID();
                else
                    avatarid_column["value"] = LLUUID::null;

                LLExportable* exportable = new LLExportable(objectp, nodep->mName, mPrimNameMap);
                mExportables[objectp->getID()] = exportable->asLLSD();

                list->addElement(element, ADD_BOTTOM);

                addToPrimList(objectp);
            }
            else if (is_avatar)
            {
                avatars.insert(objectp);
            }
        }
    }

    for (LLViewerObject* avatar : avatars)
    {
        addAvatarStuff(avatar->asAvatar());
    }

    updateNamesProgress();

    return;
}

void LOFloaterExport::addAvatarStuff(LLVOAvatar* avatarp)
{
    if (!avatarp) return;

    LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
    for (S32 type = LLWearableType::WT_SHAPE; type < LLWearableType::WT_COUNT; type++)
    {
        // guess whether this wearable actually exists
        // by checking whether it has any textures that aren't default
        bool exists = false;
        if (type == LLWearableType::WT_SHAPE)
        {
            exists = true;
        }
        else
        {
            for (S32 te = 0; te < TEX_NUM_INDICES; te++)
            {
                if ((S32)LLAvatarAppearance::getDictionary()->getTEWearableType((ETextureIndex)te) == type)
                {
                    LLViewerTexture* te_image = avatarp->getTEImage(te);
                    if (te_image->getID() != IMG_DEFAULT_AVATAR)
                    {
                        exists = true;
                        break;
                    }
                }
            }
        }

        if (exists)
        {
            std::string wearable_name = LLWearableType::getInstance()->getTypeName((LLWearableType::EType)type);
            std::string name = avatarp->getFullname() + " " + wearable_name;
            LLUUID myid;
            myid.generate();

            LLSD element;
            element["id"] = myid;

            LLSD& check_column = element["columns"][LIST_CHECKED];
            check_column["column"] = "checked";
            check_column["type"] = "checkbox";
            check_column["value"] = false;

            LLSD& type_column = element["columns"][LIST_TYPE];
            type_column["column"] = "type";
            type_column["type"] = "icon";
            const char* inv_icon = "Inv_Invalid";
            switch (type)
            {
                case LLWearableType::WT_SHAPE: inv_icon = "Inv_BodyShape"; break;
                case LLWearableType::WT_SKIN: inv_icon = "Inv_Skin"; break;
                case LLWearableType::WT_HAIR: inv_icon = "Inv_Hair"; break;
                case LLWearableType::WT_EYES: inv_icon = "Inv_Eye"; break;
                case LLWearableType::WT_SHIRT: inv_icon = "Inv_Shirt"; break;
                case LLWearableType::WT_PANTS: inv_icon = "Inv_Pants"; break;
                case LLWearableType::WT_SHOES: inv_icon = "Inv_Shoe"; break;
                case LLWearableType::WT_SOCKS: inv_icon = "Inv_Socks"; break;
                case LLWearableType::WT_JACKET: inv_icon = "Inv_Jacket"; break;
                case LLWearableType::WT_GLOVES: inv_icon = "Inv_Gloves"; break;
                case LLWearableType::WT_UNDERSHIRT: inv_icon = "Inv_Undershirt"; break;
                case LLWearableType::WT_UNDERPANTS: inv_icon = "Inv_Underpants"; break;
                case LLWearableType::WT_SKIRT: inv_icon = "Inv_Skirt"; break;
                case LLWearableType::WT_ALPHA: inv_icon = "Inv_Alpha"; break;
                case LLWearableType::WT_TATTOO: inv_icon = "Inv_Tattoo"; break;
                case LLWearableType::WT_PHYSICS: inv_icon = "Inv_Physics"; break;
                case LLWearableType::WT_UNIVERSAL: inv_icon = "Inv_Universal"; break;
            }
            type_column["value"] = inv_icon;

            LLSD& name_column = element["columns"][LIST_NAME];
            name_column["column"] = "name";
            name_column["value"] = name;

            LLExportable* exportable = new LLExportable(avatarp, (LLWearableType::EType)type, mPrimNameMap);
            mExportables[myid] = exportable->asLLSD();

            list->addElement(element, ADD_BOTTOM);
        }
    }

    // Add attachments on the avatar
    // Object names are defaulted to "Object" until retrieved later
    for (LLViewerObject* childp : avatarp->getChildren())
    {
        if (list->getItemIndex(childp->getID()) == -1)
        {
            LLSD element;
            element["id"] = childp->getID();

            LLSD& check_column = element["columns"][LIST_CHECKED];
            check_column["column"] = "checked";
            check_column["type"] = "checkbox";
            check_column["value"] = false;

            LLSD& type_column = element["columns"][LIST_TYPE];
            type_column["column"] = "type";
            type_column["type"] = "icon";
            type_column["value"] = childp->isMesh() ? "Inv_Mesh" : "Inv_Object";

            LLSD& name_column = element["columns"][LIST_NAME];
            name_column["column"] = "name";
            std::string attachPointName = LLAvatarAppearance::getAttachmentPointName(ATTACHMENT_ID_FROM_STATE(childp->getAttachmentState()));
            name_column["value"] = "Object (worn on " + utf8str_tolower(attachPointName) + ")";

            LLSD& avatarid_column = element["columns"][LIST_AVATARID];
            avatarid_column["column"] = "avatarid";
            avatarid_column["value"] = avatarp->getID();

            LLExportable* exportable = new LLExportable(childp, "Object", mPrimNameMap);
            mExportables[childp->getID()] = exportable->asLLSD();

            list->addElement(element, ADD_BOTTOM);

            addToPrimList(childp);
        }
    }

    // Request the names of attachments on the avatar
    if (!mPrimList.empty())
    {
        requestObjectProperties(mPrimList, true, avatarp->getRegion());
        requestObjectProperties(mPrimList, false, avatarp->getRegion());
    }
}

// Copied from FSAreaSearch::requestObjectProperties
void LOFloaterExport::requestObjectProperties(const std::vector<U32>& request_list, bool select, LLViewerRegion* regionp)
{
    // max number of objects that can be (de-)selected in a single packet.
    constexpr S32 MAX_OBJECTS_PER_PACKET = 255;

    bool start_new_message = true;
    S32 select_count = 0;
    LLMessageSystem* msg = gMessageSystem;

    for (U32 id : request_list)
    {
        if (start_new_message)
        {
            msg->newMessageFast(select ? _PREHASH_ObjectSelect : _PREHASH_ObjectDeselect);
            msg->nextBlockFast(_PREHASH_AgentData);
            msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
            msg->addUUIDFast(_PREHASH_SessionID, gAgentSessionID);
            select_count++;
            start_new_message = false;
        }

        msg->nextBlockFast(_PREHASH_ObjectData);
        msg->addU32Fast(_PREHASH_ObjectLocalID, id);
        select_count++;

        if (msg->isSendFull(NULL) || select_count >= MAX_OBJECTS_PER_PACKET)
        {
            msg->sendReliable(regionp->getHost());
            select_count = 0;
            start_new_message = true;
        }
    }

    if (!start_new_message)
        msg->sendReliable(regionp->getHost());
}

void LOFloaterExport::selectAllGeneric(bool(*pred)(const std::string&))
{
    LLScrollListCtrl* list = this->getChild<LLScrollListCtrl>("export_list");
    std::vector<LLScrollListItem*> items = list->getAllData();
    bool new_value = false;

    // If any matching object is not already checked, this is select (new value = true)
    // Otherwise, it is de-select (new value = false)
    for (LLScrollListItem* item : items)
    {
        std::string type = item->getColumn(LIST_TYPE)->getValue().asString();
        bool isChecked = item->getColumn(LIST_CHECKED)->getValue().asBoolean();
        if (pred(type) && !isChecked)
        {
            new_value = true;
            break;
        }
    }

    for (LLScrollListItem* item : items)
    {
        std::string type = item->getColumn(LIST_TYPE)->getValue().asString();
        if (pred(type))
            item->getColumn(LIST_CHECKED)->setValue(new_value);
    }
}

//static
void LOFloaterExport::onClickSelectAll(void* user_data)
{
    LOFloaterExport* floater = (LOFloaterExport*)user_data;
    floater->selectAllGeneric([](const std::string& type) { return true; });
}

//static
void LOFloaterExport::onClickSelectObjects(void* user_data)
{
    LOFloaterExport* floater = (LOFloaterExport*)user_data;
    floater->selectAllGeneric([](const std::string& type)
    {
        return type == "Inv_Object" || type == "Inv_Mesh";
    });
}

//static
void LOFloaterExport::onClickSelectMeshes(void* user_data)
{
    LOFloaterExport* floater = (LOFloaterExport*)user_data;
    floater->selectAllGeneric([](const std::string& type)
    {
        return type == "Inv_Mesh";
    });
}

//static
void LOFloaterExport::onClickSelectWearables(void* user_data)
{
    LOFloaterExport* floater = (LOFloaterExport*)user_data;
    floater->selectAllGeneric([](const std::string& type)
    {
        return type != "Inv_Object" && type != "Inv_Mesh";
    });
}

LLSD LOFloaterExport::getLLSD()
{
    LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
    std::vector<LLScrollListItem*> items = list->getAllData();
    LLSD result;

    for (LLScrollListItem* item : items)
    {
        if (item->getColumn(LIST_CHECKED)->getValue())
        {
            LLSD item_sd = mExportables[item->getUUID()];
            LLSD::map_iterator map_iter = item_sd.beginMap();
            LLSD::map_iterator map_end = item_sd.endMap();
            for (; map_iter != map_end; ++map_iter)
            {
                std::string key((*map_iter).first);
                LLSD data = (*map_iter).second;
                // copy it...
                result[key] = data;
            }
        }
    }

    return result;
}

//static
void LOFloaterExport::onClickSaveAs(void* user_data)
{
    LOFloaterExport* floater = (LOFloaterExport*)user_data;
    LLSD sd = floater->getLLSD();

    if (sd.size())
    {
        std::string default_filename = "untitled";

        // count the number of selected items
        LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("export_list");
        std::vector<LLScrollListItem*> items = list->getAllData();
        int item_count = 0;
        LLUUID avatarid = (*(items.begin()))->getColumn(LIST_AVATARID)->getValue().asUUID();
        bool all_same_avatarid = true;

        for (LLScrollListItem* item : items)
        {
            if (item->getColumn(LIST_CHECKED)->getValue())
            {
                item_count++;
                if (item->getColumn(LIST_AVATARID)->getValue().asUUID() != avatarid)
                {
                    all_same_avatarid = false;
                }
            }
        }

        if (item_count == 1)
        {
            // Exporting one item?  Use its name for the filename.
            // But make sure it's a root!
            LLSD target = (*(sd.beginMap())).second;
            if (target.has("parent"))
            {
                std::string parentid = target["parent"].asString();
                target = sd[parentid];
            }
            if (target.has("name"))
            {
                if (target["name"].asString().length() > 0)
                {
                    default_filename = target["name"].asString();
                }
            }
        }
        else
        {
            // Multiple items?
            // If they're all part of the same avatar, use the avatar's name as filename.
            if (all_same_avatarid)
            {
                std::string fullname;
                if (gCacheName->getFullName(avatarid, fullname))
                {
                    default_filename = fullname;
                }
            }
        }

        LLFilePicker& file_picker = LLFilePicker::instance();
        if (file_picker.getSaveFile(LLFilePicker::FFSAVE_XML, LLDir::getScrubbedFileName(default_filename + ".xml")))
        {
            std::string file_name = file_picker.getFirstFile();

            // set correct names within llsd
            LLSD::map_iterator map_iter = sd.beginMap();
            LLSD::map_iterator map_end = sd.endMap();
            std::list<LLUUID> textures;

            for (; map_iter != map_end; ++map_iter)
            {
                std::istringstream keystr((*map_iter).first);
                U32 key;
                keystr >> key;
                LLSD& item = (*map_iter).second;
                std::string item_type = item["type"].asString();
                if (item_type == "prim" || item_type == "mesh")
                {
                    std::string name = floater->mPrimNameMap[key].first;
                    item["name"] = name;
                    item["description"] = floater->mPrimNameMap[key].second;
                }
            }

            llofstream export_file(file_name);
            LLSDSerialize::toPrettyXML(sd, export_file);
            export_file.close();

            LLSD args;
            args["FILENAME"] = file_name;
            LLNotificationsUtil::add("ExportFinished", args);
        }
    }
    else
    {
        LLSD args;
        args["MESSAGE"] = "No exportable items selected";
        LLNotificationsUtil::add("GenericAlert", args);
        return;
    }

    floater->closeFloater();
}

void LOFloaterExport::addToPrimList(LLViewerObject* object)
{
    mPrimList.push_back(object->getLocalID());
    for (LLViewerObject* child : object->getChildren())
    {
        if (child->getPCode() < LL_PCODE_APP)
        {
            mPrimList.push_back(child->getLocalID());
        }
    }
}

void LOFloaterExport::updateNamesProgress()
{
    LLTextBox* ctrl = getChild<LLTextBox>("names_progress_text");
    size_t a = mPrimNameMap.size();
    size_t b = mPrimList.size();
    if (a != b)
        ctrl->setText(llformat("Retrieving names (%d of %d ...)", mPrimNameMap.size(), mPrimList.size()));
    else
        ctrl->setText(std::string());
}

void LOFloaterExport::receivePrimName(LLViewerObject* object, std::string name, std::string desc)
{
    LLUUID fullid = object->getID();
    U32 localid = object->getLocalID();
    if (std::find(mPrimList.begin(), mPrimList.end(), localid) != mPrimList.end())
    {
        mPrimNameMap[localid] = std::pair<std::string, std::string>(name, desc);
        LLScrollListCtrl* list = getChild<LLScrollListCtrl>("export_list");
        S32 item_index = list->getItemIndex(fullid);
        if (item_index != -1)
        {
            for (LLScrollListItem* item : list->getAllData())
            {
                if (item->getUUID() == fullid)
                {
                    U8 attach_state = object->getAttachmentState();
                    if (attach_state != 0)
                    {
                        std::string attachPointName = LLAvatarAppearance::getAttachmentPointName(ATTACHMENT_ID_FROM_STATE(attach_state));
                        item->getColumn(LIST_NAME)->setValue(name + " (worn on " + utf8str_tolower(attachPointName) + ")");
                    }
                    else
                    {
                        item->getColumn(LIST_NAME)->setValue(name);
                    }
                    break;
                }
            }
        }
        updateNamesProgress();
    }
}

void LOFloaterExport::processObjectProperties(LLMessageSystem* msg)
{
    S32 count = msg->getNumberOfBlocksFast(_PREHASH_ObjectData);
    for (S32 i = 0; i < count; i++)
    {
        LLUUID uuid;
        msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, uuid, i);
        if (uuid.isNull())
            continue;

        LLViewerObject* objectp = gObjectList.findObject(uuid);
        if (!objectp)
            continue;

        std::string name;
        std::string desc;

        msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, name, i);
        msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, desc, i);

        this->receivePrimName(objectp, name, desc);
    }
}
