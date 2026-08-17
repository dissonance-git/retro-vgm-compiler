#include "stdafx.h"
#include "spcplayer_controller.h"
#include "../../spcplayer/spcplayer.h"

#include <algorithm>
#include <vector>
#include <cstring>


static void set_le32(uint8_t * p_data, uint32_t value)
{
	for (uint32_t i = 0; i < 4; i++)
	{
		p_data[i] = (uint8_t)(value >> (i * 8));
	}
}

static bool create_pipe_name(pfc::string_base & out)
{
	GUID guid;
	if (FAILED(CoCreateGuid(&guid))) return false;

	out = "\\\\.\\pipe\\";
	out += pfc::print_guid(guid);

	return true;
}

bool spcplayer_controller::process_create()
{
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(saAttr);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!iInitialized)
	{
		if (FAILED(CoInitialize(NULL))) return false;
	}
	iInitialized++;

	hReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	pfc::string8 pipe_name_in, pipe_name_out;
	if (!create_pipe_name(pipe_name_in) || !create_pipe_name(pipe_name_out))
	{
		process_terminate();
		return false;
	}

	pfc::stringcvt::string_os_from_utf8 pipe_name_in_os(pipe_name_in), pipe_name_out_os(pipe_name_out);

	HANDLE hPipe = CreateNamedPipe(pipe_name_in_os, PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		process_terminate();
		return false;
	}
	hChildStd_IN_Rd = CreateFile(pipe_name_in_os, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);
	DuplicateHandle(GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd_IN_Wr, 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hPipe);

	hPipe = CreateNamedPipe(pipe_name_out_os, PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 65536, 65536, 0, &saAttr);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		process_terminate();
		return false;
	}
	hChildStd_OUT_Wr = CreateFile(pipe_name_out_os, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);
	DuplicateHandle(GetCurrentProcess(), hPipe, GetCurrentProcess(), &hChildStd_OUT_Rd, 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hPipe);

	std::string szCmdLine = "\"";
	szCmdLine += core_api::get_my_full_path();
	size_t slash = szCmdLine.find_last_of('\\');
	if (slash != std::string::npos)
		szCmdLine.erase(szCmdLine.begin() + slash + 1, szCmdLine.end());
	szCmdLine += "spcplayer.exe\"";

	if (m_set_song_length)
	{
		szCmdLine += " --time ";
		szCmdLine += pfc::format_uint(m_song);
		szCmdLine += " --fade ";
		szCmdLine += pfc::format_uint(m_fade);
		szCmdLine += " --silence ";
	}
	
	if (m_rate)
	{
		szCmdLine += " --rate ";
		szCmdLine += pfc::format_uint(m_rate.value());
	}
	if (m_bits)
	{
		szCmdLine += " --bits ";
		szCmdLine += pfc::format_uint(m_bits.value());
	}
	if (m_chn)
	{
		szCmdLine += " --numchn ";
		szCmdLine += pfc::format_uint(m_chn.value());
	}
	if (m_dspopts)
	{
		szCmdLine += " --dspopts ";
		szCmdLine += pfc::format_uint(m_dspopts.value());
	}
	if (m_inter)
	{
		szCmdLine += " --inter ";
		szCmdLine += pfc::format_uint(m_inter.value());
	}
	if (m_level)
	{
		szCmdLine += " --amp ";
		szCmdLine += pfc::format_uint(m_level.value());
	}
	if (m_mute)
	{
		szCmdLine += " --mute ";
		szCmdLine += pfc::format_uint(m_mute.value());
	}

	if (m_source_enabled)
	{
		szCmdLine += " --sources";
	}
	else if (m_telem_enabled)
	{
		szCmdLine += " --telemetry";
	}

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo = { 0 };

	siStartInfo.cb = sizeof(siStartInfo);
	siStartInfo.hStdInput = hChildStd_IN_Rd;
	siStartInfo.hStdOutput = hChildStd_OUT_Wr;
	siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	siStartInfo.wShowWindow = SW_HIDE;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	if (!CreateProcess(NULL, (LPTSTR)(LPCTSTR)pfc::stringcvt::string_os_from_utf8(szCmdLine.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo))
	{
		process_terminate();
		return false;
	}

	CloseHandle(hChildStd_OUT_Wr);
	CloseHandle(hChildStd_IN_Rd);
	hChildStd_OUT_Wr = NULL;
	hChildStd_IN_Rd = NULL;

	hProcess = piProcInfo.hProcess;
	hThread = piProcInfo.hThread;

#ifdef NDEBUG
	SetPriorityClass(hProcess, GetPriorityClass(GetCurrentProcess()));
	SetThreadPriority(hThread, GetThreadPriority(GetCurrentThread()));
#endif

	DWORD uBytesWrittenTotal = 0;

	while (uBytesWrittenTotal < m_spcp_size)
	{
		DWORD uOffset = uBytesWrittenTotal;
		DWORD uBytesToWrite = m_spcp_size - uOffset;
		DWORD uBytesWritten = 0;
		if (uBytesToWrite > 1024)
			uBytesToWrite = 1024;
		if (!WriteFile(hChildStd_IN_Wr, m_spcp_data.get_ptr() + uOffset, uBytesToWrite, &uBytesWritten, NULL) || uBytesWritten < uBytesToWrite)
			return false;
		uBytesWrittenTotal += uBytesWritten;
	}

	CloseHandle(hChildStd_IN_Wr);
	hChildStd_IN_Wr = NULL;
	reset_stream_staging();

	return true;
}

