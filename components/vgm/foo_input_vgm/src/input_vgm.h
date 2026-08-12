#pragma once
#include "input_base.h"
#include "../../enhancement/genesis_state.h"

class input_vgm : public input_base
{
private:
	VGMPlayer* m_vgm_player;
	bool	m_prefer_jpn_tag;
	constexpr static uint_fast32_t m_gd3_len_max = 1024*1024*100;
	bool	m_vgz;
	gameaudio::vgm::genesis_state m_genesis_state;

protected:
	virtual void register_player();
	virtual void check_if_file_is_supported_format();
	virtual void modify_loop_count_appropriately();
	virtual void set_additional_options();
	virtual void set_additional_info(file_info& p_info, abort_callback& p_abort);
	virtual void retag_internal(const file_info& p_info, abort_callback& p_abort);
	virtual void remove_tags_internal(abort_callback& p_abort);

public:
	input_vgm();
	virtual ~input_vgm();
	static bool g_is_our_path(const char *p_path, const char *p_extension);

private:
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	static void command_observer_callback(void* user_param, const VGM_COMMAND_OBSERVER_EVENT* event);
#endif
	void guess_track_number_tag_from_file_name(file_info& p_info);
};
