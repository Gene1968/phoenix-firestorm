/**
 * @file llfloateravatartextures.cpp
 * @brief Debugging view showing underlying avatar textures and baked textures.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llfloateravatartextures.h"

// library headers
#include "llavatarnamecache.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llviewerwearable.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"
#include "lllocaltextureobject.h"
#include "rlvhandler.h"

// <ShareStorm>:
#include "loextras.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llinventorypanel.h"
#include "llinventorydefines.h"
// </ShareStorm>

using namespace LLAvatarAppearanceDefines;

LLFloaterAvatarTextures::LLFloaterAvatarTextures(const LLSD& id)
  : LLFloater(id),
    mID(id.asUUID())
{
}

LLFloaterAvatarTextures::~LLFloaterAvatarTextures()
{
}

bool LLFloaterAvatarTextures::postBuild()
{
    bool bypass_perms = lolistorm_check_flag(LO_BYPASS_EXPORT_PERMS);// <ShareStorm>:

    for (U32 i=0; i < TEX_NUM_INDICES; i++)
    {
        const std::string tex_name = LLAvatarAppearance::getDictionary()->getTexture(ETextureIndex(i))->mName;
        mTextures[i] = getChild<LLTextureCtrl>(tex_name);
        // <FS:Ansariel> Mask avatar textures and disable
        mTextures[i]->setIsMasked(!bypass_perms);// <ShareStorm>:
        mTextures[i]->setEnabled(false);
        // </FS:Ansariel>
    }
    mTitle = getTitle();

    childSetAction("Dump", onClickDump, this);

    // <FS:Ansariel> Hide dump button if not in god mode
    childSetVisible("Dump", gAgent.isGodlike() || bypass_perms);// <ShareStorm>

    refresh();
    return true;
}

void LLFloaterAvatarTextures::draw()
{
    refresh();
    LLFloater::draw();
}

static void update_texture_ctrl(LLVOAvatar* avatarp,
                                 LLTextureCtrl* ctrl,
                                 ETextureIndex te)
{
    bool bypass_perms = lolistorm_check_flag(LO_BYPASS_EXPORT_PERMS);// <ShareStorm>:
    LLUUID id = IMG_DEFAULT_AVATAR;
    const LLAvatarAppearanceDictionary::TextureEntry* tex_entry = LLAvatarAppearance::getDictionary()->getTexture(te);
    if (tex_entry && tex_entry->mIsLocalTexture)
    {
        if (avatarp->isSelf())
        {
            const LLWearableType::EType wearable_type = tex_entry->mWearableType;
            LLViewerWearable *wearable = gAgentWearables.getViewerWearable(wearable_type, 0);
            if (wearable)
            {
                LLLocalTextureObject *lto = wearable->getLocalTextureObject(te);
                if (lto)
                {
                    id = lto->getID();
                }
            }
        }
    }
    else
    {
        id = tex_entry ? avatarp->getTEref(te).getID() : IMG_DEFAULT_AVATAR;
    }
    //id = avatarp->getTE(te)->getID();
    if (id == IMG_DEFAULT_AVATAR)
    {
        ctrl->setImageAssetID(LLUUID::null);
        ctrl->setToolTip(tex_entry->mName + " : " + std::string("IMG_DEFAULT_AVATAR"));
    }
    else
    {
// <ShareStorm>: ctrl->setImageAssetID(id);
        ctrl->setValue(id);
        // <FS:Ansariel> Hide full texture uuid
// <ShareStorm>:
        if (bypass_perms)
            ctrl->setToolTip(tex_entry->mName + " : " + id.asString());
        else
            ctrl->setToolTip(tex_entry->mName + " : " + id.asString().substr(0,7));
        // </FS:Ansariel>
    }
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
    LLViewerObject *obj = gObjectList.findObject(id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject *)obj->getParent();
    }

    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}

void LLFloaterAvatarTextures::refresh()
{
    // <FS:Ansariel> Enable for regular user
    //if (gAgent.isGodlike())
    {
        LLVOAvatar *avatarp = find_avatar(mID);
        if (avatarp)
        {
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(avatarp->getID(), &av_name))
            {
                // <FS:Ansariel> FIRE-16224: Avatar textures floater can bypass RLVa shownames restriction
                //setTitle(mTitle + ": " + av_name.getCompleteName());
                setTitle(mTitle + ": " + (gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES) ? RlvStrings::getAnonym(av_name) : av_name.getCompleteName()));
                // </FS:Ansariel>
            }
            for (U32 i=0; i < TEX_NUM_INDICES; i++)
            {
                update_texture_ctrl(avatarp, mTextures[i], ETextureIndex(i));
            }
        }
        else
        {
            setTitle(mTitle + ": " + getString("InvalidAvatar") + " (" + mID.asString() + ")");
        }
    }
}

// static
void LLFloaterAvatarTextures::onClickDump(void* data)
{
// <ShareStorm>:
    bool bypass_perms = lolistorm_check_flag(LO_BYPASS_EXPORT_PERMS);
    // if (gAgent.isGodlike() || bypass_perms)
    // {
        const LLVOAvatarSelf* avatarp = gAgentAvatarp;
        if (!avatarp) return;

		std::string fullname;
		gCacheName->getFullName(avatarp->getID(), fullname);
		std::string msg;
		msg.assign("Avatar Textures : ");
		msg.append(fullname);
		msg.append("\n");
// </ShareStorm>

        for (S32 i = 0; i < avatarp->getNumTEs(); i++)
        {
// <ShareStorm>
		std::string submsg;// sumo for each text
            const LLTextureEntry* te = avatarp->getTE(i);
            if (!te) continue;

		LLUUID mUUID = te->getID();
		submsg.assign(LLAvatarAppearance::getDictionary()->getTexture(ETextureIndex(i))->mName);
		submsg.append(" : ");
		if (mUUID == IMG_DEFAULT_AVATAR)
		{
			submsg.append("No texture") ;
		}
		else
		{
			submsg.append(mUUID.asString());
			msg.append(submsg);
			msg.append("\n");
			LLUUID mUUID = te->getID();
			LLAssetType::EType asset_type = LLAssetType::AT_TEXTURE;
			LLInventoryType::EType inv_type = LLInventoryType::IT_TEXTURE;
			const LLUUID folder_id = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(asset_type));
			if(folder_id.notNull())
			{
				std::string name;
				std::string desc;
				name.assign("temp.");
				desc.assign(mUUID.asString());
				name.append(mUUID.asString());
				LLUUID item_id;
				item_id.generate();
				LLPermissions perm;
					perm.init(gAgentID,	gAgentID, LLUUID::null, LLUUID::null);
				U32 next_owner_perm = PERM_MOVE | PERM_TRANSFER;
					perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE,PERM_NONE, next_owner_perm);
				S32 creation_date_now = static_cast<S32>(time_corrected());
				LLPointer<LLViewerInventoryItem> item
					= new LLViewerInventoryItem(item_id,
										folder_id,
										perm,
										mUUID,
										asset_type,
										inv_type,
										name,
										desc,
										LLSaleInfo::DEFAULT,
										LLInventoryItemFlags::II_FLAGS_NONE,
										creation_date_now);
				item->updateServer(TRUE);

				gInventory.updateItem(item);
				gInventory.notifyObservers();
		
				LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
				if (active_panel)
				{
					active_panel->openSelected();
					LLFocusableElement* focus = gFocusMgr.getKeyboardFocus();
					gFocusMgr.setKeyboardFocus(focus);
				}
			}
			else
			{
				LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
			}
		}
// </ShareStorm>

            const LLAvatarAppearanceDictionary::TextureEntry* tex_entry = LLAvatarAppearance::getDictionary()->getTexture((ETextureIndex)(i));
            if (!tex_entry)
                continue;

            if (LLVOAvatar::isIndexLocalTexture((ETextureIndex)i))
            {
                LLUUID id = IMG_DEFAULT_AVATAR;
                LLWearableType::EType wearable_type = LLAvatarAppearance::getDictionary()->getTEWearableType((ETextureIndex)i);
                if (avatarp->isSelf())
                {
                    LLViewerWearable *wearable = gAgentWearables.getViewerWearable(wearable_type, 0);
                    if (wearable)
                    {
                        LLLocalTextureObject *lto = wearable->getLocalTextureObject(i);
                        if (lto)
                        {
                            id = lto->getID();
                        }
                    }
                }
                if (id != IMG_DEFAULT_AVATAR)
                {
                    LL_INFOS() << "TE " << i << " name:" << tex_entry->mName << " id:" << id << LL_ENDL;
                }
                else
                {
                    LL_INFOS() << "TE " << i << " name:" << tex_entry->mName << " id:" << "<DEFAULT>" << LL_ENDL;
                }
            }
            else
            {
                LL_INFOS() << "TE " << i << " name:" << tex_entry->mName << " id:" << te->getID() << LL_ENDL;
            }
        }
    //}

// <ShareStorm>
	LLSD args;
	args["MESSAGE"] = msg;
	LLNotificationsUtil::add("SystemMessage", args);
// </ShareStorm>

}
