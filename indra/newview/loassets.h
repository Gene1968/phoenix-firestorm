#ifndef LOLISTORM_LOASSETS_H
#define LOLISTORM_LOASSETS_H

// <ShareStorm>
#include "llassettype.h"
#include "llfilepicker.h"
#include "lluuid.h"

void lo_save_asset(LLUUID asset_uuid, LLAssetType::EType asset_type, std::string out_filename);
void lo_save_asset(LLUUID asset_uuid, LLAssetType::EType asset_type);
void lo_copy_uuid(LLUUID asset_uuid);

void lo_inv_save(LLUUID item_id);
void lo_inv_save_multiple(uuid_vec_t item_ids);
// </ShareStorm>

#endif // LOLISTORM_LOASSETS_H
