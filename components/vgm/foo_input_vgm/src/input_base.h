#pragma once
#include "my_common_vars.h"
#include "my_cfg_var.h"

class input_base : public input_stubs
{
protected:
	//Main player
	PlayerA							m_main_player;
	DATA_LOADER*					m_data_loader;
	
	service_ptr_t<file>				m_file;			//handle of input file
	pfc::array_t<uint8_t>			m_file_buf;
	pfc::array_t<uint8_t>			m_decode_buf;	//decode buffer
	//t_size						m_decode_buf_len;
	pfc::string8					m_file_path;		//input file path
	//pfc::string8					m_file_ext;		//input file extension
	t_filesize						m_file_len;		//input file size
	constexpr static t_filesize		m_file_len_max = 524288000;

	//Song time
	double							m_song_len;	//song length [s]
	uint_fast32_t					m_fade_len;	//configure fade length [ms]
	uint_fast16_t					m_loop_count;
	uint_fast16_t					m_loop_count_for_ifnl;	//for input_flag_no_looping
	bool							m_loop_flag;
	uint_fast64_t					m_song_sample;
	uint_fast64_t					m_fade_sample;
	//uint_fast64_t					m_pause_sample;
	bool							m_play_infinitely;
	volatile bool					m_calculated_song_len;

	//Playback
	uint_fast32_t					m_sample_rate;
	uint_fast32_t					m_bps;
	constexpr static uint_fast32_t	m_channels = 2;
	constexpr static uint_fast32_t	m_render_ms = 10;	//buffer duration [ms]
	uint_fast32_t					m_render_sample;	// buffer duration [sample]
	uint_fast32_t					m_render_byte;
	uint_fast32_t					m_render_done;
	uint_fast64_t					m_played_sample;		//played time [sample]
	bool							m_dynamic_info;			//reporting configuration dynamically
	volatile bool					m_play_end;

	//etc.
	t_input_open_reason				m_open_reason;
	unsigned int					m_decode_flags;
	static const GUID				m_decoder_priority_table_guid;
	std::vector<PLR_DEV_INFO>		m_pdi_list;
	pfc::string8_fast				m_chip_name;
	pfc::string8_fast				m_accurate_chip_name;

public:
	input_base();
	virtual ~input_base();

	static bool g_is_our_content_type(const char *p_content_type) { return false; }
//	static bool g_is_our_path(const char* p_path, const char* p_extension);
	void open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback &p_abort);
	t_filestats get_file_stats(abort_callback & p_abort) { return m_file->get_stats(p_abort); }
#if FOOBAR2000_SDK_VERSION >= 20220810
	t_filestats2 get_stats2(uint32_t f, abort_callback& p_abort) { return m_file->get_stats2_(f, p_abort); }
#endif
	void get_info(file_info &p_info, abort_callback &p_abort);
	bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta);
	bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta) { return false; }

	void decode_initialize(unsigned int p_flags, abort_callback &p_abort);
	bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort);
	bool decode_can_seek();
	void decode_seek(double p_seconds, abort_callback &p_abort);
	void decode_on_idle(abort_callback & p_abort);

	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) { return false; }
	void set_logger(event_logger::ptr ptr) {  };

	void set_pause(bool paused);
	bool flush_on_pause() { return false; }

	void retag(const file_info &p_info, abort_callback &p_abort);
	void remove_tags(abort_callback& p_abort);
	static GUID g_get_guid()
	{
		return m_decoder_priority_table_guid;
	}

	static const char * g_get_name()
	{
		return g_my_component_name;
	}

	static GUID g_get_preferences_guid()
	{
		return g_guid_my_preferences_page;
	}
	static UINT8 event_callback(PlayerBase* player, void* user_param, UINT8 evt_type, void* evt_param);
	static DATA_LOADER* file_req_callback(void* user_param, PlayerBase* player, const char* file_name);
	static void update_muting(const PLR_DEV_OPTS_EX & p_opts);
	static void update_muting_of_all_current_chips();
	static void update_panning(const PLR_DEV_OPTS_EX& p_opts);
	static void update_panning_of_all_current_chips();
	static bool get_current_playback_chip_status(uint8_t & p_first_chip_type, pfc::string_base & p_chip_name);

protected:
	void read_file(abort_callback & p_abort);
	void load_file_to_player();
	void calc_song_len();
	void meta_add_for_multi_value_field(file_info& p_info, const char* name, const char* value);
	void meta_add_for_multi_line_field(file_info& p_info, const char* name, const char* value);

private:
	static critical_section g_lock;
	static pfc::list_t<input_base*> g_my_instance_list;

protected:
	virtual void register_player() = 0;
	virtual void check_if_file_is_supported_format() = 0;
	virtual void modify_loop_count_appropriately() = 0;
	virtual void set_additional_options() = 0;
	virtual void set_additional_info(file_info& p_info, abort_callback& p_abort) = 0;
	virtual void retag_internal(const file_info& p_info, abort_callback& p_abort) = 0;
	virtual void remove_tags_internal(abort_callback& p_abort) = 0;

};