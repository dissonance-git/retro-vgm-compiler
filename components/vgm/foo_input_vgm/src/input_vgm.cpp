#include "input_vgm.h"
#include "my_common_vars.h"
#include "my_cfg_external.h"

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
	m_vgm_player = new VGMPlayer;
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
	if (cfg_guess_track_number)
		guess_track_number_tag_from_file_name(p_info);
}

void input_vgm::retag_internal(const file_info& p_info, abort_callback& p_abort)
{
	const uint32_t fcc_gd3 = 0x20336447;
	const uint32_t supported_gd3_version = 0x100;

	//const VGM_HEADER *vgm_header = m_vgm_player->GetFileHeader();
	uint8_t *vgm_data = DataLoader_GetData(m_data_loader);
	uint32_t vgm_size = DataLoader_GetSize(m_data_loader);

	if (vgm_size > m_file_len_max)
		throw exception_io_data("VGM size is too large to edit tag");

	uint32_t eof_offset = read_le32(vgm_data + 0x04);
	uint32_t gd3_offset = read_le32(vgm_data + 0x14);

	if (eof_offset > m_file_len_max)
		throw exception_io_data("EOF offset is too large to edit tag");
	if (gd3_offset > m_file_len_max)
		throw exception_io_data("GD3 offset is too large to edit tag");

	//relative -> absolute offset
	if (eof_offset)
		eof_offset += 0x04;
	if (gd3_offset)
		gd3_offset += 0x14;

	if (!eof_offset || eof_offset != vgm_size)
		throw exception_io_data("Invalid EOF offset");

	if (gd3_offset)
	{
		uint32_t gd3_signature = read_le32(vgm_data + gd3_offset);
		uint32_t gd3_version = read_le32(vgm_data + gd3_offset + 4);
		if (gd3_signature != fcc_gd3)
			throw exception_io_data("Invalid GD3 tag");
		if (gd3_version != supported_gd3_version)
			throw exception_io_data("Unsupported GD3 version");
	}

	size_t gd3_elements = 11;
	enum
	{
		track_name_E = 0, 
		track_name_J = 1,
		game_name_E = 2,
		game_name_J = 3,
		system_name_E = 4,
		system_name_J = 5,
		author_name_E = 6,
		author_name_J = 7,
		release_date = 8,
		creator = 9,
		notes = 10
	};
	pfc::array_t<const char *> gd3_tags;
	gd3_tags.set_size(gd3_elements);
	for (size_t i = 0; i < gd3_tags.size(); i++)
		gd3_tags[i] = nullptr;

	//const char* value = nullptr;
	pfc::string8_fast formatted_author_name_1;
	pfc::string8_fast formatted_author_name_2;
	pfc::string8_fast_aggressive formatted_notes;

	if (!m_prefer_jpn_tag)
	{
		//English
		gd3_tags[track_name_E] = p_info.meta_get("title", 0);
		gd3_tags[track_name_J] = p_info.meta_get("TITLE_J", 0);
		gd3_tags[game_name_E] = p_info.meta_get("album", 0);
		gd3_tags[game_name_J] = p_info.meta_get("ALBUM_J", 0);
		gd3_tags[system_name_E] = p_info.meta_get("system", 0);
		gd3_tags[system_name_J] = p_info.meta_get("SYSTEM_J", 0);

		p_info.meta_format("artist", formatted_author_name_1);
		if (!formatted_author_name_1.is_empty())
			gd3_tags[author_name_E] = formatted_author_name_1.get_ptr();
		p_info.meta_format("ARTIST_J", formatted_author_name_2);
		if (!formatted_author_name_2.is_empty())
			gd3_tags[author_name_J] = formatted_author_name_2.get_ptr();
	}
	else
	{
		//Japanese
		gd3_tags[track_name_J] = p_info.meta_get("title", 0);
		gd3_tags[track_name_E] = p_info.meta_get("TITLE_E", 0);
		gd3_tags[game_name_J] = p_info.meta_get("album", 0);
		gd3_tags[game_name_E] = p_info.meta_get("ALBUM_E", 0);
		gd3_tags[system_name_J] = p_info.meta_get("system", 0);
		gd3_tags[system_name_E] = p_info.meta_get("SYSTEM_E", 0);

		p_info.meta_format("artist", formatted_author_name_1);
		if (!formatted_author_name_1.is_empty())
			gd3_tags[author_name_J] = formatted_author_name_1.get_ptr();
		p_info.meta_format("ARTIST_E", formatted_author_name_2);
		if (!formatted_author_name_2.is_empty())
			gd3_tags[author_name_E] = formatted_author_name_2.get_ptr();

		meta_remove_repl_value(gd3_tags[track_name_J], gd3_tags[track_name_J], gd3_tags[track_name_E]);
		meta_remove_repl_value(gd3_tags[game_name_J], gd3_tags[game_name_J], gd3_tags[game_name_E]);
		meta_remove_repl_value(gd3_tags[system_name_J], gd3_tags[system_name_J], gd3_tags[system_name_E]);
		meta_remove_repl_value(gd3_tags[author_name_J], gd3_tags[author_name_J], gd3_tags[author_name_E]);
	}

	gd3_tags[release_date] = p_info.meta_get("date", 0);
	gd3_tags[creator] = p_info.meta_get("ripper", 0);
	gd3_tags[notes] = p_info.meta_get("comment", 0);
	if (gd3_tags[notes])
	{
		formatted_notes = gd3_tags[notes];
		formatted_notes.replace_string("\r\n", "\n");
		gd3_tags[notes] = formatted_notes.get_ptr();
	}

	pfc::array_t<wchar_t> new_gd3_data;
	pfc::stringcvt::string_wide_from_utf8 converter;

	for (size_t i = 0; i < gd3_tags.size(); i++)
	{
		if (gd3_tags[i])
		{
			converter.convert(gd3_tags[i]);
			new_gd3_data.append_fromptr(converter.get_ptr(), converter.length());
		}
		new_gd3_data.append_fromptr(L"\0", 1);
	}

	if (0 == gd3_offset && new_gd3_data.size() == gd3_elements)
		return;

	size_t new_gd3_size = 0;
	if (new_gd3_data.size() != gd3_elements)
		new_gd3_size = new_gd3_data.size() * sizeof(wchar_t);
	if (new_gd3_size > m_gd3_len_max)
		throw exception_io_data("Size of GD3 tag is too large to edit tag");

	pfc::array_t<uint8_t> new_vgm_data;
	
	uint32_t gd3_size = 0;
	if (gd3_offset)
	{
		gd3_size = read_le32(vgm_data + gd3_offset + 8);
	}

	uint32_t gd3_removed_vgm_size = 0;
	uint32_t new_gd3_offset = 0;
	size_t new_vgm_size = 0;
	bool update_gd3_only = false;

	if (!gd3_offset || gd3_offset + 0x0C + gd3_size == eof_offset)
	{
		if (!m_vgz)
			update_gd3_only = true;

		gd3_removed_vgm_size = gd3_offset ? (vgm_size - gd3_size - 0x0C) : vgm_size;
		if (new_gd3_size)
		{
			new_vgm_size = gd3_removed_vgm_size + 0x0C + new_gd3_size;
			new_gd3_offset = gd3_removed_vgm_size - 0x14;
		}
		else
		{
			new_vgm_size = gd3_removed_vgm_size;
			new_gd3_offset = 0;
		}

		if (!update_gd3_only)
		{
			new_vgm_data.set_size(new_vgm_size);
			memcpy(new_vgm_data.get_ptr(), vgm_data, gd3_removed_vgm_size);
			if (new_gd3_size)
			{
				write_le32(new_vgm_data.get_ptr() + gd3_removed_vgm_size, fcc_gd3);
				write_le32(new_vgm_data.get_ptr() + gd3_removed_vgm_size + 4, supported_gd3_version);
				write_le32(new_vgm_data.get_ptr() + gd3_removed_vgm_size + 8, new_gd3_size);
				memcpy(new_vgm_data.get_ptr() + gd3_removed_vgm_size + 12, new_gd3_data.get_ptr(), new_gd3_size);
			}
		}
	}
	else
	{
		throw pfc::exception_not_implemented();
	}

	if (gd3_removed_vgm_size + new_gd3_size > m_file_len_max)
		throw exception_io_data("VGM size is too large to edit tag");

	if (update_gd3_only)
	{
		m_file->seek(0x04, p_abort);
		m_file->write_lendian_t<uint32_t>((uint32_t)(new_vgm_size - 4), p_abort);
		m_file->seek(0x14, p_abort);
		m_file->write_lendian_t<uint32_t>(new_gd3_offset, p_abort);
		m_file->seek(gd3_removed_vgm_size, p_abort);
		if (new_gd3_size)
		{
			m_file->write_lendian_t<uint32_t>(fcc_gd3, p_abort);
			m_file->write_lendian_t<uint32_t>(supported_gd3_version, p_abort);
			m_file->write_lendian_t<uint32_t>((uint32_t)new_gd3_size, p_abort);
			m_file->write(new_gd3_data.get_ptr(), new_gd3_size, p_abort);
		}
		m_file->set_eof(p_abort);
	}
	else if (m_vgz)
	{
		write_le32(new_vgm_data.get_ptr() + 4, new_vgm_size - 4);
		write_le32(new_vgm_data.get_ptr() + 0x14, new_gd3_offset);
		
		z_stream stream;
		int zlib_result = 0;

		stream.next_in = (Bytef*)new_vgm_data.get_ptr();
		stream.avail_in = new_vgm_size;
		stream.next_out = nullptr;
		stream.avail_out = 0;
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;

		zlib_result = deflateInit2(&stream,
						Z_BEST_COMPRESSION,
						Z_DEFLATED,
						(MAX_WBITS | 16),
						8,
						Z_DEFAULT_STRATEGY);

		if (zlib_result != Z_OK)
			throw exception_io_data("Failed to recompress VGZ");

		uLong estimated_size = deflateBound(&stream, new_vgm_size);

		pfc::array_t<uint8_t> new_vgz_data;
		new_vgz_data.set_size(estimated_size);
		stream.next_out = (Bytef*)new_vgz_data.get_ptr();
		stream.avail_out = estimated_size;

		zlib_result = deflate(&stream, Z_FINISH);

		if (zlib_result == Z_OK)
		{
			deflateEnd(&stream);
		}
		if (zlib_result != Z_STREAM_END)
		{
			//Error or output buffer is short 
			throw exception_io_data("Failed to recompress VGZ");
		}
		size_t new_vgz_size = stream.total_out;
		deflateEnd(&stream);

		m_file->seek(0x00, p_abort);
		m_file->write(new_vgz_data.get_ptr(), new_vgz_size, p_abort);
		m_file->set_eof(p_abort);
	}

	new_vgm_data.force_reset();
	new_gd3_data.force_reset();

	//Reload info
	m_main_player.UnloadFile();
	DataLoader_Deinit(m_data_loader);
	m_data_loader = nullptr;

	m_file->seek(0x00, p_abort);
	read_file(p_abort);
	load_file_to_player();
	m_file_buf.force_reset();
}

