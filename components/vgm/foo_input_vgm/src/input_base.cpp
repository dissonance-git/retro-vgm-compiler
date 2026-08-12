#include "input_base.h"
#include "my_cfg_external.h"

static_assert(sizeof(t_size) >= sizeof(uint32_t), "t_size is too short");
static_assert(sizeof(t_filesize) >= sizeof(uint32_t), "t_filesize is too short");

// {3624B482-6ABF-422E-A4B2-03FD8BB32850}
const GUID input_base::m_decoder_priority_table_guid =
{ 0x3624b482, 0x6abf, 0x422e, { 0xa4, 0xb2, 0x3, 0xfd, 0x8b, 0xb3, 0x28, 0x50 } };

critical_section input_base::g_lock;
pfc::list_t<input_base*> input_base::g_my_instance_list;



input_base::input_base() :  
m_data_loader(nullptr), 
m_calculated_song_len(false),
m_play_end(false), 
m_played_sample(0), 
m_render_done(0), 
m_play_infinitely(false)
{
}

input_base::~input_base()
{
	if (input_open_decode == m_open_reason)
	{
		m_main_player.Stop();
		insync(g_lock);
		t_size idx = g_my_instance_list.find_item(this);
		if (idx != ~0)
			g_my_instance_list.remove_by_idx(idx);
	}
	if (m_data_loader)
	{
		m_main_player.UnloadFile();
		DataLoader_Deinit(m_data_loader);
		m_data_loader = nullptr;
	}
	if (!m_main_player.GetRegisteredPlayers().empty())
	{
		m_main_player.UnregisterAllPlayers();
	}
}

#if 0
bool input_base::g_is_our_path(const char* p_path, const char* p_extension)
{
	if (::stricmp_utf8(p_extension, "vgm") == 0 || ::stricmp_utf8(p_extension, "vgz") == 0 ||
		::stricmp_utf8(p_extension, "s98") == 0 ||
		::stricmp_utf8(p_extension, "dro") == 0 ||
		::stricmp_utf8(p_extension, "gym") == 0)
		return true;
	return false;
}
#endif

//Generate chip name
//From in_vgm.cpp. Thanks Valley Bell!
static inline void get_current_chip_name(pfc::string_base& p_out, const std::vector<PLR_DEV_INFO>& pdi_list, bool long_chip_name)
{
	UINT8 opts = long_chip_name ? 1 : 0;
	for (size_t i = 0; i < pdi_list.size(); i++)
	{
		const PLR_DEV_INFO& pdi = pdi_list[i];
		if (pdi.parentIdx != (uint32_t)-1) continue;	//Skip linked device

		const char* chip_name = nullptr;
		size_t device_count = 1;
		
		chip_name = SndEmu_GetDevName(pdi.type, opts, pdi.devCfg);
		for (device_count = 1; i + 1 < pdi_list.size(); i++, device_count++)
		{
			const PLR_DEV_INFO& pdi_next = pdi_list[i + 1];
			const char* chip_name_next = SndEmu_GetDevName(pdi_next.type, opts, pdi_next.devCfg);
			if (chip_name != chip_name_next)	//Should we call "strcmp"?
				break;
		}
		if (pdi.type == DEVID_SN76496)
		{
			if ((pdi.devCfg->flags & 0x01) && device_count > 1)
				device_count /= 2;	//T6W28
		}
		if (!p_out.is_empty())
			p_out += ", ";

		if (device_count > 1)
		{
			p_out += pfc::format_uint(device_count);
			p_out += "x";
			p_out += chip_name;
		}
		else
		{
			p_out += chip_name;
		}
	}
}

void input_base::open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort)
{
	m_file = p_filehint;
	m_file_path = p_path;
	m_open_reason = p_reason;

	//Open input file
	input_open_file_helper(m_file, p_path, p_reason, p_abort);
	
	read_file(p_abort);

	register_player();

	load_file_to_player();

	check_if_file_is_supported_format();

	m_file_buf.force_reset();	//Free memory.

	//General options
	PlayerA::Config pa_cfg = m_main_player.GetConfiguration();
	pa_cfg.chnInvert = cfg_surround_sound ? 0x02 : 0x00;
	pa_cfg.masterVol = (INT32)(cfg_volume.get_value() * 0x10000);
	pa_cfg.masterVol /= 100;
	m_main_player.SetConfiguration(pa_cfg);
	m_sample_rate = cfg_sample_rate;
	m_bps = cfg_bps;
	m_fade_len = cfg_fade_length;
	m_fade_sample = audio_math::time_to_samples(double(m_fade_len) * .001, m_sample_rate);
	m_loop_flag = m_main_player.GetPlayer()->GetLoopTicks() ? true : false;
	m_loop_count = cfg_loop_count;
	m_loop_count_for_ifnl = 1;
	modify_loop_count_appropriately();
	m_play_infinitely = m_loop_count == 0 && m_loop_flag ? true : false;
	m_render_sample = static_cast<uint_fast32_t>(audio_math::time_to_samples((double)(m_render_ms) * .001, m_sample_rate));
	m_main_player.SetOutputSettings(m_sample_rate, m_channels, m_bps, m_render_sample);
	m_main_player.SetFadeSamples((UINT32)m_fade_sample);
	m_main_player.SetLoopCount(m_loop_count);

	//Get current chip info
	m_main_player.GetPlayer()->GetSongDeviceInfo(m_pdi_list);
	get_current_chip_name(m_chip_name, m_pdi_list, false);
	get_current_chip_name(m_accurate_chip_name, m_pdi_list, true);

	set_additional_options();
}