void spcplayer_controller::process_terminate()
{
	if (bTerminating) return;

	bTerminating = true;

	if (hProcess)
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hThread);
		CloseHandle(hProcess);
	}
	if (hChildStd_IN_Rd) CloseHandle(hChildStd_IN_Rd);
	if (hChildStd_IN_Wr) CloseHandle(hChildStd_IN_Wr);
	if (hChildStd_OUT_Rd) CloseHandle(hChildStd_OUT_Rd);
	if (hChildStd_OUT_Wr) CloseHandle(hChildStd_OUT_Wr);
	if (hReadEvent) CloseHandle(hReadEvent);
	if (iInitialized && --iInitialized == 0) CoUninitialize();
	bTerminating = false;
	hProcess = NULL;
	hThread = NULL;
	hReadEvent = NULL;
	hChildStd_IN_Rd = NULL;
	hChildStd_IN_Wr = NULL;
	hChildStd_OUT_Rd = NULL;
	hChildStd_OUT_Wr = NULL;
	reset_stream_staging();
}

bool spcplayer_controller::process_running()
{
	if (hProcess && WaitForSingleObject(hProcess, 0) == WAIT_TIMEOUT) return true;
	return false;
}

uint32_t spcplayer_controller::process_read_bytes_pass(void * out, uint32_t size)
{
	OVERLAPPED ol = {};
	ol.hEvent = hReadEvent;
	ResetEvent(hReadEvent);
	DWORD bytesDone;
	SetLastError(NO_ERROR);
	if (ReadFile(hChildStd_OUT_Rd, out, size, &bytesDone, &ol)) return bytesDone;
	if (::GetLastError() != ERROR_IO_PENDING) return 0;

	const HANDLE handles[1] = { hReadEvent };
	SetLastError(NO_ERROR);
	DWORD state;
	state = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);

	if (state == WAIT_OBJECT_0 && GetOverlappedResult(hChildStd_OUT_Rd, &ol, &bytesDone, TRUE)) return bytesDone;

#if _WIN32_WINNT >= 0x600
	CancelIoEx(hChildStd_OUT_Rd, &ol);
#else
	CancelIo(hChildStd_OUT_Rd);
#endif

	return 0;
}

int spcplayer_controller::get_silent_value()
{
	if (m_bits && 8 == m_bits.value())
		return 0x80;
	return 0xFF;
}

