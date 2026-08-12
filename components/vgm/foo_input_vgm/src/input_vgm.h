#pragma once
#include "input_base.h"
#include "../../enhancement/genesis_state.h"
#include "../../enhancement/psg_block_capture.h"
#include "../../enhancement/sn76489_enhanced.h"

class input_vgm : public input_base
{
private:
	VGMPlayer* m_vgm_player;
	bool	m_prefer_jpn_tag;
	constexpr static uint_fast32_t m_gd3_len_max = 1024*1024*100;
	bool	m_vgz;
	gameaudio::vgm::genesis_state m_genesis_state;
	gameaudio::vgm::psg_block_capture m_psg_capture;
	std::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
	std::array<bool, 2> m_psg_present{{false, false}};
	std::array<bool, 2> m_psg_shadow_valid{{false, false}};
	bool m_psg_capture_active = false;

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
	bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort);
	void decode_seek(double p_seconds, abort_callback &p_abort);

private:
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	static void command_observer_callback(void* user_param, const VGM_COMMAND_OBSERVER_EVENT* event);
#endif
	void configure_enhanced_psg();
	void apply_psg_event_outside_render(const gameaudio::vgm::command_event& event);
	void guess_track_number_tag_from_file_name(file_info& p_info);
};
