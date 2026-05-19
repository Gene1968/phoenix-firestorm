// LOstorm: Shoutout to whoever originally wrote this some time last century
#ifndef LOLISTORM_LOFLOATEREXPORT_H
#define LOLISTORM_LOFLOATEREXPORT_H

#include "llfloater.h"
#include "llselectmgr.h"
#include "llvoavatar.h"

class LOFloaterExport : public LLFloater
{
public:
    LOFloaterExport(const LLSD& key);
    virtual bool postBuild();

    void repopulate();
    LLSD getLLSD();

    // Called by llviewermessage.cpp
    void processObjectProperties(LLMessageSystem* msg);

    static void onClickSelectAll(void* user_data);
    static void onClickSelectObjects(void* user_data);
    static void onClickSelectMeshes(void* user_data);
    static void onClickSelectWearables(void* user_data);
    static void onClickSaveAs(void* user_data);

private:
    virtual ~LOFloaterExport();
    void addAvatarStuff(LLVOAvatar* avatarp);
    void addToPrimList(LLViewerObject* object);
    void receivePrimName(LLViewerObject* object, std::string name, std::string desc);
    void requestObjectProperties(const std::vector<U32>& request_list, bool select, LLViewerRegion* regionp);
    void selectAllGeneric(bool(*pred)(const std::string&));
    void updateNamesProgress();

    enum LIST_COLUMN_ORDER
    {
        LIST_CHECKED,
        LIST_TYPE,
        LIST_NAME,
        LIST_AVATARID
    };

    std::vector<U32> mPrimList;
    std::map<U32, std::pair<std::string, std::string>> mPrimNameMap;
    std::map<LLUUID, LLSD> mExportables;
};

#endif // LOLISTORM_LOFLOATEREXPORT_H
