#include "llviewerprecompiledheaders.h"

#include "llprefsnotifications.h"

#include "llbutton.h"
#include "llnotifications.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h"

#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "prefshuggystorm.h"
#include "lluserauth.h" // for reinitializing gUserAuth
#include "llbase64.h"
#include "llappviewer.h" // for MAC address stuff
#include "llrand.h"
#include <sstream>
#include "llmd5.h"
#include "llsys.h" // for LLOSInfo
#include "spoofing.h"
//#include "llagent.h"; // may use later for disabling mid-session hardware info changing if it causes any bugs


class PrefsHuggystormImpl final : public LLPanel
{
	public:
		PrefsHuggystormImpl();
		~PrefsHuggystormImpl() override
		{
		}
		
		void draw() override;
		bool postBuild() override;
		
		void apply();
		void cancel();

		void buildLists();
		void refreshValues();
		
		
	private:
		bool mEnablePermissionsBypass;
		bool mEnableHardwareSpoofing;
		std::string mSpoofedMACAddress;
		std::string mSpoofedID0;
		
		bool mEnableViewerVersionSpoofing;
		std::string mSpoofedVersionChannelName;
		S32 mSpoofedVersionMajor;
		S32 mSpoofedVersionMinor;
		S32 mSpoofedVersionBranch;
		S32 mSpoofedVersionRelease;
		
		bool mEnableMetaCache;
		
		
	// Callbacks:
		static void onCommitSpoofEnabled(LLUICtrl* ctrl, void* data);
		static void onClickRandomizeCallback(void* data);
		
		static void onCommitVersionEnabled(LLUICtrl* ctrl, void* data);
	
	// setter thingies:
		void setAddressEditorVisibility(bool enabled);
		void updateVersionEditor();

};

PrefsHuggystormImpl::PrefsHuggystormImpl()
: LLPanel("Huggystorm Preferences Panel")
{
	LLUICtrlFactory::getInstance()->buildPanel(this,
											   "panel_preferences_huggystorm.xml");
	
	
	refreshValues();
	buildLists();
	postBuild();
}

bool PrefsHuggystormImpl::postBuild()
{
	childSetCommitCallback("enable_hardware_spoofing", onCommitSpoofEnabled, this);
	childSetCommitCallback("enable_version_spoofing", onCommitVersionEnabled, this);
	childSetAction("randomize_hardware_info", onClickRandomizeCallback, this);
	return true;
}


void PrefsHuggystormImpl::refreshValues()
{
	
	mEnablePermissionsBypass = gSavedSettings.getBool("EnablePermissionsBypass");
	
	mEnableHardwareSpoofing = gSavedSettings.getBool("EnableHardwareSpoofing");
	mSpoofedMACAddress = LLBase64::decode( gSavedSettings.getString("SpoofedMACAddress") );
	childSetEnabled("randomize_hardware_info", mEnableHardwareSpoofing);
	childSetEnabled("mac_address_editor", false);
	
	// saved as base64-encoded string representing raw hash
	mSpoofedID0 = LLBase64::decode( gSavedSettings.getString("SpoofedID0") );
	childSetEnabled("id0_editor", false); // Should only be filled in via the "randomize" button.
	// May change later, but doing so adds unnecessary complexity
	
	setAddressEditorVisibility(mEnableHardwareSpoofing);
	
	// viewer version spoofing stuff
	mSpoofedVersionChannelName = gSavedSettings.getString("SpoofedVersionChannelName");
	mSpoofedVersionMajor = gSavedSettings.getS32("SpoofedVersionMajor");
	mSpoofedVersionMinor = gSavedSettings.getS32("SpoofedVersionMinor");
	mSpoofedVersionBranch = gSavedSettings.getS32("SpoofedVersionBranch");
	mSpoofedVersionRelease = gSavedSettings.getS32("SpoofedVersionRelease");
	mEnableViewerVersionSpoofing = gSavedSettings.getBool("EnableViewerVersionSpoofing");
	
	childSetValue("version_channel_name", mSpoofedVersionChannelName);
	childSetValue("version_major", mSpoofedVersionMajor);
	childSetValue("version_minor", mSpoofedVersionMinor);
	childSetValue("version_branch", mSpoofedVersionBranch);
	childSetValue("version_release", mSpoofedVersionRelease);
	
	updateVersionEditor();
	toggleSpoofedVersion();
	
	std::cout << "Version string: " << gCurrentVersion << std::endl;
	
	mEnableMetaCache = gSavedSettings.getBool("UseMetaCache");
	childSetValue("enable_meta_cache", mEnableMetaCache);
}

