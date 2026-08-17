#pragma once

#include "snesapu/SNESAPU.h"
#include "../../spcplayer/spcplayer.h"
#include "spc_source_block.h"
#include <optional>

//This class is based on foo_input_vio2sf's TwoSFPlayer class.
//Thanks for the superb work, kode54.
class spcplayer_controller
{
public:
	spcplayer_controller();

	~spcplayer_controller();


	void	EmuAPU(void *pBuf, u32 len, b8 type);
	void	EmuAPU_with_telem(void *pBuf, u32 len, b8 type, SpcBlockTelem& out_telem);
	void	EmuAPU_with_sources(void *pBuf, u32 len, b8 type, SpcBlockTelem& out_telem, spc_source_block& out_sources);
	void	LoadSPCFile(void *pSPC, u32 len);
	void	SetAPULength(u32 song, u32 fade);
	void	SetAPUOpt(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts);
	void	SetDSPAmp(u32 level);
	u32		SetScript700(void *pSource, u32 len);
	void    SetVoiceMute(u32 mute);
	void    SetTelemetryEnabled(bool enabled);
	void    SetSourceEnabled(bool enabled);


	void end_decode_initialization();

	void shutdown();

	void restart();

	// Returns the last fully read child telemetry block.
	const SpcBlockTelem& get_last_telem() const { return m_last_telem; }
	const spc_source_block& get_last_source_block() const { return m_last_source; }

private:
	bool startup();

	bool process_create();
	void process_terminate();
	bool process_running();
	void process_read_bytes(void * buffer, uint32_t size);
	uint32_t process_read_bytes_pass(void * buffer, uint32_t size);
	bool process_read_telem(SpcBlockTelem& out_telem);
	bool process_read_source(uint32_t expected_frames, spc_source_block& out_source);
	bool read_next_stream_block();
	uint32_t next_stream_block_frames() const;
	void reset_stream_staging();

	int get_silent_value();

	pfc::array_t<t_uint8> m_spcp_data;
	t_size				  m_spcp_size;
	pfc::array_t<t_uint8> m_spc_data;
	t_size				  m_spc_size;
	pfc::array_t<t_uint8> m_script700_data;
	t_size				  m_script700_size;


	bool         initialized;
	unsigned int iInitialized;
	bool         bTerminating;
	HANDLE       hProcess;
	HANDLE       hThread;
	HANDLE       hReadEvent;
	HANDLE       hChildStd_IN_Rd;
	HANDLE       hChildStd_IN_Wr;
	HANDLE       hChildStd_OUT_Rd;
	HANDLE       hChildStd_OUT_Wr;

	u32 m_song;
	u32 m_fade;
	std::optional<u32> m_mix;
	std::optional<u32> m_chn;
	std::optional<u32> m_bits;
	std::optional<u32> m_rate;
	std::optional<u32> m_inter;
	std::optional<u32> m_dspopts;
	std::optional<u32> m_mute;
	std::optional<u32> m_level;
	bool m_set_song_length;

	bool          m_telem_enabled;
	bool          m_source_enabled;
	SpcBlockTelem m_last_telem;
	spc_source_block m_last_source;

	// Child framing is independent of foobar's requested decode size. Always
	// read one complete [PCM][TLEM][SRCE] packet here, then splice slices into
	// arbitrary parent requests without ever crossing metadata as PCM.
	pfc::array_t<t_uint8> m_stream_pcm;
	uint32_t m_stream_frames;
	uint32_t m_stream_offset;
	uint64_t m_stream_song_frames_read;
	SpcBlockTelem m_stream_telem;
	spc_source_block m_stream_source;
};
