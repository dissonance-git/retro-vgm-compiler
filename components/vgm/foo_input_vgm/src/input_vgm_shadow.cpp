#include "input_vgm.h"

#include <emu/cores/sn764intf.h>
#include <limits>

void input_vgm::configure_enhancement_shadow()
{
	if (m_shadow_configured)
		return;
	m_shadow_configured = true;
	m_shadow_replay_sample = 0;

#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	m_genesis_state.set_event_tap(&input_vgm::source_event_tap, this);

	for (const PLR_DEV_INFO& device : m_pdi_list)
	{
		if (device.parentIdx != (uint32_t)-1 || device.devCfg == nullptr || device.instance >= 2)
			continue;

		const size_t instance = static_cast<size_t>(device.instance);

		if (device.type == DEVID_SN76496)
		{
			const SN76496_CFG* source_cfg = reinterpret_cast<const SN76496_CFG*>(device.devCfg);
			gameaudio::vgm::sn76489_enhanced::config enhanced_cfg;
			enhanced_cfg.chip_clock_hz = source_cfg->_genCfg.clock;
			enhanced_cfg.sample_rate_hz = m_sample_rate;
			enhanced_cfg.white_noise_feedback = source_cfg->noiseTaps;
			enhanced_cfg.shift_register_width = source_cfg->shiftRegWidth;
			enhanced_cfg.clock_divider = source_cfg->clkDiv ? source_cfg->clkDiv : 8;
			enhanced_cfg.sega_style_psg = source_cfg->segaPSG != 0;
			enhanced_cfg.ncr_style_psg = source_cfg->ncrPSG != 0;
			enhanced_cfg.negate_output = source_cfg->negate != 0;
			enhanced_cfg.oversample = 4;

			m_enhanced_psg[instance].configure(enhanced_cfg);
			m_psg_present[instance] = true;

			// T6W28 split-chip behavior and NCR-style timing stay on libvgm's
			// reference renderer until dedicated enhanced models are validated.
			m_psg_config_supported[instance] =
				(source_cfg->_genCfg.flags == 0) && m_enhanced_psg[instance].supported();
			m_psg_shadow_valid[instance] = m_psg_config_supported[instance];
		}
		else if (device.type == DEVID_YM2612)
		{
			m_dac_present[instance] = true;
			m_dac_shadow_valid[instance] = true;
			m_enhanced_dac[instance].reset();
		}
		else if (device.type == DEVID_QSOUND && instance == 0)
		{
			// VGM command C4 addresses the single QSound instance used by the
			// format/player path. Source audio and native mix accounting are
			// evidence sidecars; libvgm stays the only audible renderer.
			m_qsound_present = true;
			m_qsound_shadow_valid = true;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
			m_qsound_audio_shadow_valid = true;
#else
			m_qsound_audio_shadow_valid = false;
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
			m_qsound_mix_shadow_valid = true;
#else
			m_qsound_mix_shadow_valid = false;
#endif
			m_qsound_state.reset();
		}
	}
#endif
}

#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
void input_vgm::qsound_source_callback(void* user_param, const VGM_QSOUND_SOURCE_FRAME* event)
{
	input_vgm* self = static_cast<input_vgm*>(user_param);
	if (self == nullptr || event == nullptr || !self->m_qsound_audio_capture_active)
		return;

	self->m_qsound_audio_capture.observe(
		event->chipID,
		event->sampleRate,
		event->nativeSample,
		event->source,
		static_cast<size_t>(event->sourceCount));
}
#endif

#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
void input_vgm::qsound_mix_callback(void* user_param, const VGM_QSOUND_MIX_FRAME* event)
{
	input_vgm* self = static_cast<input_vgm*>(user_param);
	if (self == nullptr || event == nullptr || !self->m_qsound_mix_capture_active)
		return;

	gameaudio::vgm::qsound_native_mix_frame frame;
	frame.native_sample = event->nativeSample;
	frame.accounting_valid = event->accountingValid != 0;
	frame.echo_input = event->echoInput;
	frame.echo_output = event->echoOutput;
	for (size_t ch = 0; ch < 2; ++ch)
	{
		frame.wet_post_delay[ch] = event->wetPostDelay[ch];
		frame.dry_post_delay[ch] = event->dryPostDelay[ch];
		frame.reference_output[ch] = event->output[ch];
	}

	self->m_qsound_mix_capture.observe(event->chipID, event->sampleRate, &frame);
}
#endif

