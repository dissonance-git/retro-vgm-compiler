/**
 * foobar2000 wrapper for SNESAPU.dll
 *
 *     Licence :
 *        GNU General Public License
 *
 */

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#include "stdafx.h"
#include "input_snesapu.hpp"
#include "id666.hpp"
#include "idex666.hpp"
#include "script700.hpp"
#include <mbstring.h>

#ifdef _WIN64
#else
#ifndef _DEBUG
pfc::array_t<t_uint8>	input_snesapu::m_DllFileBuffer;
t_size					input_snesapu::m_DllFileLength = 0;
#endif
#endif

#define SPC_MAGIC "SNES-SPC700 Sound File Data "
#define DLL_NAME  "SNESAPU.DLL"
#define WRITE_SUPPORT

static const u32 SPC_MAX_CH = 8;

// larger values cause seeking distortion [ms]
static const u32 BUFFER_DURATION = 100;

// 0x1000*0x80
static const size_t EAR_SAFE_AMP_MAX = 524288;

// preferences page
const GUID guid_snesapu_preference_page =
{ 0xa8e61d30, 0xb2e2, 0x4cce,{ 0xad, 0x31, 0x73, 0x9e, 0x6c, 0xa8, 0x5d, 0xf1 } };

// foobar2000-configurable variables
extern cfg_int cfg_samplerate;
extern cfg_int cfg_bitspersample;
extern cfg_int cfg_channels;
extern cfg_int cfg_interpolation;
extern cfg_int cfg_dsp_option;
extern cfg_int cfg_fastseek;
extern cfg_int cfg_usespclength;
extern cfg_int cfg_forcelength;
extern cfg_int cfg_songlength;
extern cfg_int cfg_fadelength;
extern cfg_int cfg_amplification;
extern cfg_int cfg_ignore_script700;
extern cfg_int cfg_ignore_xid6_amplification;
extern cfg_int cfg_sem71_enabled;

/**
 * Get SNESAPU.DLL's kind.
 *
 * @return Original / Sunburst ver
 */
input_snesapu::SNESAPU_CORE input_snesapu::GetKind()
{
#ifdef _WIN64
	return CORE_SSDLabo;
#else
	if (!m_DllInst) {
		if ( !LoadDll() )
			throw exception_io_data("Unable to load SNESAPU.dll");
	}
	if (m_Apu.SetScript700) {
		return CORE_SSDLabo;
	}
	return CORE_ALPHA2;
#endif
}

/**
 * Load SNEAPU.DLL
 *
 * @return true / false
 */
bool input_snesapu::LoadDll()
{
#ifdef _WIN64
#else
	if (m_DllInst) {
		return true;
	}

	pfc::string8 path = core_api::get_my_full_path();
	path.truncate( path.scan_filename() );
	path += DLL_NAME;

#ifdef _DEBUG
	m_DllInst = uLoadLibrary(path);
#else
	if (0 == m_DllFileLength)
	{
		service_ptr_t<file> p_dll_file;
		abort_callback_dummy abort_dummy;
		filesystem::g_open(p_dll_file, path, filesystem::open_mode_read, abort_dummy);
		m_DllFileLength = static_cast<t_size>(p_dll_file->get_size(abort_dummy));
		m_DllFileBuffer.resize(m_DllFileLength);
		p_dll_file->read(m_DllFileBuffer.get_ptr(), m_DllFileLength, abort_dummy);
	}
	m_DllInst = MemoryLoadLibrary(m_DllFileBuffer.get_ptr(), m_DllFileLength);

#define GetProcAddress MemoryGetProcAddress
#endif

	::ZeroMemory(&m_Apu, sizeof(m_Apu));
	if (m_DllInst) {
		*(void**)&m_Apu.EmuAPU			= (void*)GetProcAddress(m_DllInst, "EmuAPU");
		*(void**)&m_Apu.GetAPUData		= (void*)GetProcAddress(m_DllInst, "GetAPUData");
		*(void**)&m_Apu.LoadSPCFile		= (void*)GetProcAddress(m_DllInst, "LoadSPCFile");
		*(void**)&m_Apu.SeekAPU			= (void*)GetProcAddress(m_DllInst, "SeekAPU");
		*(void**)&m_Apu.SetAPULength	= (void*)GetProcAddress(m_DllInst, "SetAPULength");
		*(void**)&m_Apu.SetAPUOpt		= (void*)GetProcAddress(m_DllInst, "SetAPUOpt");
		*(void**)&m_Apu.SetDSPAmp		= (void*)GetProcAddress(m_DllInst, "SetDSPAmp");
//		*(void**)&m_Apu.SetDSPEFBCT		= (void*)GetProcAddress(m_DllInst, "SetDSPEFBCT");
//		*(void**)&m_Apu.SetDSPPitch		= (void*)GetProcAddress(m_DllInst, "SetDSPPitch");
//		*(void**)&m_Apu.SetDSPStereo	= (void*)GetProcAddress(m_DllInst, "SetDSPStereo");
		*(void**)&m_Apu.SNESAPUInfo		= (void*)GetProcAddress(m_DllInst, "SNESAPUInfo");

		*(void**)&m_Apu.SetScript700	= (void*)GetProcAddress(m_DllInst, "SetScript700");
//		*(void**)&m_Apu.InitWork_700	= (void*)GetProcAddress(m_DllInst, "InitWork_700");

		m_Apu.GetAPUData(&m_Apu.ram, &m_Apu.xram, NULL, &m_Apu.t64Cnt,
						 &m_Apu.dsp, &m_Apu.voice, &m_Apu.vMMaxL, &m_Apu.vMMaxR);
	}

#ifdef GetProcAddress
#undef GetProcAddress
#endif

	if (m_DllInst == NULL) {
		console::info("Could not locate SNESAPU.DLL.");
		return false;
	}

	if (m_Apu.SNESAPUInfo == NULL || m_Apu.LoadSPCFile == NULL || m_Apu.EmuAPU == NULL) {
		console::info("SNESAPU.DLL function lookups failed.");
		return false;
	}

	u32 Version, Minimum, Options;
	m_Apu.SNESAPUInfo(&Version, &Minimum, &Options);
	if (Version < 0x20000 || Minimum > 0x20000) {
		console::info("Incompatible SNESAPU.DLL version.");
		FreeDll();
		return false;
	}
#endif
	return true;
}