void PrefsHuggystormImpl::updateVersionEditor()
{
	bool enabled = gSavedSettings.getBool("EnableViewerVersionSpoofing");
	childSetValue("enable_version_spoofing", enabled);
	childSetEnabled("version_major", enabled);
	childSetEnabled("version_branch", enabled);
	childSetEnabled("version_release", enabled);
	childSetEnabled("version_minor", enabled);
	childSetEnabled("version_channel_name", enabled);
}

// not used for now
void PrefsHuggystormImpl::buildLists()
{
}

// draw the panel
void PrefsHuggystormImpl::draw()
{
	LLPanel::draw();
}

// apply settings
void PrefsHuggystormImpl::apply()
{
	
	refreshValues();
}

// revert settings back to previous values
void PrefsHuggystormImpl::cancel()
{
	gSavedSettings.setBool("EnableHardwareSpoofing", mEnableHardwareSpoofing);
	gSavedSettings.setBool("EnablePermissionsBypass", mEnablePermissionsBypass);
	gSavedSettings.setString("SpoofedMACAddress", LLBase64::encode(mSpoofedMACAddress) );
	gSavedSettings.setString("SpoofedID0", LLBase64::encode(mSpoofedID0) );
	
	gSavedSettings.setString("SpoofedVersionChannelName", mSpoofedVersionChannelName);
	gSavedSettings.setS32("SpoofedVersionMajor", mSpoofedVersionMajor);
	gSavedSettings.setS32("SpoofedVersionMinor", mSpoofedVersionMinor);
	gSavedSettings.setS32("SpoofedVersionBranch", mSpoofedVersionBranch);
	gSavedSettings.setS32("SpoofedVersionRelease", mSpoofedVersionRelease);
	
	// have to do this since any changes to the hardware info requires it
	reinitialize_user_auth();
	toggleSpoofedVersion();
	
	gSavedSettings.setBool("UseMetaCache", mEnableMetaCache);
}


// callbacks