void input_base::read_file(abort_callback & p_abort)
{
	m_file_len = m_file->get_size(p_abort);
	if (m_file_len > m_file_len_max)
		throw exception_io_unsupported_format();
	m_file_buf.set_size((t_size)m_file_len + 16);
	m_file->read(m_file_buf.get_ptr(), (t_size)m_file_len, p_abort);
}

void input_base::load_file_to_player()
{
	m_data_loader = MemoryLoader_Init(m_file_buf.get_ptr(), (UINT32)m_file_len);
	if (NULL == m_data_loader)
	{
		m_data_loader = nullptr;
		throw std::bad_alloc();
	}
	DataLoader_SetPreloadBytes(m_data_loader, 0x100);
	UINT8 ret = DataLoader_Load(m_data_loader);
	if (ret)
	{
		DataLoader_CancelLoading(m_data_loader);
		throw exception_io_unsupported_format();
	}
	ret = m_main_player.LoadFile(m_data_loader);
	if (ret)
	{
		DataLoader_CancelLoading(m_data_loader);
		throw exception_io_unsupported_format();
	}
}

void input_base::calc_song_len()
{
	if (m_calculated_song_len)
		return;
	if (m_play_infinitely) m_main_player.SetLoopCount(1);
	m_song_len = m_main_player.GetTotalTime(PLAYTIME_LOOP_INCL | PLAYTIME_TIME_FILE);
	m_song_sample = audio_math::time_to_samples(m_song_len, m_sample_rate);
	if (m_play_infinitely) m_main_player.SetLoopCount(m_loop_count);

	if (m_loop_flag && !m_play_infinitely)
	{
		m_song_sample += m_fade_sample;
		m_song_len += (double)m_fade_len * .001;
	}
	m_calculated_song_len = true;
}

void input_base::meta_add_for_multi_value_field(file_info& p_info, const char* name, const char* value)
{
	if (!value || '\0' == value[0] || !name)
		return;

	const char* begin = value;
	t_size len = 0;

	while (1)
	{
		if (value[0] == '\0' || value[0] == ',' && value[1] == ' ' )
		{
			if (len > 0)
				p_info.meta_add_ex(name, ~0, begin, len);

			if (*value == '\0') break;
			len = 0;
			if (*value == ',')
			value += 2;
			else
				value += 3;

			begin = value;
			continue;
		}
		len++;
		value++;
	}
}

void input_base::meta_add_for_multi_line_field(file_info& p_info, const char* name, const char* value)
{
	if (!value || '\0' == value[0] || !name)
		return;
	pfc::array_t<char> converted_buf;
	t_size value_len = strlen(value);
	converted_buf.set_size(value_len + 1);
	if (converted_buf.get_size() < value_len) throw pfc::exception_overflow();

	const int CR = 13;
	const int LF = 10;
	t_size i = 0;
	t_size p = 0;
	for (i = 0; i < value_len; i++)
	{
		if (converted_buf.get_size() <= i + p + 1) converted_buf.increase_size(16);
		if (value[i] == '\0')
			break;
		if (value[i] == CR && value[i + 1] != LF)
		{
			converted_buf[i + p] = CR;
			p++;
			converted_buf[i + p] = LF;
			continue;
		}
		else if (value[i] == CR && value[i + 1] == LF)
		{
			converted_buf[i + p] = CR;
			converted_buf[i + p] = LF;
			continue;
		}
		else if (value[i] == LF)
		{
			converted_buf[i + p] = CR;
			p++;
			converted_buf[i + p] = LF;
			continue;
		}
		converted_buf[i + p] = value[i];
	}
	if (converted_buf.get_size() <= i + p) converted_buf.increase_size(1);
	converted_buf[i + p] = '\0';
	p_info.meta_add(name, converted_buf.get_ptr());
}

