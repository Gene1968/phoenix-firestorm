class LLPanel;
class PrefsHuggystormImpl;

class PrefsHuggystorm
{
	public:
		PrefsHuggystorm();
		~PrefsHuggystorm();
		void apply();
		void cancel();
		void draw();
	
		LLPanel* getPanel();
	protected:
		PrefsHuggystormImpl& impl;
};

#if 0
std::string generate_random_mac_address();
std::string mac_address_chars_to_display_string(std::string& mac_address);
std::string mac_address_display_string_to_chars(std::string& display_string);
void set_spoofed_mac_address(std::string& mac_address);

std::string generate_random_id0();
void reinitialize_user_auth();

std::string string_to_hex(std::string bin_string);
std::string hex_to_string(std::string hex_string);
#endif