#ifndef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
void input_vgm::invalidate_unobserved_dac_stream(const gameaudio::vgm::command_event& event) noexcept
{
	// VGM 0x90 configures dac_control. Its payload is:
	// stream id, chip type/id, destination command high, destination command low.
	// dac_control writes occur outside VGMPlayer's command parser, so until that
	// scheduler gets its own source observer we must not claim a valid enhanced
	// YM2612 DAC reconstruction for streams targeting register $2A.
	if (event.kind != gameaudio::vgm::command_event_kind::command || event.command != 0x90 ||
		event.payload == nullptr || event.payload_size < 4)
		return;

	const uint8_t chip_type = static_cast<uint8_t>(event.payload[1] & 0x7F);
	const size_t instance = static_cast<size_t>((event.payload[1] >> 7) & 0x01);
	const uint16_t destination = static_cast<uint16_t>(event.payload[2] << 8) | event.payload[3];
	if (chip_type == DEVID_YM2612 && (destination & 0x00FF) == 0x2A)
		m_dac_shadow_valid[instance] = false;
}
#endif

void input_vgm::source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept
{
	input_vgm* self = static_cast<input_vgm*>(user_param);
	if (self == nullptr || !self->m_shadow_configured)
		return;

	if (event.kind == gameaudio::vgm::command_event_kind::reset)
	{
		for (size_t instance = 0; instance < self->m_enhanced_psg.size(); ++instance)
		{
			self->m_enhanced_psg[instance].reset();
			self->m_psg_shadow_valid[instance] = self->m_psg_config_supported[instance];
			self->m_enhanced_dac[instance].reset();
			self->m_dac_shadow_valid[instance] = self->m_dac_present[instance];
		}
		self->m_qsound_state.reset();
		self->m_qsound_shadow_valid = self->m_qsound_present;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
		self->m_qsound_audio_shadow_valid = self->m_qsound_present;
#else
		self->m_qsound_audio_shadow_valid = false;
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
		self->m_qsound_mix_shadow_valid = self->m_qsound_present;
#else
		self->m_qsound_mix_shadow_valid = false;
#endif
		self->m_shadow_replay_sample = 0;
		return;
	}

#ifndef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
	self->invalidate_unobserved_dac_stream(event);
#endif

	if (self->m_vgm_player == nullptr)
		return;

	const uint_fast64_t absolute_sample =
		static_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

	if (self->m_source_capture_active)
	{
		self->m_psg_capture.observe(event, absolute_sample);
		self->m_dac_capture.observe(event, absolute_sample);
		if (self->m_qsound_present && self->m_qsound_shadow_valid)
			self->m_qsound_capture.observe(event, absolute_sample);
		return;
	}

	self->advance_shadow_to(absolute_sample);
	self->apply_source_event_outside_render(event);
}

void input_vgm::advance_shadow_to(uint_fast64_t absolute_sample) noexcept
{
	if (absolute_sample <= m_shadow_replay_sample)
		return;

	uint_fast64_t remaining = absolute_sample - m_shadow_replay_sample;
	const uint_fast64_t max_chunk = static_cast<uint_fast64_t>(std::numeric_limits<size_t>::max());

	while (remaining != 0)
	{
		const size_t chunk = static_cast<size_t>(remaining > max_chunk ? max_chunk : remaining);
		for (size_t instance = 0; instance < m_enhanced_psg.size(); ++instance)
		{
			if (m_psg_present[instance] && m_psg_shadow_valid[instance])
				m_enhanced_psg[instance].advance(chunk);
		}
		remaining -= chunk;
	}

	m_shadow_replay_sample = absolute_sample;
}

