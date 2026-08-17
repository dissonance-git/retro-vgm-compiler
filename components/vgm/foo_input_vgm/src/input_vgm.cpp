#include "input_vgm.h"
#include "my_common_vars.h"
#include "my_cfg_external.h"
#ifdef LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI
#include "source_aware_vgm_player.h"
#endif

static inline uint32_t read_le32(const uint8_t* data)
{
	return ((uint32_t)data[3] << 24) | ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[0];
}

static inline void write_le32(uint8_t* data, uint32_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
	data[2] = (uint8_t)(value >> 16);
	data[3] = (uint8_t)(value >> 24);
}

static inline void write_le16(uint8_t* data, uint16_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
}

input_vgm::input_vgm() :
	m_vgm_player(nullptr), 
	m_vgz(false)
{
}

input_vgm::~input_vgm()
{
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	if (m_vgm_player != nullptr)
		m_vgm_player->SetCommandObserver(nullptr, nullptr);
#endif
}

static inline void meta_add_repl_value(file_info& p_info, const char* p_name, const char* p_name_repl)
{
	if (!p_name || !p_name_repl)
		return;
	const char* p_value = p_info.meta_get(p_name, 0);
	if (p_value)
		return;

	t_size count = p_info.meta_get_count_by_name(p_name_repl);
	for (t_size i = 0; i < count; i++)
	{
		p_value = p_info.meta_get(p_name_repl, i);
		//if (p_value)
		p_info.meta_add(p_name, p_value);
	}
}

static inline void meta_remove_repl_value(const char * & p_out, const char* p_value, const char* p_value_repl)
{
	if (!p_value || !p_value_repl)
		return;
	
	if (!stricmp_utf8(p_value, p_value_repl))
		p_out = nullptr;
}

#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
void input_vgm::command_observer_callback(void* user_param, const VGM_COMMAND_OBSERVER_EVENT* event)
{
	if (user_param == nullptr || event == nullptr)
		return;

	input_vgm* self = static_cast<input_vgm*>(user_param);
	gameaudio::vgm::command_event mapped;
	mapped.tick = event->tick;
	mapped.file_offset = event->filePos;
	mapped.command = event->command;
	mapped.payload = event->payload;
	mapped.payload_size = event->payloadLen;

	switch (event->type)
	{
	case VGMCOE_RESET:
		mapped.kind = gameaudio::vgm::command_event_kind::reset;
		break;
	case VGMCOE_YM2612_DAC:
		mapped.kind = gameaudio::vgm::command_event_kind::ym2612_dac;
		break;
	case VGMCOE_COMMAND:
	default:
		mapped.kind = gameaudio::vgm::command_event_kind::command;
		break;
	}

	self->m_genesis_state.observe(mapped);
}
#endif

void input_vgm::register_player()
{
#ifdef LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI
	// The subclass leaves the protected VGMPlayer render unchanged while
	// collecting exactly aligned source contributions through the guarded
	// libvgm hooks. Unpatched libvgm builds retain the historical player below.
	m_vgm_player = new SourceAwareVGMPlayer;
#else
	m_vgm_player = new VGMPlayer;
#endif
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	m_vgm_player->SetCommandObserver(&input_vgm::command_observer_callback, this);
#endif
	m_main_player.RegisterPlayerEngine(m_vgm_player);
}

void input_vgm::check_if_file_is_supported_format()
{
	PlayerBase* p = m_main_player.GetPlayer();
	if (!p || p->GetPlayerType() != FCC_VGM)
	{
		throw exception_io_unsupported_format();
	}
	if (m_file_buf[0] == 0x1F && m_file_buf[1] == 0x8B)
		m_vgz = true;
}

void input_vgm::modify_loop_count_appropriately()
{
	m_loop_count = m_vgm_player->GetModifiedLoopCount(m_loop_count);
	m_loop_count_for_ifnl = m_vgm_player->GetModifiedLoopCount(m_loop_count_for_ifnl);
}

void input_vgm::set_additional_options()
{
	VGM_PLAY_OPTIONS vpo;
	UINT8 ret = m_vgm_player->GetPlayerOptions(vpo);
	if (!ret)
	{
		vpo.playbackHz = cfg_playback_rate;
		vpo.hardStopOld = cfg_hard_stop_old_vgms;
		m_vgm_player->SetPlayerOptions(vpo);
	}
	m_prefer_jpn_tag = cfg_prefer_jpn_tag > 0 ? true : false;
}