void spcplayer_controller::process_read_bytes(void * out, uint32_t size)
{
	if (process_running() && size)
	{
		uint8_t * ptr = (uint8_t *)out;
		uint32_t done = 0;
		while (done < size)
		{
			uint32_t delta = process_read_bytes_pass(ptr + done, size - done);
			if (delta == 0)
			{
				memset(out, get_silent_value(), size);
				break;
			}
			done += delta;
		}
	}
	else memset(out, get_silent_value(), size);
}

bool spcplayer_controller::process_read_telem(SpcBlockTelem& out_telem)
{
	if (!process_running()) return false;

	uint8_t raw[sizeof(SpcBlockTelem)];
	uint32_t done = 0;
	while (done < sizeof(raw))
	{
		uint32_t delta = process_read_bytes_pass(raw + done, sizeof(raw) - done);
		if (delta == 0) return false;
		done += delta;
	}

	std::memcpy(&out_telem, raw, sizeof(SpcBlockTelem));
	if (out_telem.magic != SPCP_TELEM_MAGIC)
	{
		std::memset(&out_telem, 0, sizeof(SpcBlockTelem));
		return false;
	}
	return true;
}

bool spcplayer_controller::process_read_source(uint32_t expected_frames, spc_source_block& out_source)
{
	if (!process_running()) return false;

	SpcSourceBlockHeader header{};
	uint8_t* header_ptr = reinterpret_cast<uint8_t*>(&header);
	uint32_t done = 0;
	while (done < sizeof(header))
	{
		uint32_t delta = process_read_bytes_pass(header_ptr + done, static_cast<uint32_t>(sizeof(header)) - done);
		if (delta == 0) return false;
		done += delta;
	}

	if (header.magic != SPCP_SOURCE_MAGIC ||
		header.version != SPCP_SOURCE_VERSION ||
		header.header_size != sizeof(SpcSourceBlockHeader) ||
		header.block_samples != expected_frames ||
		header.plane_count != SPCP_SOURCE_PLANES ||
		header.sample_format != SPCP_SOURCE_FORMAT_F32 ||
		header.audio_lanes != SPCP_SOURCE_AUDIO_LANES ||
		header.reserved16 != 0 || header.reserved32 != 0)
	{
		return false;
	}

	out_source.resize(expected_frames);
	uint8_t* dst = reinterpret_cast<uint8_t*>(out_source.planar.data());
	const uint32_t bytes = static_cast<uint32_t>(out_source.planar.size() * sizeof(float));
	done = 0;
	while (done < bytes)
	{
		uint32_t delta = process_read_bytes_pass(dst + done, bytes - done);
		if (delta == 0)
		{
			out_source.clear();
			return false;
		}
		done += delta;
	}
	return true;
}

void spcplayer_controller::reset_stream_staging()
{
	m_stream_pcm.force_reset();
	m_stream_frames = 0;
	m_stream_offset = 0;
	m_stream_song_frames_read = 0;
	std::memset(&m_stream_telem, 0, sizeof(m_stream_telem));
	m_stream_source.clear();
	m_last_source.clear();
}

uint32_t spcplayer_controller::next_stream_block_frames() const
{
	const uint32_t rate = m_rate.value_or(32000);
	const uint32_t normal = spcp_stream_block_samples(rate, m_source_enabled);
	if (!m_set_song_length) return normal;

	const uint64_t total =
		(static_cast<uint64_t>(m_song) + static_cast<uint64_t>(m_fade)) * rate / 64000u;
	if (m_stream_song_frames_read >= total) return normal; // child is in --silence mode
	const uint64_t remaining = total - m_stream_song_frames_read;
	return static_cast<uint32_t>(std::min<uint64_t>(normal, remaining));
}

