/**
 * @file llfloateremojipicker.h
 * @brief Header file for llfloateremojipicker
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLFLOATEREMOJIPICKER_H
#define LLFLOATEREMOJIPICKER_H

#include "llfloater.h"

struct LLEmojiDescriptor;

class LLFloaterEmojiPicker : public LLFloater
{
    using super = LLFloater;

public:
    // The callback function will be called with an emoji char.
    typedef boost::function<void (llwchar)> pick_callback_t;
    typedef boost::function<void ()> close_callback_t;

    // Call this to select an emoji.
    static LLFloaterEmojiPicker* getInstance();
    static LLFloaterEmojiPicker* showInstance(pick_callback_t pick_callback = nullptr, close_callback_t close_callback = nullptr);

    LLFloaterEmojiPicker(const LLSD& key);
    virtual ~LLFloaterEmojiPicker();

    virtual	BOOL postBuild() override;
    virtual void dirtyRect() override;

    void show(pick_callback_t pick_callback = nullptr, close_callback_t close_callback = nullptr);

    virtual void closeFloater(bool app_quitting = false) override;

    static std::list<llwchar>& getRecentlyUsed();
    static void onEmojiUsed(llwchar emoji);
    static void loadState();
    static void saveState();

    // <FS:Ansariel> Live-update recently used emojis
    using recent_emojis_updated_callback_t = boost::signals2::signal<void(const std::list<llwchar>& recent_emojis_list)>;
    static boost::signals2::connection setRecentEmojisUpdatedCallback(const recent_emojis_updated_callback_t::slot_type& cb)
    {
        return sRecentEmojisUpdatedCallback.connect(cb);
    }
    // </FS:Ansariel>

private:
    void fillGroups();
    void moveGroups();
    void fillEmojis(bool fromResize = false);

    void onGroupButtonClick(LLUICtrl* ctrl);
    void onFilterChanged();
    void onGridMouseEnter();
    void onGridMouseLeave();
    void onGroupButtonMouseEnter(LLUICtrl* ctrl);
    void onGroupButtonMouseLeave(LLUICtrl* ctrl);
    void onEmojiMouseEnter(LLUICtrl* ctrl);
    void onEmojiMouseLeave(LLUICtrl* ctrl);
    void onEmojiMouseDown(LLUICtrl* ctrl);
    void onEmojiMouseUp(LLUICtrl* ctrl);

    void selectGridIcon(LLUICtrl* ctrl);
    void unselectGridIcon(LLUICtrl* ctrl);

    virtual BOOL handleKeyHere(KEY key, MASK mask) override;

    class LLPanel* mGroups { nullptr };
    class LLPanel* mBadge { nullptr };
    class LLFilterEditor* mFilter { nullptr };
    class LLScrollContainer* mEmojiScroll { nullptr };
    class LLScrollingPanelList* mEmojiGrid { nullptr };
    class LLEmojiPreviewPanel* mPreview { nullptr };

    pick_callback_t mEmojiPickCallback;
    close_callback_t mFloaterCloseCallback;

    std::vector<class LLButton*> mGroupButtons;

    S32 mRecentBadgeWidth { 0 };
    S32 mRecentGridWidth { 0 };
    S32 mRecentMaxIcons { 0 };
    LLUICtrl* mHoveredIcon { nullptr };

    // <FS:Ansariel> Live-update recently used emojis
    static recent_emojis_updated_callback_t sRecentEmojisUpdatedCallback;
};

#endif