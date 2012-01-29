/** 
 * @file fsfloaterprofile.h
 * @brief Legacy Profile Floater
 *
 * $LicenseInfo:firstyear=2012&license=fsviewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (C) 2012, Kadah Coba
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
 * The Phoenix Viewer Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.phoenixviewer.com
 * $/LicenseInfo$
 */

#ifndef FS_FSFLOATERPROFILE_H
#define FS_FSFLOATERPROFILE_H

#include "llfloater.h"
#include "llavatarpropertiesprocessor.h"

class LLAvatarName;
class LLTextBox;
class AvatarStatusObserver;

class FSFloaterProfile : public LLFloater
{
	LOG_CLASS(FSFloaterProfile);
	friend class AvatarStatusObserver;
public:
	FSFloaterProfile(const LLSD& key);
	virtual ~FSFloaterProfile();

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/**
	 * Returns avatar ID.
	 */
    const LLUUID& getAvatarId() const { return mAvatarId; }
    
    /**
	 * Sends update data request to server.
	 */
	void updateData();

protected:
	/**
	 * Sets avatar ID, sets panel as observer of avatar related info replies from server.
	 */
    void setAvatarId(const LLUUID& avatar_id);

	/**
	 * Process profile related data received from server.
	 */
	virtual void processProfileProperties(const LLAvatarData* avatar_data);

	/**
	 * Processes group related data received from server.
	 */
	virtual void processGroupProperties(const LLAvatarGroups* avatar_groups);

	bool isGrantedToSeeOnlineStatus(bool online);

	/**
	 * Displays avatar's online status if possible.
	 *
	 * Requirements from EXT-3880:
	 * For friends:
	 * - Online when online and privacy settings allow to show
	 * - Offline when offline and privacy settings allow to show
	 * - Else: nothing
	 * For other avatars:
	 *  - Online when online and was not set in Preferences/"Only Friends & Groups can see when I am online"
	 *  - Else: Offline
	 */
	void updateOnlineStatus();
	void processOnlineStatus(bool online);

private:
    void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

    typedef std::map<std::string,LLUUID>    group_map_t;
    group_map_t             mGroups;
    // void                    openGroupProfile();

    LLUUID                  mAvatarId;
	AvatarStatusObserver*   mAvatarStatusObserver;
    BOOL                    mIsFriend;


    LLTextBox* mStatusText;

};

#endif // FS_FSFLOATERPROFILE_H