void input_vgm::apply_source_event_outside_render(const gameaudio::vgm::command_event& event) noexcept
{
	if (event.kind == gameaudio::vgm::command_event_kind::command &&
		event.payload != nullptr && event.payload_size >= 1)
	{
		size_t psg_instance = 0;
		bool psg_write = true;
		bool stereo_mask = false;
		switch (event.command)
		{
		case 0x50:
			psg_instance = 0;
			break;
		case 0x30:
			psg_instance = 1;
			break;
		case 0x4F:
			psg_instance = 0;
			stereo_mask = true;
			break;
		default:
			psg_write = false;
			break;
		}

		if (psg_write && m_psg_present[psg_instance] && m_psg_shadow_valid[psg_instance])
		{
			if (stereo_mask)
				m_enhanced_psg[psg_instance].write_stereo_mask(event.payload[0]);
			else
				m_enhanced_psg[psg_instance].write(event.payload[0]);
		}
	}

	if (m_qsound_present && m_qsound_shadow_valid &&
		event.kind == gameaudio::vgm::command_event_kind::command && event.command == 0xC4 &&
		event.payload != nullptr && event.payload_size >= 3)
	{
		const uint16_t value = static_cast<uint16_t>(
			(static_cast<uint16_t>(event.payload[0]) << 8) | static_cast<uint16_t>(event.payload[1]));
		m_qsound_state.apply(event.payload[2], value);
	}

	size_t dac_instance = 0;
	gameaudio::vgm::ym2612_dac_event_kind dac_kind = gameaudio::vgm::ym2612_dac_event_kind::data;
	uint8_t dac_value = 0;
	bool dac_event = false;

	if (event.kind == gameaudio::vgm::command_event_kind::ym2612_dac &&
		event.payload != nullptr && event.payload_size >= 1)
	{
		dac_event = true;
		dac_instance = 0;
		dac_kind = gameaudio::vgm::ym2612_dac_event_kind::data;
		dac_value = event.payload[0];
	}
	else if (event.kind == gameaudio::vgm::command_event_kind::command &&
		event.payload != nullptr && event.payload_size >= 2 &&
		(event.command == 0x52 || event.command == 0xA2))
	{
		dac_instance = event.command == 0xA2 ? 1 : 0;
		if (event.payload[0] == 0x2A)
		{
			dac_event = true;
			dac_kind = gameaudio::vgm::ym2612_dac_event_kind::data;
			dac_value = event.payload[1];
		}
		else if (event.payload[0] == 0x2B)
		{
			dac_event = true;
			dac_kind = gameaudio::vgm::ym2612_dac_event_kind::enable;
			dac_value = event.payload[1];
		}
	}

	if (dac_event && m_dac_present[dac_instance] && m_dac_shadow_valid[dac_instance])
	{
		if (dac_kind == gameaudio::vgm::ym2612_dac_event_kind::enable)
			m_enhanced_dac[dac_instance].set_enabled((dac_value & 0x80) != 0);
		else
			m_enhanced_dac[dac_instance].write(dac_value);
	}
}

void input_vgm::replay_captured_sources(uint_fast32_t rendered_samples) noexcept
{
	for (size_t instance = 0; instance < m_enhanced_psg.size(); ++instance)
	{
		if (m_psg_present[instance] && m_psg_shadow_valid[instance])
		{
			if (m_psg_capture.overflowed(instance))
			{
				m_psg_shadow_valid[instance] = false;
			}
			else
			{
				const gameaudio::vgm::sn76489_timed_write* writes = m_psg_capture.writes(instance);
				const size_t write_count = m_psg_capture.count(instance);
				size_t cursor = 0;

				for (size_t index = 0; index < write_count; ++index)
				{
					const size_t offset = writes[index].sample_offset > rendered_samples
						? static_cast<size_t>(rendered_samples)
						: writes[index].sample_offset;
					if (offset < cursor)
					{
						m_psg_shadow_valid[instance] = false;
						break;
					}

					m_enhanced_psg[instance].advance(offset - cursor);
					cursor = offset;
					if (writes[index].kind == gameaudio::vgm::sn76489_write_kind::stereo_mask)
						m_enhanced_psg[instance].write_stereo_mask(writes[index].data);
					else
						m_enhanced_psg[instance].write(writes[index].data);
				}

				if (m_psg_shadow_valid[instance] && cursor < rendered_samples)
					m_enhanced_psg[instance].advance(static_cast<size_t>(rendered_samples) - cursor);
			}
		}

		if (m_dac_present[instance] && m_dac_shadow_valid[instance])
		{
			if (m_dac_capture.overflowed(instance))
				m_dac_shadow_valid[instance] = false;
			else
				m_enhanced_dac[instance].render_timed(
					m_dac_capture.events(instance), m_dac_capture.count(instance), nullptr, rendered_samples);
		}
	}

	if (m_qsound_present && m_qsound_shadow_valid)
	{
		if (m_qsound_capture.overflowed())
		{
			m_qsound_shadow_valid = false;
		}
		else
		{
			const gameaudio::vgm::qsound_timed_source_control* controls = m_qsound_capture.controls();
			const size_t control_count = m_qsound_capture.count();
			size_t previous_offset = 0;
			bool have_previous = false;

			for (size_t index = 0; index < control_count; ++index)
			{
				const size_t offset = controls[index].sample_offset > rendered_samples
					? static_cast<size_t>(rendered_samples)
					: controls[index].sample_offset;
				if (have_previous && offset < previous_offset)
				{
					m_qsound_shadow_valid = false;
					break;
				}
				previous_offset = offset;
				have_previous = true;
				m_qsound_state.apply(controls[index].write);
			}
		}
	}

	m_shadow_replay_sample = m_psg_capture.block_start_sample() + rendered_samples;
}

bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
	configure_enhancement_shadow();

	const uint_fast64_t block_start = m_vgm_player != nullptr
		? static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE))
		: m_played_sample;
	advance_shadow_to(block_start);
	m_psg_capture.begin_block(block_start);
	m_dac_capture.begin_block(block_start);
	m_qsound_capture.begin_block(block_start);
	m_qsound_audio_capture.begin_block();
	m_qsound_mix_capture.begin_block();

	bool qsound_audio_attached = false;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
	if (m_qsound_present && m_qsound_audio_shadow_valid && m_vgm_player != nullptr)
	{
		qsound_audio_attached =
			m_vgm_player->SetQSoundSourceObserver(&input_vgm::qsound_source_callback, this) != 0;
		if (!qsound_audio_attached)
			m_qsound_audio_shadow_valid = false;
	}
#endif
	m_qsound_audio_capture_active = qsound_audio_attached;

	bool qsound_mix_attached = false;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
	if (m_qsound_present && m_qsound_mix_shadow_valid && m_vgm_player != nullptr)
	{
		qsound_mix_attached =
			m_vgm_player->SetQSoundMixObserver(&input_vgm::qsound_mix_callback, this) != 0;
		if (!qsound_mix_attached)
			m_qsound_mix_shadow_valid = false;
	}
#endif
	m_qsound_mix_capture_active = qsound_mix_attached;
	m_source_capture_active = true;

	bool result = false;
	try
	{
		// This remains the only audible renderer. QSound source and native-mix
		// callbacks are read-only evidence sidecars around the same native frames;
		// p_chunk is still produced solely by the historical reference path.
		result = input_base::decode_run(p_chunk, p_abort);
	}
	catch (...)
	{
		m_source_capture_active = false;
		m_qsound_audio_capture_active = false;
		m_qsound_mix_capture_active = false;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
		if (qsound_audio_attached && m_vgm_player != nullptr)
			m_vgm_player->SetQSoundSourceObserver(nullptr, nullptr);
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
		if (qsound_mix_attached && m_vgm_player != nullptr)
			m_vgm_player->SetQSoundMixObserver(nullptr, nullptr);
#endif
		throw;
	}
	m_source_capture_active = false;
	m_qsound_audio_capture_active = false;
	m_qsound_mix_capture_active = false;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
	if (qsound_audio_attached && m_vgm_player != nullptr)
		m_vgm_player->SetQSoundSourceObserver(nullptr, nullptr);
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
	if (qsound_mix_attached && m_vgm_player != nullptr)
		m_vgm_player->SetQSoundMixObserver(nullptr, nullptr);
#endif

	if (!result)
		return false;

	if (qsound_audio_attached && !m_qsound_audio_capture.valid())
		m_qsound_audio_shadow_valid = false;
	if (qsound_mix_attached && !m_qsound_mix_capture.valid())
		m_qsound_mix_shadow_valid = false;

	// Both observers originate from the same coherent superctr native update.
	// A disagreement is evidence corruption, not a resampling problem to repair.
	if (qsound_audio_attached && qsound_mix_attached &&
		m_qsound_audio_shadow_valid && m_qsound_mix_shadow_valid)
	{
		const bool aligned =
			m_qsound_audio_capture.native_sample_rate() == m_qsound_mix_capture.native_sample_rate() &&
			m_qsound_audio_capture.count() == m_qsound_mix_capture.count() &&
			(m_qsound_audio_capture.count() == 0 ||
				m_qsound_audio_capture.first_native_sample() == m_qsound_mix_capture.first_native_sample());
		if (!aligned)
		{
			m_qsound_audio_shadow_valid = false;
			m_qsound_mix_shadow_valid = false;
		}
	}

	replay_captured_sources(m_render_done);
	return true;
}

void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
	configure_enhancement_shadow();
	m_source_capture_active = false;
	m_qsound_audio_capture_active = false;
	m_qsound_mix_capture_active = false;
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
	if (m_vgm_player != nullptr)
		m_vgm_player->SetQSoundSourceObserver(nullptr, nullptr);
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
	if (m_vgm_player != nullptr)
		m_vgm_player->SetQSoundMixObserver(nullptr, nullptr);
#endif
	m_qsound_audio_capture.begin_block();
	m_qsound_mix_capture.begin_block();

	// libvgm emits reset/replayed source events during seek. The command tap
	// rebuilds source controls while the historical QSound renderer remains
	// authoritative. Native source/mix audio is captured only during real decode
	// blocks, never while seeking through discarded audio.
	input_base::decode_seek(p_seconds, p_abort);

	if (m_vgm_player != nullptr)
		advance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
}