void PrefsHuggystormImpl::onCommitVersionEnabled(LLUICtrl* ctrl, void* data)
{
	PrefsHuggystormImpl* self = (PrefsHuggystormImpl*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	
	// is this redundant? who knows
	std::cout << "enable version spoofing: " << check->get() << std::endl;
	gSavedSettings.setBool("EnableViewerVersionSpoofing", check->get());
	self->updateVersionEditor();
}

void PrefsHuggystormImpl::onCommitSpoofEnabled(LLUICtrl* ctrl, void* data)
{
	PrefsHuggystormImpl* self = (PrefsHuggystormImpl*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;
	
	// This will enable/disable the MAC address and ID0 input boxes
	bool enabled = check->get();
	self->childSetEnabled("mac_address_editor", false);
	self->childSetEnabled("id0_editor", false);
	self->childSetEnabled("randomize_hardware_info", enabled);
	self->setAddressEditorVisibility(enabled);
	
	
	if(enabled)
		reinitialize_user_auth();
	else
		reinitialize_user_auth();
}

void PrefsHuggystormImpl::onClickRandomizeCallback(void* data)
{
	PrefsHuggystormImpl* self = (PrefsHuggystormImpl*)data;
	
	// for MAC address
	std::string spoofded_mac = generate_random_mac_address();
	self->childSetText("mac_address_editor", mac_address_chars_to_display_string(spoofded_mac) );
	std::string display = self->childGetText("mac_address_editor");
	gSavedSettings.setString("SpoofedMACAddress", LLBase64::encode(spoofded_mac) );
	
	// 
	if(spoofded_mac.size() == MAC_ADDRESS_BYTES)
	{
		set_spoofed_mac_address(spoofded_mac);
	}
		
	// for ID0
	std::string spoofded_id0 = generate_random_id0();
	self->childSetText("id0_editor", string_to_hex(spoofded_id0) );
	gSavedSettings.setString("SpoofedID0", LLBase64::encode(spoofded_id0) );
	
	reinitialize_user_auth();
}


// This sets the fields as blank whenever spoofed hardware info
// is not enabled, and filled in when it is enabled.
void PrefsHuggystormImpl::setAddressEditorVisibility(bool enabled)
{
	if(enabled)
	{
		std::string mac_address_ = LLBase64::decode( 
					gSavedSettings.getString("SpoofedMACAddress") );
		childSetText("mac_address_editor",
			mac_address_chars_to_display_string(mac_address_) );
		
		std::string id0_ = LLBase64::decode(gSavedSettings.getString("SpoofedID0") );
		childSetText("id0_editor", string_to_hex(id0_) );
	}
	else
	{
		childSetText("mac_address_editor", "" );
		childSetText("id0_editor", "");
	}
}

////////

// class to be used

PrefsHuggystorm::PrefsHuggystorm():
	impl(*new PrefsHuggystormImpl)
{
	
}

PrefsHuggystorm::~PrefsHuggystorm()
{
	delete &impl;
}

void PrefsHuggystorm::draw()
{
	impl.draw();
}

void PrefsHuggystorm::apply()
{
	impl.apply();
}

void PrefsHuggystorm::cancel()
{
	impl.cancel();
}

LLPanel* PrefsHuggystorm::getPanel()
{
	return &impl;
}

#if 0
static const unsigned char byte_1_2_max = 51;


std::string generate_random_mac_address()
{
	std::string mac_address; // makes me feel better lmao
	for(size_t i = 0; i < MAC_ADDRESS_BYTES; i += 1)
	{
		if(i == 0 || i == 1)
			mac_address += (char)ll_rand((S32)byte_1_2_max);
		else
			mac_address += (char)ll_rand(256);
	}
	return mac_address;
}


std::string string_to_hex(std::string bin_string)
{
	std::string hex_string = "";
	int i = 0;
	for(unsigned char t: bin_string )
	{
		std::ostringstream ss;
		ss << std::hex << (unsigned int)t;
		std::string byte_display = ss.str();
		if(byte_display.size() == 1)
		{
			// put 0 on front
			byte_display = "0" + byte_display;
		}
		hex_string += byte_display;
		i += 1;
	}
	return hex_string;
}

std::string hex_to_string(std::string hex_string)
{
	std::ostringstream ss;
	std::string hex = "";
	int hexint;
	for(size_t hex_i = 0; hex_i < hex_string.size(); hex_i += 1)
	{
		if(hex.size() == 2)
		{
			std::istringstream(hex) >> std::hex >> hexint;
			ss << (unsigned char)hexint;
			hex = "";
		}
		hex += hex_string[hex_i];
	}
	std::istringstream(hex) >> std::hex >> hexint;
	ss << (unsigned char)hexint;
	return ss.str();
}


std::string mac_address_chars_to_display_string(std::string& mac_address)
{
	if(mac_address.size() == 6)
	{
		// changing this to my own implementation since LL's seems to put garbage on the end
		
		std::string mac_string = "";
		int i = 0;
		for(unsigned char t: mac_address )
		{
			std::ostringstream ss;
			ss << std::hex << (unsigned int)t;
			std::string byte_display = ss.str();
			if(byte_display.size() == 1)
			{
				// put 0 on front
				byte_display = "0" + byte_display;
			}
			mac_string += byte_display;
			if(i < 5)
			{
				mac_string += "-";
			}
			i += 1;
		}
		return mac_string;
	}
	else return "";
}


std::string mac_address_display_string_to_chars(std::string& display_string)
{
	std::string mac_string = "";
	std::ostringstream ss;
	std::string hex = "";
	int hexint;
	for(size_t hex_i = 0; hex_i < display_string.size(); hex_i += 1)
	{
		if(display_string[hex_i] != '-')
		{
			if(hex.size() == 2)
			{
				std::istringstream(hex) >> std::hex >> hexint;
				ss << (unsigned char)hexint;
				hex = "";
			}
			hex += display_string[hex_i];
		}
	}
	std::istringstream(hex) >> std::hex >> hexint;
	ss << (unsigned char)hexint;
	mac_string = ss.str();
	return mac_string;
}


void set_spoofed_mac_address(std::string& mac_address)
{
	for(size_t i = 0; i < mac_address.size(); i += 1)
	{
		gSpoofedMACAddress[i] = mac_address.c_str()[i];
	}
}

// Generates a fake MD5 hash of id0
// Ok, so it actually has to be a valid MD5 hash.
// Just going to hash whatever random thing this generates then.
std::string generate_random_id0()
{
	std::string to_hash = "";
	std::ostringstream ss;

	size_t i = 0;
	while( i < 32)
	{
		char byte = (char)ll_rand(256);
		if( byte != '\0')
		{
			ss << byte;
			i += 1;
		} 
	}
	to_hash = ss.str();
	LLMD5 hash;
	char hashed_id0_string[MD5HEX_STR_SIZE];
	hash.update( (unsigned char*)to_hash.c_str(), MD5HEX_STR_SIZE );
	hash.finalize();
	hash.hex_digest(hashed_id0_string);
	std::string output(hashed_id0_string);
	std::cout << "id0 length: " << output.size() << std::endl;
	return output;
}

void reinitialize_user_auth()
{
	// just copy all the crap from llappviewer.cpp that initializes gUserAuth
	char hashed_mac_string[MD5HEX_STR_SIZE];
	LLMD5 hashed_mac;
	
	bool hardware_spoofing = gSavedSettings.getBool("EnableHardwareSpoofing");
	std::string spoofed_mac_str = LLBase64::decode(gSavedSettings.getString("SpoofedMACAddress") );
	if(hardware_spoofing && spoofed_mac_str.size() == MAC_ADDRESS_BYTES)
	{
		std::cout << "USING SPOOFED MAC ADDRESS" << std::endl;
		strcpy( (char*)gSpoofedMACAddress, spoofed_mac_str.c_str() );
		hashed_mac.update(gSpoofedMACAddress, MAC_ADDRESS_BYTES);
	}
	else
	{
		hashed_mac.update(gMACAddress, MAC_ADDRESS_BYTES);
	}
	
	std::string serial_to_use;
	if(hardware_spoofing)
	{
		std::cout << "USING SPOOFED ID0" << std::endl;
		serial_to_use = LLBase64::decode(gSavedSettings.getString("SpoofedID0") );
	}
	else
	{
		serial_to_use = gAppViewerp->getSerialNumber();
	}
	std::cout << "serial length: " << serial_to_use.size() << std::endl;
	hashed_mac.finalize();
	hashed_mac.hex_digest(hashed_mac_string);
	const LLOSInfo* osinfo = LLOSInfo::getInstance();
	gUserAuth.init(osinfo->getOSVersionString(), osinfo->getOSStringSimple(),
				   gCurrentVersion,
				   gSavedSettings.getString("VersionChannelName"),
					serial_to_use, hashed_mac_string);
}
#endif