/**
 * Free SNESAPU.DLL
 *
 */
void input_snesapu::FreeDll()
{
#ifdef _WIN64
#else
	if (!m_DllInst) {
		return;
	}

	if (m_DllInst) {
#ifdef _DEBUG
		FreeLibrary(m_DllInst);
#else
		MemoryFreeLibrary(m_DllInst);
#endif
		m_DllInst = NULL;
	}
#endif
}

/**
 * Destructor
 *
 */
input_snesapu::~input_snesapu()
{
	FreeDll();
}

void meta_add_multi_value_field(file_info& p_info, const char* name, const char* value)
{
	if (!value || '\0' == value[0] || !name)
		return;

	const char* begin = value;
	t_size len = 0;

	while (1)
	{
		if (value[0] == '\0' || value[0] == ',' && value[1] == ' ')
		{
			if (len > 0)
				p_info.meta_add_ex(name, ~0, begin, len);

			if (*value == '\0') break;
			len = 0;
			value += 2;
			begin = value;
			continue;
		}
		len++;
		value++;
	}
}

/**
 * Get file information
 *
 * @param p_info
 * @param p_abort
 */
void input_snesapu::get_info(file_info &p_info, abort_callback &p_abort)
{
	if (m_CnfForceLength) {
		p_info.info_set_int("spc_songlength", m_TagTimeIntro);	//millisecond
		p_info.info_set_int("spc_fadelength", m_TagTimeFade);
		p_info.set_length(static_cast<double>(m_SongLength + m_FadeLength) / 1000.0);
	}
	if (m_TagSong.is_empty() == false) {
		p_info.meta_set("title", m_TagSong);
	}
	if (m_TagGame.is_empty() == false) {
		p_info.meta_set("album", m_TagGame);
	}
	if (m_TagArtist.is_empty() == false) {
		meta_add_multi_value_field(p_info, "artist", m_TagArtist);	//We treat "artist" as multi value field. separator is ', '.
	}
	if (m_TagDumper.is_empty() == false) {
		p_info.meta_set("dumper", m_TagDumper);
	}
	if (m_TagComment.is_empty() == false) {
		p_info.meta_set("comment", m_TagComment);
	}
	// Date song was dumped
	if (m_TagDate.is_empty() == false) {
		p_info.info_set("spc_dumped_date", m_TagDate);
	}
	// Publisher's name
	if (m_TagPublisher.is_empty() == false) {
		p_info.meta_set("copyright", m_TagPublisher);
	}
	// Copyright year
	if (m_TagCopylight.is_empty() == false) {
		p_info.meta_set("date", m_TagCopylight);
	}
	if (m_TagOstName.is_empty() == false) {
		p_info.meta_set("ost", m_TagOstName);
	}
	if (m_TagOstDisc.is_empty() == false) {
		p_info.meta_set("discnumber", m_TagOstDisc);
	}
	if (m_TagOstTrack.is_empty() == false) {
		p_info.meta_set("tracknumber", m_TagOstTrack);
	}
	if (m_TagEmulater.is_empty() == false) {
		p_info.meta_set("emulator", m_TagEmulater);
	}
	if (m_TagAmp != 0xFFFFFFFF) {
		p_info.info_set_int("spc_amplification", m_TagAmp);
	}

	p_info.info_set("codec", "SPC");
}

bool input_snesapu::decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta)
{
	if ( m_DynInfo ) {
		m_DynInfo = false;
		p_out.info_set_int("samplerate", m_CnfSampleRate);
		p_out.info_set_int("channels", m_CnfChannels);
		p_out.info_set_int("bitspersample", m_CnfBPS);
		p_timestamp_delta = -((double)m_LastDecoded / 1000.0);
		return true;
	}
	return false;
}

/**
 * Open file
 *
 * @param p_filehint
 * @param p_path
 * @param p_reason
 * @param p_abort
 *
 */
void input_snesapu::open(service_ptr_t<file> p_filehint, const char *p_path, t_input_open_reason p_reason, abort_callback &p_abort)
{
#if !defined(WRITE_SUPPORT)
	if (p_reason == input_open_info_write) {
		throw exception_io_unsupported_format();
	}
#endif

	m_File = p_filehint;
	m_SpcPath.set_string(p_path);
	input_open_file_helper(m_File, p_path, p_reason, p_abort);

	// read spc file
	m_SpcFileLength	= static_cast<u32>(m_File->get_size(p_abort));
	m_SpcFile.set_size(m_SpcFileLength);
	m_File->read(m_SpcFile.get_ptr(), m_SpcFileLength, p_abort);

	// check header
	if (::memcmp(m_SpcFile.get_ptr(), SPC_MAGIC, 28)) {
		throw exception_io_unsupported_format();
	}

	// load settings
	m_CnfSampleRate		         = cfg_samplerate;
	m_CnfBPS			         = cfg_bitspersample;
	m_CnfChannels		         = cfg_channels;
	m_CnfMixing			         = 3;
	m_CnfInterpolation	         = cfg_interpolation;
	m_CnfOptions		         = cfg_dsp_option;
	m_CnfFastSeek		         = cfg_fastseek?true:false;
	m_CnfForceLength	         = cfg_forcelength?true:false;
	m_CnfUseSpcLength	         = cfg_usespclength?true:false;
	m_SongLength		         = cfg_songlength;
	m_FadeLength		         = cfg_fadelength;
	m_CnfIgnoreScript700         = cfg_ignore_script700 ? true : false;
	m_CnfIgnoreXID6Amplification = cfg_ignore_xid6_amplification ? true : false;

	ReadSpcInfo();
}

