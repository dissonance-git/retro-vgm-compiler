#include "stdafx.h"
#if !defined(__INPUT_SNESAPU_HPP__)
#define __INPUT_SNESAPU_HPP__

/**
 * foobar2000 wrapper for SNESAPU.dll
 *
 *     Licence :
 *        GNU General Public License
 *
 */

//#include "foobar2000.h"
#include "snesapu/Types.h"
#include "snesapu/SNESAPU.h"
#ifdef _WIN64
#include "spcplayer_controller.h"
#include "role_classifier_snes.h"
#include "semantic_mixer_71.h"
#include "semantic_stereo_enhancer.h"
#include "spc_adapter.h"
#include "../../../inference/chip_hint_bus.h"
#include "../../spcplayer/spcplayer.h"
#else
#ifndef _DEBUG
#include "../../MemoryModule/MemoryModule.h"
#endif
#endif

class input_snesapu : public input_stubs
{
  private:
#pragma pack(push, 1)
	struct SPCTEXT {
		s8	date[11];							// date dumped
		s8	songlen[3];							// song length (seconds before fading)
		s8	fadelen[5];							// fade length (milliseconds)
		s8	artist[32];							// artist
		u8	chandis;							// channels disabled
		u8	emulator;							// emulator used for dump
		u8	__r2[45];
	};
	struct SPCBIN {
		u8	date;
		u8	month;
		u16	year;
		s8	__r2[7];
		u16	songlen;
		s8	__r3;
		u8	fadelen[3];
		s8	__r4;
		s8	artist[32];
		s8	__r5;
		u8	emulator;
		s8	__r6[46];
	};
	struct SPCHEAD {
		s8	tag[33];							// file tag
		s8	term[2];							// tag terminator
		u8	tagkind;							// tag kind
		s8	ver;								// version #/100
		u8	pc[2];								// spc registers
		u8	a;
		u8	x;
		u8	y;
		u8	psw;
		u8	sp;
		u16	__r1;
		s8	song[32];							// song title
		s8	game[32];							// game title
		s8	dumper[16];							// name of dumper
		s8	comment[32];						// comments
		union {
			SPCBIN bin;
			SPCTEXT text;
		};
	};
#pragma pack(pop)

	// instance
#ifdef _WIN64
	spcplayer_controller	m_Apu;
#else
	SAPUFunc				m_Apu;				// SNESAPU instance
#endif
#if defined(_WIN64) || defined(_DEBUG)
	HINSTANCE				m_DllInst;			// SNESAPU dll handle
#else
	HMEMORYMODULE					m_DllInst;
	static pfc::array_t<t_uint8>	m_DllFileBuffer;
	static t_size					m_DllFileLength;
#endif
	service_ptr_t<file>		m_File;
	pfc::array_t<t_uint8>	m_DecodeBuffer;

#ifdef _WIN64
	// Semantic 7.1 enhancement
	bool						m_Sem71Enabled;		// 7.1 mode active
	SpcBlockTelem				m_Telem;			// current voice telemetry block
	SnesRoleClassifier			m_Classifier;		// per-voice role inference
	SpcAdapter					m_Adapter;
	SemanticStereoEnhancer		m_Enhancer;
	pfc::array_t<t_uint8>		m_Buf71;			// 7.1 interleaved output buffer
#endif

	// file & path
	u32						m_SpcFileLength;
	pfc::array_t<t_uint8>	m_SpcFile;			// in-memory copy of SPC file
	pfc::string8			m_SpcPath;			// SPC file path

	// reporting sample format configuration
	bool                    m_DynInfo;
	u32                     m_LastDecoded;

	// option
	u32						m_CnfSampleRate;
	u32						m_CnfBPS;
	u32						m_CnfChannels;
	u32						m_CnfMixing;
	u32						m_CnfInterpolation;
	u32						m_CnfOptions;
	bool					m_CnfFastSeek;
	bool					m_CnfForceLength;
	bool					m_CnfUseSpcLength;
	bool					m_CnfIgnoreScript700;
	bool					m_CnfIgnoreXID6Amplification;