static inline void add_chip_info(file_info& p_info, const std::vector<PLR_DEV_INFO>& pdi_list, pfc::string_base & accurate_chip_name_list)
{
	for (size_t i = 0; i < pdi_list.size(); i++)
	{
		const PLR_DEV_INFO& pdi = pdi_list[i];
		if (pdi.parentIdx != (uint32_t)-1) continue;
		const char *accurate_chip_name = SndEmu_GetDevName(pdi.type, 1, pdi.devCfg);
		size_t device_count = 1;

		pfc::string8_fast field_name = accurate_chip_name;
		if (pdi.instance == 0)
		{
			t_size pos = accurate_chip_name_list.find_first(accurate_chip_name);
			if (pos != ~0 && pos > 0 && accurate_chip_name_list[pos - 1] == 'x')
			{
				if (pos > 1)
					device_count = pfc::char_to_dec(accurate_chip_name_list[pos - 2]);
			}

			if (device_count > 1)
				field_name += "_1";
		}
		else if (pdi.instance < 0xFF)
		{
			if (pdi.type == DEVID_SN76496 && (pdi.devCfg->flags & 0x01))
				continue;

			field_name += "_";
			field_name += pfc::format_uint(pdi.instance + 1, 0, 10);
		}

		field_name += "_CLOCK_RATE";
		p_info.info_set_int(field_name.get_ptr(), pdi.devCfg->clock);
	}
}

void input_base::get_info(file_info & p_info, abort_callback & p_abort)
{
	calc_song_len();
	p_info.set_length(m_song_len);
	p_info.info_set_int("channels", m_channels);
	p_info.info_set("encoding", "synthesized");
	
	PLR_SONG_INFO psi;
	m_main_player.GetPlayer()->GetSongInfo(psi);
	p_info.info_set_float("VOLUME", psi.volGain / (double)0x10000, 2);

	p_info.info_set("CHIP_NAME", m_chip_name.get_ptr());
	p_info.info_set("ACCURATE_CHIP_NAME", m_accurate_chip_name.get_ptr());

	add_chip_info(p_info, m_pdi_list, m_accurate_chip_name);

	set_additional_info(p_info, p_abort);
}

bool input_base::decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
{
	if (m_dynamic_info)
	{
		m_dynamic_info = false;
		p_out.info_set_int("samplerate", m_sample_rate);
		p_out.info_set_int("bitspersample", m_bps);
		return true;
	}
	return false;
}

static inline UINT8 get_chip_sample_rate_mode_for_plr_dev_opts(UINT8 chip_type, UINT8 my_chip_sample_mode)
{
	switch (my_chip_sample_mode)
	{
	case 0:
		return DEVRI_SRMODE_NATIVE;
	case 1:
		return DEVRI_SRMODE_HIGHEST;
	case 2:
		return DEVRI_SRMODE_CUSTOM;
	case 3:
	{
		if (chips_options::is_FM_chip(chip_type))
			return DEVRI_SRMODE_NATIVE;
		return DEVRI_SRMODE_HIGHEST;
	}
	default:
		break;
	}
	return 0;
}

void input_base::decode_initialize(unsigned int p_flags, abort_callback & p_abort)
{
	m_decode_flags = p_flags;
	if ((m_decode_flags & input_flag_no_looping) && m_play_infinitely)
	{
		m_play_infinitely = false;
		m_loop_count = m_loop_count_for_ifnl;
		m_main_player.SetLoopCount(m_loop_count);
		m_calculated_song_len = false;
	}

	calc_song_len();

	UINT8 resampling_mode = static_cast<UINT8>(cfg_resampling_mode);
	UINT8 chip_sample_rate_mode = static_cast<UINT8>(cfg_chip_sample_rate_mode);
	UINT32 chip_sample_rate = cfg_chip_sample_rate;
	PlayerBase* p_player = m_main_player.GetPlayer();

	auto set_chip_options = [&resampling_mode, &chip_sample_rate_mode, &chip_sample_rate, &p_player](uint8_t type, uint8_t instance)
	{
		UINT32 id = PLR_DEV_ID(type, instance);
		PLR_DEV_OPTS pdo;
		UINT8 ret = p_player->GetDeviceOptions(id, pdo);
		if (ret)
			return;
		unsigned index = chips_options::get_index_from_chip_type(type);
		if (index >= chips_options::g_chip_type_count)
			return;
		cfg_chips_options.copy<~0>(pdo, index, instance);
		pdo.resmplMode = resampling_mode;
		pdo.srMode = get_chip_sample_rate_mode_for_plr_dev_opts(type, chip_sample_rate_mode);
		pdo.smplRate = chip_sample_rate;
		p_player->SetDeviceOptions(id, pdo);
	};

	for (const PLR_DEV_INFO & e : m_pdi_list)
	{
		set_chip_options(e.type, e.instance);
		if (chips_options::has_linked_chip(e.type))
		{
			set_chip_options(chips_options::get_linked_chip_type(e.type), e.instance);
		}
	}

	t_uint64 pause_sample = audio_math::time_to_samples((double)(m_loop_flag ? cfg_pause_looping : cfg_pause_non_looping) * .001, m_sample_rate);
	m_main_player.SetEndSilenceSamples((UINT32)pause_sample);
	m_main_player.SetEventCallback(input_base::event_callback, this);
	m_main_player.SetFileReqCallback(input_base::file_req_callback, this);
	m_main_player.Start();

	m_render_byte = m_render_sample * m_channels * (m_bps / 8);
	m_decode_buf.set_size((t_size)(m_render_byte * 2));
	m_play_end = false;
	m_played_sample = 0;
	m_render_done = 0;
	m_dynamic_info = true;

	insync(g_lock);
	g_my_instance_list.add_item(this);
}