/**
 * Initialize for decode
 *
 * @param p_flags
 * @param p_abort
 *
 */
void input_snesapu::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
	if (LoadDll() == false) {
		throw exception_io_data("Unable to load SNESAPU.dll");
	}

	// load script700
	switch (GetKind()) {
	  case CORE_SSDLabo:
	  {
		script700 parser;
		if ( !m_CnfIgnoreScript700 && parser.load(m_SpcPath) == true) {
			void *pScript = parser.get_ptr();
			if (pScript) {
#ifdef _WIN64
				if ((s32)(m_Apu.SetScript700(pScript, parser.get_size())) <= -1) {
#else
				if ((s32)(m_Apu.SetScript700(pScript)) <= -1) {
#endif
					console::info("Script700 parse error");
				}
			}
		} else {
#ifdef _WIN64
#else
			m_Apu.SetScript700(NULL);
#endif
		}
		break;
	  }
	  default:
		break;
	}

	// send options/file to snesapu
#ifdef _WIN64
	m_Apu.LoadSPCFile(m_SpcFile.get_ptr(), m_SpcFileLength);
#else
	m_Apu.LoadSPCFile(m_SpcFile.get_ptr());
#endif

	m_Apu.SetAPUOpt(m_CnfMixing, m_CnfChannels, m_CnfBPS, m_CnfSampleRate, m_CnfInterpolation, m_CnfOptions);
	m_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
	m_Apu.SetVoiceMute(m_TagMask);
#else
	for (u32 i=0; i < SPC_MAX_CH; i++) {
		// mute
		m_Apu.voice[i].mFlg = ((m_TagMask >> i) & 0x01)?1:0;
		// init max volume
		m_Apu.voice[i].vMaxL = m_Apu.voice[i].vMaxR = 0;
	}

	*m_Apu.vMMaxL = *m_Apu.vMMaxR = 0;
#endif
	m_PlayedTime = 0;
	m_SilentTime = 0;

	if (m_CnfForceLength) {
		m_Apu.SetAPULength(m_SongLength * 64, m_FadeLength * 64);
	}
	// if finite playtime is wanted, enable forced length
	if (p_flags & input_flag_no_looping) {
		m_CnfUseSpcLength = m_CnfForceLength = true;
	}

#ifdef _WIN64
	// Semantic Stereo Enhancer (spatial pre-conditioning)
	m_Sem71Enabled = cfg_sem71_enabled;
	m_Apu.SetTelemetryEnabled(m_Sem71Enabled);

	m_Apu.end_decode_initialization();
#else
	(void)0;
#endif

	// allocate stereo decode buffer (always 2ch from spcplayer)
	u32 BufferLength = static_cast<u32>(((BUFFER_DURATION * m_CnfSampleRate) + BUFFER_DURATION) / 1000);
	BufferLength = BufferLength * 2 * (m_CnfBPS / 8);  // always 2ch from spcplayer
	m_DecodeBuffer.set_size(BufferLength);

#ifdef _WIN64
	if (m_Sem71Enabled)
	{
		// 7.1 output: 8 channels, same bit depth, same sample count
		u32 Buf71Length = static_cast<u32>(((BUFFER_DURATION * m_CnfSampleRate) + BUFFER_DURATION) / 1000);
		Buf71Length = Buf71Length * 8 * (m_CnfBPS / 8);
		m_Buf71.set_size(Buf71Length);
		m_Enhancer.reset();
	}
#endif

	m_DynInfo = true;
	m_LastDecoded = 0;
}

/**
 * Decode
 *
 * @param p_chunk
 * @param p_abort
 * @return true / false
 */
bool input_snesapu::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
	// decide how many samples we want from sapu
	u32 wanted_ms = BUFFER_DURATION;

	// clip samples requested if nearing song end
	if (m_CnfForceLength) {
		if (m_PlayedTime >= (m_SongLength + m_FadeLength)) {
			return false;
		}
		
		if ((m_PlayedTime + wanted_ms) > (m_SongLength + m_FadeLength)) {
			wanted_ms = (m_SongLength + m_FadeLength) - m_PlayedTime;
		}
	}

	if (m_UseDefaultTime && m_SilentTime >= 5000) {
		return false;
	}
	
	// EmuAPU() wants no. of samples to render
	u32 wanted_sample = ((wanted_ms * m_CnfSampleRate) / 1000);

	{
		// Stereo output path — feeds Scene7 Spatial DSP downstream.
#ifdef _WIN64
		m_Apu.EmuAPU_with_telem(m_DecodeBuffer.get_ptr(), wanted_sample, 1, m_Telem);
#else
		m_Apu.EmuAPU(m_DecodeBuffer.get_ptr(), wanted_sample, 1);
#endif

#ifdef _WIN64
		if (m_Sem71Enabled)
		{
			m_Classifier.update(m_Telem, (float)wanted_ms);
			ChipSemanticHintBus::instance().publish(m_Adapter.make_hint());

			if (m_CnfBPS == 16)
			{
				m_Enhancer.process_s16(
					(const int16_t*)m_DecodeBuffer.get_ptr(),
					wanted_sample, m_Telem, m_Classifier,
					(int16_t*)m_DecodeBuffer.get_ptr());
			}
			else if (m_CnfBPS == 32)
			{
				m_Enhancer.process_s32(
					(const int32_t*)m_DecodeBuffer.get_ptr(),
					wanted_sample, m_Telem, m_Classifier,
					(int32_t*)m_DecodeBuffer.get_ptr());
			}
		}
#endif

		p_chunk.set_data_fixedpoint(
			m_DecodeBuffer.get_ptr(),
			wanted_sample * m_CnfChannels * (m_CnfBPS / 8),
			m_CnfSampleRate, m_CnfChannels, m_CnfBPS,
			audio_chunk::g_guess_channel_config(m_CnfChannels));

		// check silent
		if (m_UseDefaultTime) {
			bool detect(false);
#ifdef _WIN64
			audio_sample* p_sample = p_chunk.get_data();
			for (size_t i = 0; i < wanted_sample; i++)
			{
				for (size_t k = 0; k < m_CnfChannels; k++)
				{
					if (p_sample[k] != 0)
					{
						detect = true;
						goto DETECT_NON_SILENCE;
					}
				}
				p_sample += m_CnfChannels;
			}
DETECT_NON_SILENCE:
#else
			for (u32 i = 0; i < SPC_MAX_CH; i++) {
				if ((m_Apu.dsp->voice[i].volL || m_Apu.dsp->voice[i].volR) && m_Apu.dsp->voice[i].envx) {
					detect = true;
					break;
				}
			}
#endif
			if (detect)
				m_SilentTime = 0;
			else
				m_SilentTime += wanted_ms;
		}
	}

	m_PlayedTime += wanted_ms;

	m_LastDecoded = wanted_ms;

	return true;
}