void input_vgm::set_additional_info(file_info& p_info, abort_callback& p_abort)
{
	p_info.info_set("codec", "VGM");

	//Header
	const VGM_HEADER* vgm_header = m_vgm_player->GetFileHeader();
	UINT32 vgm_version = vgm_header->fileVer;

	//In SDK-2022-08-10, operator "<<" appears to be removed.
	pfc::string_formatter temp_string = pfc::format_uint(vgm_version >> 8, 0, 16).get_ptr();
	temp_string += ".";
	temp_string += pfc::format_uint(vgm_version & 0xFF, 2, 16);
	p_info.info_set("VGM_VERSION", temp_string.get_ptr());

	p_info.info_set_int("VGM_SONG_SAMPLES", vgm_header->numTicks);

	if (vgm_header->loopTicks > 0)
	p_info.info_set_int("VGM_LOOP_SAMPLES", vgm_header->loopTicks);
	
	if (vgm_header->recordHz > 0)
		p_info.info_set_int("VGM_RECORDED_RATE", vgm_header->recordHz);

	//GD3 tag
	const char* const* tag_list = m_vgm_player->GetTags();
	const char* p_name_temp = nullptr;
	for (const char* const* p = tag_list; *p; p += 2)
	{
		if (p[1][0] == '\0')
			continue;

		if (0 == strcmp("TITLE", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "TITLE_E" : "title";
			p_info.meta_add(p_name_temp, p[1]);
		}
		else if (0 == strcmp("TITLE-JPN", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "title" : "TITLE_J";
			p_info.meta_add(p_name_temp, p[1]);
		}
		else if (0 == strcmp("ARTIST", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "ARTIST_E" : "artist";
			meta_add_for_multi_value_field(p_info, p_name_temp, p[1]);
		}
		else if (0 == strcmp("ARTIST-JPN", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "artist" : "ARTIST_J";
			meta_add_for_multi_value_field(p_info, p_name_temp, p[1]);
		}
		else if (0 == strcmp("GAME", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "ALBUM_E" : "album";
			p_info.meta_add(p_name_temp, p[1]);
		}
		else if (0 == strcmp("GAME-JPN", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "album" : "ALBUM_J";
			p_info.meta_add(p_name_temp, p[1]);
		}
		else if (0 == strcmp("SYSTEM", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "SYSTEM_E" : "system";
			p_info.meta_add(p_name_temp, p[1]);
		}
		else if (0 == strcmp("SYSTEM-JPN", p[0]))
		{
			p_name_temp = m_prefer_jpn_tag ? "system" : "SYSTEM_J";
			p_info.meta_add(p_name_temp, p[1]);
		}
		else if (0 == strcmp("DATE", p[0]))
		{
			p_info.meta_add(p[0], p[1]);
		}
		else if (0 == strcmp("ENCODED_BY", p[0]))
		{
			p_info.meta_add("ripper", p[1]);
		}
		else if (0 == strcmp("COMMENT", p[0]))
		{
			meta_add_for_multi_line_field(p_info, p[0], p[1]);
		}
	}
	if (m_prefer_jpn_tag)
	{
		meta_add_repl_value(p_info, "title", "TITLE_E");
		meta_add_repl_value(p_info, "artist", "ARTIST_E");
		meta_add_repl_value(p_info, "album", "ALBUM_E");
		meta_add_repl_value(p_info, "system", "SYSTEM_E");
	}
	else
	{
		meta_add_repl_value(p_info, "title", "TITLE_J");
		meta_add_repl_value(p_info, "artist", "ARTIST_J");
		meta_add_repl_value(p_info, "album", "ALBUM_J");
		meta_add_repl_value(p_info, "system", "SYSTEM_J");
	}

	// Some players incorrectly put the actual artist information in the dumper field.
	// If the artist is empty, copy the dumper field to the artist field.
	if (p_info.meta_get_count_by_name("artist") == 0)
	{
		const char* p_value = p_info.meta_get("dumper", 0);
		if (p_value)
			meta_add_for_multi_value_field(p_info, "artist", p_value);
	}

	if (p_info.meta_get_count_by_name("comment") == 0)
	{
		const char* p_value = p_info.meta_get("notes", 0);
		if (p_value)
			meta_add_for_multi_line_field(p_info, "comment", p_value);
	}

	// Markers
	if (m_vgm_player->GetCurLoop() > 0)
		p_info.info_set_int("VGM_CURRENT_LOOP", m_vgm_player->GetCurLoop());
}

void input_vgm::get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort)
{
	input_base::get_info(p_subsong, p_info, p_abort);
}

void input_vgm::retag_set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort)
{
	input_base::retag_set_info(p_subsong, p_info, p_abort);
}

void input_vgm::retag_commit(abort_callback& p_abort)
{
	input_base::retag_commit(p_abort);
}

bool input_vgm::is_our_content_type(const char* p_content_type)
{
	return false;
}

bool input_vgm::is_our_path(const char* p_path, const char* p_extension)
{
	return !_stricmp(p_extension, "vgm") || !_stricmp(p_extension, "vgz");
}

void input_vgm::decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback& p_abort)
{
	input_base::decode_initialize(p_subsong, p_flags, p_abort);
}

void input_vgm::set_cfg_from_vgm_player()
{
	// no-op; kept for compatibility with historical call sites
}

void input_vgm::write_vgm_player_config()
{
	// no-op; kept for compatibility with historical call sites
}