bool input_base::decode_run(audio_chunk & p_chunk, abort_callback & p_abort)
{
	if (m_play_end)
		return false;
    uint32_t render_byte = m_main_player.Render((UINT32)m_render_byte, m_decode_buf.get_ptr());
	if (0 == render_byte && m_play_end)
		return false;

	uint32_t render_sample = render_byte;
	render_sample /= m_channels * m_bps / 2;

	p_chunk.set_data_fixedpoint(
		m_decode_buf.get_ptr(),
		(t_size)render_byte,
		m_sample_rate, m_channels, m_bps,
		audio_chunk::g_guess_channel_config(m_channels));

	if (!m_play_infinitely)
		m_played_sample += render_sample;
	m_render_done = render_sample;
	return true;
}

bool input_base::decode_can_seek()
{
	if (m_play_infinitely)
		return false;
	return true;
}

void input_base::decode_seek(double p_seconds, abort_callback & p_abort)
{
	t_uint64 seek_sample = ::audio_math::time_to_samples(p_seconds, m_sample_rate);

	if (seek_sample < 0 || seek_sample > m_song_sample + 1) throw exception_io_seek_out_of_range();

	m_dynamic_info = true;
	m_render_done = 0;

	m_main_player.Seek(PLAYPOS_SAMPLE, (UINT32)seek_sample);

	m_played_sample = seek_sample;
}

void input_base::decode_on_idle(abort_callback & p_abort)
{
}

void input_base::set_pause(bool paused)
{
}

void input_base::retag(const file_info & p_info, abort_callback & p_abort)
{
	retag_internal(p_info, p_abort);
}

void input_base::remove_tags(abort_callback& p_abort)
{
	remove_tags_internal(p_abort);
}

UINT8 input_base::event_callback(PlayerBase * player, void * user_param, UINT8 evt_type, void * evt_param)
{
	input_base* p_input_general = reinterpret_cast<input_base*>(user_param);
	switch (evt_type)
	{
	case PLREVT_END:
	{
		p_input_general->m_play_end = true;
		break;
	}
	default:
		break;
	}
	return 0;
}

DATA_LOADER* input_base::file_req_callback(void* user_param, PlayerBase* player, const char* file_name)
{
	pfc::string8 file_req_path;
	uGetModuleFileName(NULL, file_req_path);
	file_req_path.truncate(file_req_path.scan_filename());
	file_req_path += file_name;

	abort_callback_dummy abort;
	if (!filesystem::g_exists(file_req_path.c_str(), abort))
	{ 
		pfc::string_formatter console_msg = "Failed to find \"";
		console_msg += file_name;
		console_msg += "\".";
		console::warning(console_msg);
		return NULL;
	}

	service_ptr_t<file> p_file_req;
	filesystem::g_open(p_file_req, file_req_path, filesystem::open_mode_read, abort);
	t_filesize file_req_len = p_file_req->get_size(abort);
	if (file_req_len > input_base::m_file_len_max)
		return NULL;
	pfc::array_t<uint8_t> file_req_buf;
	file_req_buf.set_size((t_size)file_req_len + 16);
	p_file_req->read(file_req_buf.get_ptr(), (t_size)file_req_len, abort);

	DATA_LOADER* p_file_req_data_loader = MemoryLoader_Init(file_req_buf.get_ptr(), (UINT32)file_req_len);
	if (NULL != p_file_req_data_loader)
	{
		UINT8 ret = DataLoader_Load(p_file_req_data_loader);
		if (!ret)
			return p_file_req_data_loader;
		DataLoader_Deinit(p_file_req_data_loader);
	}
	p_file_req_data_loader = nullptr;
	pfc::string_formatter console_msg = "Failed to read \"";
	console_msg += file_name;
	console_msg += "\".";
	console::warning(console_msg);
	return NULL;
}