/**
 * Seek
 *
 * @param p_seconds
 * @param p_abort
 */
void input_snesapu::decode_seek(double p_seconds, abort_callback &p_abort)
{
	// compare desired seek pos with current pos
#ifdef _WIN64
	u32 NewPos = static_cast<u32>(p_seconds * 1000.0);
#else
	u32 NewPos = static_cast<u32>(p_seconds * 64 * 1000.0);
#endif

	m_DynInfo = true;
	m_LastDecoded = 0;

	// reset apu if necessary
#ifdef _WIN64
	if (m_PlayedTime > NewPos)
	{
		m_Apu.restart();
		m_PlayedTime = 0;
		m_Enhancer.reset();
	}

	u32 wanted_ms = BUFFER_DURATION;
	u32 wanted_sample = ((wanted_ms * m_CnfSampleRate) / 1000);
	while (NewPos - m_PlayedTime > wanted_ms)
	{
		p_abort.check();
		m_Apu.EmuAPU(m_DecodeBuffer.get_ptr(), wanted_sample, 1);
		m_PlayedTime += wanted_ms;
	}
	u32 remain_ms = NewPos - m_PlayedTime;
	if (remain_ms > 0)
	{
		u32 remain_sample = ((remain_ms * m_CnfSampleRate) / 1000);
		m_Apu.EmuAPU(m_DecodeBuffer.get_ptr(), remain_sample, 1);
	}
#else
	if (*m_Apu.t64Cnt >= NewPos) {
		m_Apu.LoadSPCFile(m_SpcFile.get_ptr());
		if (m_CnfForceLength) {
			m_Apu.SetAPULength(m_SongLength * 64, m_FadeLength * 64);
		}
	}
	m_Apu.SeekAPU((NewPos - *m_Apu.t64Cnt), m_CnfFastSeek?0xff:0);
#endif
	m_SilentTime = 0;
	m_PlayedTime = static_cast<u32>(p_seconds * 1000);
}

/**
 * Is seekeable?
 *
 * @return true / false
 */
bool input_snesapu::decode_can_seek()
{
	return m_CnfForceLength;
}

/**
 * Read ID666 tag
 *
 */
void input_snesapu::ReadID666()
{
	SPCHEAD *SpcHead = reinterpret_cast<SPCHEAD*>(m_SpcFile.get_ptr());

	pfc::stringcvt::string_utf8_from_ansi ansi_data(SpcHead->song, ID6_TEXT_MAX);
	m_TagSong.set_string(ansi_data);
	ansi_data.convert(SpcHead->game, ID6_TEXT_MAX);
	m_TagGame.set_string(ansi_data);
	ansi_data.convert(SpcHead->dumper, ID6_DUMPER_MAX);
	m_TagDumper.set_string(ansi_data);
	ansi_data.convert(SpcHead->comment, ID6_TEXT_MAX);
	m_TagComment.set_string(ansi_data);

	// Discrimination text / binary format
	if (m_SpcFile[0xd2] < 0x30) {
		// binary format
		if (SpcHead->bin.artist[0] != 0) {
			ansi_data.convert(SpcHead->bin.artist, ID6_TEXT_MAX);
			m_TagArtist.set_string(ansi_data);
		}
		if (SpcHead->bin.year && SpcHead->bin.year <= 9999) {
#if FOOBAR2000_SDK_VERSION >= 20220810
			pfc::string8_fast date_format = pfc::string_printf("%d%c%02d%c%02d", SpcHead->bin.year, '-', SpcHead->bin.month, '-', SpcHead->bin.date);
#else
			pfc::string_printf date_format("%d%c%02d%c%02d", SpcHead->bin.year,'-',SpcHead->bin.month,'-',SpcHead->bin.date);
#endif
			m_TagDate.set_string(date_format);
		}
		// songlen
		if (SpcHead->bin.songlen) {
			m_TagTimeIntro = SpcHead->bin.songlen * 1000;
		}
		// fadelen
		int fadelen = SpcHead->bin.fadelen[0];
		fadelen += SpcHead->bin.fadelen[1] << 8;
		fadelen += SpcHead->bin.fadelen[2] <<16;
		if (fadelen) {
			m_TagTimeFade = fadelen;
		}
		// emulator
		if (SpcHead->bin.emulator >= 0 &&
			SpcHead->bin.emulator < XID6_EMULIST) {
			m_TagEmulater.set_string(emuList[SpcHead->bin.emulator]);
		}
	} else {
		// text format
		if (SpcHead->text.artist[0] != 0) {
			ansi_data.convert(SpcHead->text.artist, ID6_TEXT_MAX);
			m_TagArtist.set_string(ansi_data);
		}
		if (SpcHead->text.date[0] != 0) {
			ansi_data.convert(SpcHead->text.date, ID6_DATE_MAX);
			m_TagDate.set_string(ansi_data.get_ptr() + 6, 4);
			m_TagDate += "-";
			m_TagDate.add_string(ansi_data.get_ptr(), 2);
			m_TagDate += "-";
			m_TagDate.add_string(ansi_data.get_ptr() + 3, 2);
		}
		if (SpcHead->text.emulator >= 0x30 &&
			SpcHead->text.emulator < 0x30+XID6_EMULIST) {
			m_TagEmulater.set_string(emuList[SpcHead->text.emulator - 0x30]);
		}
		{
			char spc_songlength[4] = {0};
			char spc_fadelength[6] = {0};
			::CopyMemory(spc_songlength, SpcHead->text.songlen, 3);
			::CopyMemory(spc_fadelength, SpcHead->text.fadelen, 5);
			m_TagTimeIntro = atoi(spc_songlength) * 1000;
			m_TagTimeFade = atoi(spc_fadelength);
		}
	}
}