bool spcplayer_controller::read_next_stream_block()
{
	if (!process_running() || !m_chn || !m_bits) return false;

	const uint32_t frames = next_stream_block_frames();
	if (frames == 0) return false;
	const uint32_t bpf = m_chn.value() * (m_bits.value() / 8);
	const uint32_t pcm_bytes = frames * bpf;
	m_stream_pcm.set_size(pcm_bytes);

	uint32_t done = 0;
	while (done < pcm_bytes)
	{
		uint32_t delta = process_read_bytes_pass(m_stream_pcm.get_ptr() + done, pcm_bytes - done);
		if (delta == 0) return false;
		done += delta;
	}

	std::memset(&m_stream_telem, 0, sizeof(m_stream_telem));
	if (m_telem_enabled)
	{
		if (!process_read_telem(m_stream_telem) || m_stream_telem.block_samples != frames)
			return false;
		m_last_telem = m_stream_telem;
	}

	m_stream_source.clear();
	if (m_source_enabled)
	{
		if (!process_read_source(frames, m_stream_source))
			return false;
		m_last_source = m_stream_source;
	}

	m_stream_frames = frames;
	m_stream_offset = 0;
	if (m_set_song_length)
	{
		const uint64_t rate = m_rate.value_or(32000);
		const uint64_t total =
			(static_cast<uint64_t>(m_song) + static_cast<uint64_t>(m_fade)) * rate / 64000u;
		if (m_stream_song_frames_read < total)
			m_stream_song_frames_read = std::min<uint64_t>(total, m_stream_song_frames_read + frames);
	}
	return true;
}

spcplayer_controller::spcplayer_controller() : 
	m_set_song_length(false), m_song(0), m_fade(0), 
	m_spcp_size(0), m_spc_size(0), m_script700_size(0),
	m_telem_enabled(false), m_source_enabled(false),
	m_stream_frames(0), m_stream_offset(0), m_stream_song_frames_read(0)
{
	initialized = false;
	iInitialized = 0;
	bTerminating = false;
	hProcess = NULL;
	hThread = NULL;
	hReadEvent = NULL;
	hChildStd_IN_Rd = NULL;
	hChildStd_IN_Wr = NULL;
	hChildStd_OUT_Rd = NULL;
	hChildStd_OUT_Wr = NULL;
	std::memset(&m_last_telem, 0, sizeof(m_last_telem));
	std::memset(&m_stream_telem, 0, sizeof(m_stream_telem));
}

spcplayer_controller::~spcplayer_controller()
{
	process_terminate();
}

void spcplayer_controller::shutdown()
{
	process_terminate();
	initialized = false;
}

void spcplayer_controller::restart()
{
	shutdown();
	startup();
}

bool spcplayer_controller::startup()
{
    if (initialized) return true;

	if (!process_create())
		return false;

	initialized = true;
    
	return true;
}

void spcplayer_controller::end_decode_initialization()
{
	m_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size;
	m_spcp_data.set_size(m_spcp_size);

	const char *p_signature = SPCP_HEADER_SIGNATURE;
	size_t signature_size = strlen(p_signature);
	for (size_t i = 0; i < signature_size; i++)
	{
		m_spcp_data[i] = p_signature[i];
	}

	set_le32(m_spcp_data.get_ptr() + 4, SPCP_HEADER_VERSION);
	set_le32(m_spcp_data.get_ptr() + 8, SPCP_HEADER_SIZE);
	set_le32(m_spcp_data.get_ptr() + 12, m_spc_size);
	set_le32(m_spcp_data.get_ptr() + 16, m_script700_size);
	memcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE, m_spc_data.get_ptr(), m_spc_size);
	if (m_script700_size)
		memcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size, m_script700_data.get_ptr(), m_script700_size);

	m_spc_data.force_reset();
	m_script700_data.force_reset();

	startup();
}

void spcplayer_controller::EmuAPU(void* pBuf, u32 len, b8 type)
{
	if (m_source_enabled)
	{
		SpcBlockTelem telem{};
		spc_source_block sources;
		EmuAPU_with_sources(pBuf, len, type, telem, sources);
		return;
	}
	if (m_telem_enabled)
	{
		SpcBlockTelem telem{};
		EmuAPU_with_telem(pBuf, len, type, telem);
		return;
	}
	if (!process_running()) throw exception_io_data();
	process_read_bytes(pBuf, len*m_chn.value()*(m_bits.value()/8));
}