void input_base::update_muting(const PLR_DEV_OPTS_EX & p_opts)
{
	UINT32 id = p_opts.chip_id;
	PLR_MUTE_OPTS temp_opts;
	insync(g_lock);
	for (t_size i = 0; i < g_my_instance_list.get_size(); i++)
	{
		PlayerBase* p_player = g_my_instance_list[i]->m_main_player.GetPlayer();
		UINT8 ret = p_player->GetDeviceMuting(id, temp_opts);
		if (ret)
			continue;
		p_player->SetDeviceMuting(id, p_opts.muteOpts);
	}
}

void input_base::update_muting_of_all_current_chips()
{
	PLR_MUTE_OPTS pmo;
	insync(g_lock);
	for (t_size i = 0; i < g_my_instance_list.get_size(); i++)
	{
		input_base* p = g_my_instance_list[i];
		PlayerBase* p_player = p->m_main_player.GetPlayer();
		for (const PLR_DEV_INFO & e : p->m_pdi_list)
		{
			UINT32 id = PLR_DEV_ID(e.type, e.instance);
			UINT8 ret = p_player->GetDeviceMuting(id, pmo);
			if (ret)
				continue;

			unsigned index = chips_options::get_index_from_chip_type(e.type);
			const PLR_MUTE_OPTS & src_pmo = cfg_chips_options[index][e.instance].muteOpts;
			pmo.disable = src_pmo.disable;
			for (size_t m = 0; m < std::size(pmo.chnMute); m++)
				pmo.chnMute[m] = src_pmo.chnMute[m];
			p_player->SetDeviceMuting(id, pmo);
		}
	}
}

void input_base::update_panning(const PLR_DEV_OPTS_EX& p_opts)
{
	UINT32 id = p_opts.chip_id;
	PLR_DEV_OPTS opts;
	insync(g_lock);
	for (t_size i = 0; i < g_my_instance_list.get_size(); i++)
	{
		PlayerBase* p_player = g_my_instance_list[i]->m_main_player.GetPlayer();
		UINT8 ret = p_player->GetDeviceOptions(id, opts);
		if (ret)
			continue;
		
		for (size_t m = 0; m < std::size(opts.panOpts.chnPan); m++)
			for (size_t n = 0; n < std::size(*opts.panOpts.chnPan); n++)
				opts.panOpts.chnPan[m][n] = p_opts.panOpts.chnPan[m][n];
		
		p_player->SetDeviceOptions(id, opts);
	}
}

void input_base::update_panning_of_all_current_chips()
{
	PLR_DEV_OPTS opts;
	insync(g_lock);
	for (t_size i = 0; i < g_my_instance_list.get_size(); i++)
	{
		input_base* p = g_my_instance_list[i];
		PlayerBase* p_player = p->m_main_player.GetPlayer();
		for (const PLR_DEV_INFO& e : p->m_pdi_list)
		{
			UINT32 id = PLR_DEV_ID(e.type, e.instance);
			UINT8 ret = p_player->GetDeviceOptions(id, opts);
			if (ret)
				continue;

			unsigned index = chips_options::get_index_from_chip_type(e.type);
			const PLR_DEV_OPTS_EX& src_opts = cfg_chips_options[index][e.instance];
			for (size_t m = 0; m < std::size(opts.panOpts.chnPan); m++)
				for (size_t n = 0; n < std::size(*opts.panOpts.chnPan); n++)
					opts.panOpts.chnPan[m][n] = src_opts.panOpts.chnPan[m][n];

			p_player->SetDeviceOptions(id, opts);
		}
	}
}

bool input_base::get_current_playback_chip_status(uint8_t & p_first_chip_type, pfc::string_base & p_chip_name)
{
	insync(g_lock);
	for (t_size i = 0; i < g_my_instance_list.get_size(); i++)
	{
		input_base* p = g_my_instance_list[i];
		if (p->m_decode_flags & input_flag_playback)
		{
			p_first_chip_type = p->m_pdi_list[0].type;
			p_chip_name = p->m_chip_name;
			return true;
		}
	}
	return false;
}