/**
 * Read extended ID666 tag
 *
 */
void input_snesapu::ReadExID666()
{
	if (m_SpcFileLength < 0x1020c) {
		return;
	}

	if (::memcmp(&m_SpcFile[OFS_XID6], XID6_MAGIC, 4) == 0) {
	    pfc::stringcvt::string_utf8_from_ansi ansi_data;
		s32 RemainSize = *(reinterpret_cast<s32*>(&m_SpcFile[OFS_XID6 + 4]));
		s32 RealSize = m_SpcFileLength - (OFS_XID6 + XID6_HEADER_LENGTH);

		if (RemainSize > RealSize) {
			RemainSize = RealSize;
		}

		m_ExtraDataLength = m_SpcFileLength - (OFS_XID6 + XID6_HEADER_LENGTH + RemainSize);

		XID6Chk *xid6 = reinterpret_cast<XID6Chk*>(&m_SpcFile[OFS_XID6 + XID6_HEADER_LENGTH]);
		while (RemainSize > 0) {
			RemainSize -= 4;
			s32 TagLength = (xid6->val + 3) & ~3;	// 4byte boudary

			switch (xid6->id) {
			  case XID6_SONG:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagSong.set_string(ansi_data);
				break;
			  case XID6_GAME:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagGame.set_string(ansi_data);
				break;
			  case XID6_ARTIST:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagArtist.set_string(ansi_data);
				break;
			  case XID6_PUB:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagPublisher.set_string(ansi_data);
				break;
			  case XID6_OST:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagOstName.set_string(ansi_data);
				break;
			  case XID6_DUMPER:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagDumper.set_string(ansi_data);
				break;
			  case XID6_CMNTS:
				ansi_data.convert((const char*)((&xid6->val)+1), xid6->val);
				m_TagComment.set_string(ansi_data);
				break;
			  case XID6_DISC:
				{
#if FOOBAR2000_SDK_VERSION >= 20220810
				  pfc::string8_fast disc = pfc::string_printf("%d", xid6->val);
#else
					pfc::string_printf disc("%d", xid6->val);
#endif
					m_TagOstDisc.set_string(disc);
				}
				break;
			  case XID6_TRACK:
				{
					if (xid6->val & 0xff) {
						if ((xid6->val & 0xff) > 0x20 && (xid6->val & 0xff) < 0x7f) {
#if FOOBAR2000_SDK_VERSION >= 20220810
							pfc::string8_fast track = pfc::string_printf("%d%c", (xid6->val >> 8), (xid6->val & 0xff));
#else
							pfc::string_printf track("%d%c", (xid6->val >> 8), (xid6->val & 0xff));
#endif
							m_TagOstTrack.set_string(track);
						}
						//else if ((xid6->val >> 8) > 0)	{
							//pfc::string_printf track("%d", (xid6->val >> 8));
							//m_TagOstTrack.set_string(track);
						//}
					} else {
#if FOOBAR2000_SDK_VERSION >= 20220810
						pfc::string8_fast track = pfc::string_printf("%d", (xid6->val >> 8));
#else
						pfc::string_printf track("%d", (xid6->val >> 8));
#endif
						m_TagOstTrack.set_string(track);
					}
				}
				break;
			  case XID6_COPY:
				{
#if FOOBAR2000_SDK_VERSION >= 20220810
					pfc::string8_fast copylight = pfc::string_printf("%d", xid6->val);
#else
					pfc::string_printf copylight("%d", xid6->val);
#endif
					m_TagCopylight.set_string(copylight);
				}
				break;
			  case XID6_EMU:
				m_XID6Emulater = xid6->val;
				if (xid6->val >= 0 && xid6->val < XID6_EMULIST) {
					m_TagEmulater.set_string(emuList[xid6->val]);
				}
				break;
			  case XID6_DATE:
				{
#if FOOBAR2000_SDK_VERSION >= 20220810
					pfc::string8_fast date_format = pfc::string_printf("%d%c%02d%c%02d", xid6->data >> 16, '-', (xid6->data >> 8) & 0xff, '-', xid6->data & 0xff);
#else
					pfc::string_printf date_format("%d%c%02d%c%02d", xid6->data>>16,'-',(xid6->data>>8)&0xff,'-',xid6->data & 0xff);
#endif
					m_TagDate.set_string(date_format);
					m_XID6Date = xid6->data;
					break;
				}
			  case XID6_LOOPX:
				{
				    m_XID6LoopX = m_TagLoopLimit = xid6->val;
					m_TagLoopLimit = xid6->val;
					if (m_TagLoopLimit == 0)
					  m_TagLoopLimit = 1;
					if (m_TagLoopLimit > 9)
					  m_TagLoopLimit = 9;
				}
				break;
			  case XID6_FADE:
				m_XID6TimeFade = xid6->data;
				m_TagTimeFade = xid6->data / 64;
				break;
			  case XID6_INTRO:
				m_XID6TimeIntro = xid6->data;
				m_TagTimeIntro = xid6->data / 64;
				break;
			  case XID6_LOOP:
				m_XID6TimeLoop = xid6->data;
				m_TagTimeLoop = xid6->data / 64;
				break;
			  case XID6_END:
				m_XID6TimeEnd = xid6->data;
				m_TagTimeEnd = (s32)xid6->data / 64;
				break;
			  case XID6_MUTE:
				m_TagMask = xid6->val & 0xff;
				break;
			  case XID6_AMP:
				if (xid6->type == XID6_TVAL) {
					m_XID6Amp = (xid6->val << 12);
				} else if (xid6->type == XID6_TINT) {
					m_XID6Amp =  xid6->data;
				}
				m_TagAmp = m_XID6Amp;
				if (m_TagAmp <  32768) m_TagAmp = 32768;
				if (m_TagAmp > EAR_SAFE_AMP_MAX) m_TagAmp = EAR_SAFE_AMP_MAX;
				break;
			}
			if (xid6->type != XID6_TVAL) {
				xid6 = reinterpret_cast<XID6Chk*>(reinterpret_cast<u8*>(xid6) + TagLength);
				RemainSize -= TagLength;
			}
			xid6 = reinterpret_cast<XID6Chk*>(reinterpret_cast<u8*>(xid6) + 4);
		}
	}
	else
	{
			m_ExtraDataLength = m_SpcFileLength - OFS_XID6;
	}
}