void spcplayer_controller::EmuAPU_with_telem(void* pBuf, u32 len, b8 type, SpcBlockTelem& out_telem)
{
	if (!process_running()) throw exception_io_data();
	if (m_source_enabled)
	{
		spc_source_block ignored;
		EmuAPU_with_sources(pBuf, len, type, out_telem, ignored);
		return;
	}

	const uint32_t bpf = m_chn.value() * (m_bits.value() / 8);
	uint8_t* dst = static_cast<uint8_t*>(pBuf);
	uint32_t remaining = len;

	while (remaining > 0)
	{
		if (m_stream_offset >= m_stream_frames && !read_next_stream_block())
			throw exception_io_data();
		const uint32_t available = m_stream_frames - m_stream_offset;
		const uint32_t take = std::min(remaining, available);
		std::memcpy(
			dst,
			m_stream_pcm.get_ptr() + static_cast<size_t>(m_stream_offset) * bpf,
			static_cast<size_t>(take) * bpf);
		dst += static_cast<size_t>(take) * bpf;
		m_stream_offset += take;
		remaining -= take;
		out_telem = m_stream_telem;
	}
	m_last_telem = out_telem;
}

void spcplayer_controller::EmuAPU_with_sources(
	void* pBuf,
	u32 len,
	b8 type,
	SpcBlockTelem& out_telem,
	spc_source_block& out_sources)
{
	if (!process_running() || !m_source_enabled) throw exception_io_data();

	const uint32_t bpf = m_chn.value() * (m_bits.value() / 8);
	uint8_t* dst = static_cast<uint8_t*>(pBuf);
	uint32_t remaining = len;
	uint32_t dst_frame = 0;
	out_sources.resize(len);

	while (remaining > 0)
	{
		if (m_stream_offset >= m_stream_frames && !read_next_stream_block())
			throw exception_io_data();
		const uint32_t available = m_stream_frames - m_stream_offset;
		const uint32_t take = std::min(remaining, available);

		std::memcpy(
			dst,
			m_stream_pcm.get_ptr() + static_cast<size_t>(m_stream_offset) * bpf,
			static_cast<size_t>(take) * bpf);
		out_sources.copy_slice_from(m_stream_source, m_stream_offset, dst_frame, take);

		dst += static_cast<size_t>(take) * bpf;
		m_stream_offset += take;
		dst_frame += take;
		remaining -= take;
		out_telem = m_stream_telem;
	}

	m_last_telem = out_telem;
	m_last_source = out_sources;
}

void spcplayer_controller::LoadSPCFile(void* pSPC, u32 len)
{
	m_spc_data.set_size(len);
	memcpy(m_spc_data.get_ptr(), pSPC, len);
	m_spc_size = len;
}

void spcplayer_controller::SetAPULength(u32 song, u32 fade)
{
	m_song = song;
	m_fade = fade;
	m_set_song_length = true;
}

void spcplayer_controller::SetAPUOpt(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts)
{
	m_mix = mix;
	m_chn = chn;
	m_bits = bits;
	m_rate = rate;
	m_inter = inter;
	m_dspopts = opts;
}

void spcplayer_controller::SetDSPAmp(u32 level)
{
	m_level = level;
}

u32 spcplayer_controller::SetScript700(void* pSource, u32 len)
{
	m_script700_data.set_size(len);
	memcpy(m_script700_data.get_ptr(), pSource, len);
	m_script700_size = len;
	return 1;
}

void spcplayer_controller::SetVoiceMute(u32 mute)
{
	m_mute = mute;
}

void spcplayer_controller::SetTelemetryEnabled(bool enabled)
{
	m_telem_enabled = enabled || m_source_enabled;
}

void spcplayer_controller::SetSourceEnabled(bool enabled)
{
	m_source_enabled = enabled;
	if (enabled) m_telem_enabled = true;
	reset_stream_staging();
}