void input_vgm::remove_tags_internal(abort_callback& p_abort)
{
	file_info_impl info;
	info.meta_remove_all();
	retag_internal(info, p_abort);
}

//TODO:
void input_vgm::guess_track_number_tag_from_file_name(file_info & p_info)
{
	const char* p_name = m_prefer_jpn_tag ? "ALBUM_E" : "album";
	const char* game_name = p_info.meta_get(p_name, 0);
#if FOOBAR2000_SDK_VERSION >= 20220104
	pfc::string8 file_name = pfc::string_filename(m_file_path.get_ptr());
#else
	pfc::string_filename file_name(m_file_path);
#endif
	t_size file_name_len = file_name.get_length();
	t_size n = 0;
	for (; (n < file_name_len && '0' <= file_name[n] && file_name[n] <= '9'); n++) {}

	//track number is usually between 1 - 999
	if (n > 0 && n < 4 && (file_name[n] >= 'a' && file_name[n] <= 'z' || file_name[n] >= 'A' && file_name[n] <= 'Z'))
		n++;
	
	if (n > 0 && n < 5 && (file_name[n] == ' ' || file_name[n] == '_' || file_name[n] == '.' && file_name[n + 1] == ' '))
		p_info.meta_add_ex("tracknumber", ~0, file_name, n);
#if 0
	//Should use folder name
	else if (game_name)
	{		
		n = 0;

		while ( n < file_name_len )
		{
			if (file_name[n] == '\0')
				break;
			if (file_name[n] == ' ' && file_name[n + 1] == '-' && file_name[n + 2] == ' ')
			{		
				if (stricmp_utf8_max(file_name, game_name, n))
					break;
				n += 3;
				t_size begin = n;
				t_size len = 0;
				for (; (n < file_name_len && '0' <= file_name[n] && file_name[n] <= '9'); n++, len++) {}
				if (n > 0 && n < 4 && (file_name[n] >= 'a' && file_name[n] <= 'z' || file_name[n] >= 'A' && file_name[n] <= 'Z'))
					n++;
				if (len > 0 && len < 5 && (file_name[n] == '\0' || file_name[n] == ' ' && file_name[n + 1] == '-' && file_name[n + 2] == ' '))
					p_info.meta_add_ex("tracknumber", ~0, file_name.get_ptr() + begin, len);
				break;
			}
			n++;
		}
	}
#endif
}

bool input_vgm::g_is_our_path(const char * p_path, const char * p_extension)
{
	if ((::stricmp_utf8(p_extension, "vgm") == 0 || ::stricmp_utf8(p_extension, "vgz") == 0) &&
		cfg_support_vgm
		)
		return true;
	return false;
}

static input_singletrack_factory_t<input_vgm> g_input_vgm_factory;

DECLARE_FILE_TYPE_EX("vgm", "VGM file", "VGM files");
namespace file_type_vgz { DECLARE_FILE_TYPE_EX("vgz", "VGZ file", "VGZ files"); }