/**
 *
 */
void input_snesapu::ReadSpcInfo()
{
	ReadID666();
	ReadExID666();

	// clamp song lengths
	if (m_TagTimeIntro > XID6_LENGTH_MAX_MS) {
		m_TagTimeIntro = XID6_LENGTH_MAX_MS;
	}
	if (m_TagTimeLoop > XID6_LENGTH_MAX_MS) {
		m_TagTimeLoop = XID6_LENGTH_MAX_MS;
	}
	if (m_TagTimeEnd > (s32)XID6_LENGTH_MAX_MS) {
		m_TagTimeEnd = XID6_LENGTH_MAX_MS;
	}
	if (m_TagTimeFade > XID6_LENGTH_MAX_MS) {
		m_TagTimeFade = XID6_LENGTH_MAX_MS;
	}
	// decide song length
	if (m_CnfUseSpcLength) {
		if (m_TagTimeLoop || m_TagTimeEnd) {
			u32 temp_intro = m_TagTimeIntro;
			if (m_XID6TimeIntro == 0xFFFFFFFF)
				temp_intro = 0;
			//TimeEnd can contain a negative value.
			s32 temp_length = static_cast<s32>(temp_intro + m_TagTimeLoop * m_TagLoopLimit) + m_TagTimeEnd;
			if (temp_length > 0) {
				m_CnfForceLength = true;
				m_UseDefaultTime = false;
				m_SongLength = temp_length;
				m_FadeLength = m_TagTimeFade;
			}
			else if (m_TagTimeIntro) {
				m_CnfForceLength = true;
				m_UseDefaultTime = false;
				m_SongLength = m_TagTimeIntro;
				m_FadeLength = m_TagTimeFade;
			}
		}
		else if (m_TagTimeIntro) {	//m_TagTimeIntro is time to play song before fading out.
			m_CnfForceLength = true;
			m_UseDefaultTime = false;
			m_SongLength = m_TagTimeIntro;
			m_FadeLength = m_TagTimeFade;
		}
	}

	//decide the amplification level
	m_CurAmp = ((m_TagAmp == 0xFFFFFFFF || m_CnfIgnoreXID6Amplification) ? cfg_amplification : (m_TagAmp / 0x1000));
}

void input_snesapu::WriteID666(const file_info &p_info, abort_callback &p_abort)
{
	bool IsText = (m_SpcFile[0xd2] < 0x30)?false:true;
	SPCHEAD Header;
	::CopyMemory(&Header, m_SpcFile.get_ptr(), 0x100);
	::ZeroMemory(reinterpret_cast<u8*>(&Header)+0x2c, 0x72);

	size_t TagMax = p_info.meta_get_count();
	for (size_t i=0; i < TagMax; i++) {
		pfc::string8 TagName = p_info.meta_enum_name(i);
		pfc::stringcvt::string_ansi_from_utf8 TagValue(p_info.meta_get(TagName, 0));
		size_t TagLength = TagValue.length();
		if (TagLength > ID6_TEXT_MAX)
			TagLength = ID6_TEXT_MAX;

		if (::stricmp_utf8(TagName, "title") == 0) {
			::CopyMemory(Header.song, TagValue.get_ptr(), TagLength);
			continue;
		}
		if (::stricmp_utf8(TagName, "album") == 0) {
			::CopyMemory(Header.game, TagValue.get_ptr(), TagLength);
			continue;
		}
		if (::stricmp_utf8(TagName, "artist") == 0) {
			pfc::string8 FormattedValue;
			p_info.meta_format("artist", FormattedValue);
			TagValue.convert(FormattedValue);
			TagLength = TagValue.length();
			if (TagLength > ID6_TEXT_MAX)
				TagLength = ID6_TEXT_MAX;

			if (IsText) {
				::ZeroMemory(Header.text.artist, ID6_TEXT_MAX);
				::CopyMemory(Header.text.artist, TagValue.get_ptr(), TagLength);
			} else {
				::ZeroMemory(Header.bin.artist, ID6_TEXT_MAX);
				::CopyMemory(Header.bin.artist, TagValue.get_ptr(), TagLength);
			}
			continue;
		}
		if (::stricmp_utf8(TagName, "comment") == 0) {
			::CopyMemory(Header.comment, TagValue.get_ptr(), TagLength);
			continue;
		}
		if (::stricmp_utf8(TagName, "dumper") == 0) {
			if (TagLength > ID6_DUMPER_MAX)
				TagLength = ID6_DUMPER_MAX;
			::CopyMemory(Header.dumper, TagValue.get_ptr(), TagLength);
			continue;
		}
#if 0
		if (::stricmp_utf8(TagName, "date") == 0) {
			continue;
		}
		if (::stricmp_utf8(TagName, "emulator") == 0) {
			continue;
		}
#endif
	}
#if 0
	pfc::string8 SongLength = p_info.info_get("spc_songlength");
	if (!SongLength.is_empty()) {
		if (IsText) {
			::lstrcpynA(Header.text.songlen, SongLength.get_ptr(), 3);
		} else {
			int num = atoi(SongLength.get_ptr()) / 1000;
			Header.bin.songlen = num;
		}
	}
	pfc::string8 FadeLength = p_info.info_get("spc_fadelength");
	if (!FadeLength.is_empty()) {
		if (IsText) {
			::lstrcpynA(Header.text.fadelen, FadeLength.get_ptr(), 5);
		} else {
			int num = atoi(FadeLength.get_ptr());
			Header.bin.fadelen[0] = num       & 0xff;
			Header.bin.fadelen[1] = (num>> 8) & 0xff;
			Header.bin.fadelen[2] = (num>>16) & 0xff;
		}
	}
#endif
	m_File->seek(0, p_abort);
	m_File->write(reinterpret_cast<char*>(&Header), sizeof(Header), p_abort);
}