	// tag
	pfc::string8			m_TagSong;
	pfc::string8			m_TagGame;
	pfc::string8			m_TagArtist;
	pfc::string8			m_TagDumper;
	pfc::string8			m_TagComment;
	pfc::string8			m_TagOstName;
	pfc::string8			m_TagOstDisc;
	pfc::string8			m_TagOstTrack;
	pfc::string8			m_TagPublisher;
	pfc::string8			m_TagCopylight;
	pfc::string8			m_TagEmulater;
	pfc::string8			m_TagDate;
	u32						m_TagTimeIntro;
	u32						m_TagTimeLoop;
	s32						m_TagTimeEnd;
	u32						m_TagLoopLimit;
	u32						m_TagTimeFade;
	u16						m_TagMask;
	u32						m_TagAmp;

	u16						m_XID6Emulater;
	u32						m_XID6Date;
	u32						m_XID6TimeIntro;
	u32						m_XID6TimeLoop;
	u32						m_XID6TimeEnd;	//type is u32 because I want to store original value.
	u16						m_XID6LoopX;
	u32						m_XID6TimeFade;
	u32						m_XID6Amp;
	u32						m_CurAmp;
	u32						m_ExtraDataLength;

	// play
	u32						m_SongLength;		// play time [ms]
	u32						m_FadeLength;		// fade time [ms]
	u32						m_PlayedTime;		// played time [ms]
	u32						m_SilentTime;		// silent detection time [ms]
	bool					m_UseDefaultTime;

	void ReadSpcInfo();
	void ReadID666();
	void ReadExID666();
	void WriteID666(const file_info &p_info, abort_callback &p_abort);
	void WriteExID666(const file_info &p_info, abort_callback &p_abort);
	bool LoadDll();
	void FreeDll();

	void WriteExID6Header(u8 id, u8 type, u16 data, abort_callback& p_abort);
	void WriteExID6Data(u8 id, u16 data, abort_callback& p_abort);
	void WriteExID6Ing(u8 id, u32 value, abort_callback& p_abort);
	void WriteExID6String(u8 id, const char* value, abort_callback& p_abort);
	void ResetTagValue();

	enum SNESAPU_CORE {
		CORE_ALPHA2,
		CORE_41568K,
		CORE_SSDLabo
	};
	SNESAPU_CORE GetKind();

  public:
	input_snesapu()
		:  m_DllInst(NULL)
		,  m_Adapter(m_Classifier)
	{
		ResetTagValue();
	}
	~input_snesapu();

	static bool g_is_our_path(const char *p_path, const char *p_extension) {
		return (::stricmp_utf8(p_extension,"spc") == 0);
	}

	void get_info(file_info &p_info, abort_callback &p_abort);
	void open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback &p_abort);
	void decode_initialize(unsigned int p_flags, abort_callback &p_abort);
	bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort);
	void decode_seek(double p_seconds, abort_callback &p_abort);
	bool decode_can_seek();

	t_filestats get_file_stats(abort_callback & p_abort) {return m_File->get_stats(p_abort);}
#if FOOBAR2000_SDK_VERSION >= 20220810
	t_filestats2 get_stats2(uint32_t f, abort_callback& p_abort) {return m_File->get_stats2_(f, p_abort);}
#endif
	void decode_on_idle(abort_callback & p_abort) {m_File->on_idle(p_abort);}
	bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta);
	bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta) {return false;}
	static bool g_is_our_content_type(const char *p_content_type) {return false;}
	void retag(const file_info &p_info, abort_callback &p_abort);
	void remove_tags(abort_callback &p_abort);

	static GUID g_get_guid();
	static const char * g_get_name();
	static GUID g_get_preferences_guid();
};

#endif /* __INPUT_SNESAPU___ */