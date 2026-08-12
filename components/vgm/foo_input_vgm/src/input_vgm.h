#pragma once
#include "input_base.h"
#include "../../enhancement/genesis_state.h"
#include "../../enhancement/psg_block_capture.h"
#include "../../enhancement/sn76489_enhanced.h"
#include "../../enhancement/ym2612_dac_block_capture.h"
#include "../../enhancement/ym2612_dac_enhanced.h"

class input_vgm : public input_base
{
private:
	VGMPlayer* m_vgm_player;
	bool	m_prefer_jpn_tag;
	constexpr static uint_fast32_t m_gd3_len_max = 1024*1024*100;
	bool	m_vgz;

	gameaudio::vgm::genesis_state m_genesis_state;
	gameaudio::vgm::psg_block_capture m_psg_capture;
	gameaudio::vgm::ym2612_dac_block_capture m_dac_capture;
	std::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
	std::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;

	std::array<bool, 2> m_psg_present{{false, false}};
	std::array<bool, 2> m_psg_config_supported{{false, false}};
	std::array<bool, 2> m_psg_shadow_valid{{false, false}};
	std::array<bool, 2> m_dac_present{{false, false}};
	std::array<bool, 2> m_dac_shadow_valid{{false, false}};

	bool m_source_capture_active = false;
	bool m_shadow_configured = false;
	uint_fast64_t m_shadow_replay_sample = 0;

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
	bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort) override;
	void decode_seek(double p_seconds, abort_callback &p_abort) override;

private:
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	static void command_observer_callback(void* user_param, const VGM_COMMAND_OBSERVER_EVENT* event);
#endif
	static void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;
	void configure_enhancement_shadow();
	void apply_source_event_outside_render(const gameaudio::vgm::command_event& event) noexcept;
	void advance_shadow_to(uint_fast64_t absolute_sample) noexcept;
	void replay_captured_sources(uint_fast32_t rendered_samples) noexcept;
	void invalidate_unobserved_dac_stream(const gameaudio::vgm::command_event& event) noexcept;
	void guess_track_number_tag_from_file_name(file_info& p_info);
};