void input_snesapu::WriteExID6Header(u8 id, u8 type, u16 data, abort_callback& p_abort)
{
	m_File->write_object_t(id, p_abort);
	m_File->write_object_t(type, p_abort);
	m_File->write_lendian_t(data, p_abort);
}
void input_snesapu::WriteExID6Data(u8 id, u16 data, abort_callback& p_abort)
{
	WriteExID6Header(id, XID6_TVAL, data, p_abort);
}
void input_snesapu::WriteExID6Ing(u8 id, u32 value, abort_callback& p_abort)
{
	WriteExID6Header(id, XID6_TINT, 4, p_abort);
	m_File->write_lendian_t(value, p_abort);
}
void input_snesapu::WriteExID6String(u8 id, const char* value, abort_callback& p_abort)
{
	unsigned char Padding[4] = { '\0', '\0', '\0', '\0' };

	size_t Length = strlen(value);
	if (Length > 255) Length = 255;

	WriteExID6Header(id, XID6_TSTR, Length + 1, p_abort);
	m_File->write_object(value, Length, p_abort);
	m_File->write_object(Padding, 4 - (Length & 3), p_abort);
}

void input_snesapu::WriteExID666(const file_info &p_info, abort_callback &p_abort)
{
	//I referred to foo_gep. Thanks kode54 !
	pfc::stringcvt::string_ansi_from_utf8 converter;
	t_filesize offset_tag_start;
	const char* value = 0;
	pfc::string8 formatted_value;	//for multi-value field
	t_uint32 xid6_total_length = 0;

	m_File->seek(OFS_XID6, p_abort);
	m_File->set_eof(p_abort);

	m_File->write_object(XID6_MAGIC, 4, p_abort);
	m_File->write_object_t(xid6_total_length, p_abort);

	offset_tag_start = OFS_XID6 + XID6_HEADER_LENGTH;

	value = p_info.meta_get("title", 0);
	if (value)
	{
		converter.convert(value);
		if (converter.length() > ID6_TEXT_MAX)
			WriteExID6String(XID6_SONG, converter, p_abort);
	}

	value = p_info.meta_get("album", 0);
	if (value)
	{
		converter.convert(value);
		if (converter.length() > ID6_TEXT_MAX)
			WriteExID6String(XID6_GAME, converter, p_abort);
	}

	//multi-value field
	p_info.meta_format("artist", formatted_value);
	if (!formatted_value.is_empty())
	{
		converter.convert(formatted_value);
		if (converter.length() > ID6_TEXT_MAX)
			WriteExID6String(XID6_ARTIST, converter, p_abort);
	}

	value = p_info.meta_get("dumper", 0);
	if (value)
	{
		converter.convert(value);
		if (converter.length() > ID6_DUMPER_MAX)
			WriteExID6String(XID6_DUMPER, converter, p_abort);
	}

	// Write original value
	if (m_XID6Date != 0xFFFFFFFF)
		WriteExID6Ing(XID6_DATE, m_XID6Date, p_abort);
	if (m_XID6Emulater != 0xFFFF)
		WriteExID6Data(XID6_EMU, m_XID6Emulater, p_abort);

	value = p_info.meta_get("comment", 0);
	if (value)
	{
		converter.convert(value);
		if (converter.length() > ID6_TEXT_MAX)
			WriteExID6String(XID6_CMNTS, converter, p_abort);
}

	value = p_info.meta_get("OST", 0);
	if (value)
	{
		converter.convert(value);
		WriteExID6String(XID6_OST, converter, p_abort);
	}

	value = p_info.meta_get("discnumber", 0);
	if (value)
	{
		char* end;
		unsigned disc = strtoul(value, &end, 10);
		if (*end == '\0' && disc > 0 && disc <= 9)
			WriteExID6Data(XID6_DISC, disc, p_abort);
	}

	value = p_info.meta_get("tracknumber", 0);
	if (value)
	{
		char* end;
		unsigned track = strtoul(value, &end, 10);
		if (track == 0 && *value == '0' || track > 0 && track < 100) {
			track = track * 0x100;
			if (*end > 0x20 && *end < 0x7f)
				track += *end;
			WriteExID6Data(XID6_TRACK, track, p_abort);
		}
	}

	//Publisher's name
	value = p_info.meta_get("copyright", 0);
	if (value)
	{
		converter.convert(value);
		WriteExID6String(XID6_PUB, converter, p_abort);
	}

	//Copyright year
	value = p_info.meta_get("date", 0);
	if (value)
	{
		char* end;
		unsigned copyright_year = strtoul(value, &end, 10);
		if (*end == '\0' && copyright_year > 0 && copyright_year < 65536)
			WriteExID6Data(XID6_COPY, copyright_year, p_abort);
	}

	/*value = p_info.info_get("spc_songlength");
	if (value)
	{
		char * end;
		unsigned length = strtoul(value, &end, 10);
		if (!*end && length > 0)
			WriteExID6Ing(XID6_INTRO, length * 64, p_abort);
	}
	*/

	// Write original value
	if (m_XID6TimeIntro != 0xFFFFFFFF)
		WriteExID6Ing(XID6_INTRO, m_XID6TimeIntro, p_abort);

	if (m_XID6TimeLoop != 0xFFFFFFFF)
		WriteExID6Ing(XID6_LOOP, m_XID6TimeLoop, p_abort);

	if (m_XID6TimeEnd != 0x7FFFFFFF)	//TimeEnd can contain a negative value. 0x7FFFFFFF is the maximum positive value for a 32-bit signed binary.
		WriteExID6Ing(XID6_END, m_XID6TimeEnd, p_abort);

	/*value = p_info.info_get("spc_fadelength");
	if (value)
	{
		char * end;
		unsigned fade = strtoul(value, &end, 10);
		if (!*end)
			WriteExID6Ing(XID6_FADE, fade * 64, p_abort);
	}
	*/

	if (m_XID6TimeFade != 0xFFFFFFFF)
		WriteExID6Ing(XID6_FADE, m_XID6TimeFade, p_abort);

	if (m_TagMask)
		WriteExID6Data(XID6_MUTE, m_TagMask, p_abort);

	if (m_XID6LoopX != 0xFFFF)
		WriteExID6Data(XID6_LOOPX, m_XID6LoopX, p_abort);

	if (m_XID6Amp != 0xFFFFFFFF)
		WriteExID6Ing(XID6_AMP, m_XID6Amp, p_abort);

	//Update XID6's Size
	t_filesize offset = m_File->get_position(p_abort);
	offset -= offset_tag_start;
	if (offset > 0xFFFFFFFF)
		offset = 0xFFFFFFFF;

	if (offset)
	{
		xid6_total_length = t_uint32(offset);
		m_File->seek(offset_tag_start - 4, p_abort);
		m_File->write_lendian_t(xid6_total_length, p_abort);
	}
	else
	{
		m_File->seek(OFS_XID6, p_abort);
		m_File->set_eof(p_abort);
	}
}

void input_snesapu::ResetTagValue()
{
	m_UseDefaultTime = true;
	m_TagSong.reset();
	m_TagGame.reset();
	m_TagArtist.reset();
	m_TagDumper.reset();
	m_TagComment.reset();
	m_TagOstDisc.reset();
	m_TagOstTrack.reset();
	m_TagPublisher.reset();
	m_TagCopylight.reset();
	m_TagEmulater.reset();
	m_TagDate.reset();
	m_TagTimeIntro = 0;
	m_TagTimeLoop = 0;
	m_TagTimeEnd = 0;
	m_TagLoopLimit = 1;
	m_TagTimeFade = 0;
	m_TagMask = 0;
	m_TagAmp = 0xFFFFFFFF;
	m_XID6Emulater = 0xFFFF;
	m_XID6Date = 0xFFFFFFFF;
	m_XID6TimeIntro = 0xFFFFFFFF;
	m_XID6TimeLoop = 0xFFFFFFFF;
	m_XID6TimeEnd = 0x7FFFFFFF; //TimeEnd can contain a negative value. 0x7FFFFFFF is the maximum positive value for a 32-bit signed binary.
	m_XID6LoopX = 0xFFFF;
	m_XID6TimeFade = 0xFFFFFFFF;
	m_XID6Amp = 0xFFFFFFFF;
	m_CurAmp = 0x10000;
	m_ExtraDataLength = 0;
}

void input_snesapu::retag(const file_info &p_info, abort_callback &p_abort)
{
#if !defined(WRITE_SUPPORT)
	throw exception_io_unsupported_format();
#else
	if (m_ExtraDataLength > 0)
		throw exception_io_unsupported_format();

	WriteID666(p_info, p_abort);
	WriteExID666(p_info, p_abort);

	// Reread info
	m_File->seek(0, p_abort);
	m_SpcFileLength	= static_cast<u32>(m_File->get_size(p_abort));
	m_SpcFile.set_size(m_SpcFileLength);
	m_File->read(m_SpcFile.get_ptr(), m_SpcFileLength, p_abort);

	ResetTagValue();
	ReadSpcInfo();
#endif
}

void input_snesapu::remove_tags(abort_callback &p_abort)
{
	SPCHEAD *SpcHead = reinterpret_cast<SPCHEAD*>(m_SpcFile.get_ptr());

	// Clear ID666 tags
	memset(SpcHead->song, 0, sizeof(SPCHEAD) - offsetof(SPCHEAD, song));

	// Truncate xid6 tag from file
	m_File->seek(0, p_abort);
	m_SpcFileLength = 66048;
	m_SpcFile.set_size(m_SpcFileLength);
	m_File->write(m_SpcFile.get_ptr(), m_SpcFileLength, p_abort);
}

GUID input_snesapu::g_get_guid()
{
	return { 0x463e68cd, 0xe975, 0x46f8,{ 0x8e, 0xd5, 0x4b, 0x8a, 0x15, 0xb3, 0x68, 0xb8 } };
}

const char * input_snesapu::g_get_name()
{
	return "SNESAPU input";
}

GUID input_snesapu::g_get_preferences_guid()
{
	return guid_snesapu_preference_page;
}

static input_singletrack_factory_t<input_snesapu> g_input_snesapu_factory;

static const char* const g_component_about =
"Uses improved SNESAPU.dll being developed by Sunburst for SPC decoding.\n"
"\n"
"Improved SNESAPU.dll\n"
"Copyright (C) 2003-2024 degrade-factory\n"
"\n"
"Original SNESAPU.dll\n"
"Copyright (C) 2001-2004 Alpha-II Productions\n"
#ifndef _WIN64
"\n"
"Memory DLL loading code\n"
"Copyright (c) 2004-2015 by Joachim Bauch\n"
#endif
;

// version info
DECLARE_COMPONENT_VERSION("SNESAPU input",
                          "0.90",
                          g_component_about
							  )
DECLARE_FILE_TYPE("SPC files", "*.spc");

						  // This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_snesapu.dll");