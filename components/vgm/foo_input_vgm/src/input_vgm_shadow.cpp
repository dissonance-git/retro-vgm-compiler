#include "input_vgm.h"

#include <emu/cores/sn764intf.h>

void input_vgm::configure_enhanced_psg()
{
	if (m_psg_configured)
		return;
	m_psg_configured = true;
	m_psg_replay_sample = 0;

#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	m_genesis_state.set_event_tap(&input_vgm::source_event_tap, this);

	for (const PLR_DEV_INFO& device : m_pdi_list)
	{
		if (device.type != DEVID_SN76496 || device.parentIdx != (uint32_t)-1 ||
			device.instance >= m_enhanced_psg.size() || device.devCfg == nullptr)
			continue;

		const size_t instance = static_cast<size_t>(device.instance);
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

		// libvgm uses the generic device flag to identify T6W28 split-chip
		// behavior. That topology and NCR-style timing stay on the established
		// reference renderer until a dedicated enhanced model is validated.
		m_psg_config_supported[instance] =
			(source_cfg->_genCfg.flags == 0) && m_enhanced_psg[instance].supported();
		m_psg_shadow_valid[instance] = m_psg_config_supported[instance];
	}
#endif
}

void input_vgm::source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept
{
	input_vgm* self = static_cast<input_vgm*>(user_param);
	if (self == nullptr || !self->m_psg_configured)
		return;

	if (event.kind == gameaudio::vgm::command_event_kind::reset)
	{
		for (size_t instance = 0; instance < self->m_enhanced_psg.size(); ++instance)
		{
			self->m_enhanced_psg[instance].reset();
			self->m_psg_shadow_valid[instance] = self->m_psg_config_supported[instance];
		}
		self->m_psg_replay_sample = 0;
		return;
	}

	if (self->m_vgm_player == nullptr)
		return;

	const uint_fast64_t absolute_sample =
		static_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

	if (self->m_psg_capture_active)
	{
		self->m_psg_capture.observe(event, absolute_sample);
		return;
	}

	self->advance_psg_to(absolute_sample);
	self->apply_psg_event_outside_render(event);
}

void input_vgm::advance_psg_to(uint_fast64_t absolute_sample) noexcept
{
	if (absolute_sample <= m_psg_replay_sample)
		return;

	const uint_fast64_t delta64 = absolute_sample - m_psg_replay_sample;
	const size_t delta = delta64 > static_cast<uint_fast64_t>(SIZE_MAX)
		? SIZE_MAX
		: static_cast<size_t>(delta64);

	for (size_t instance = 0; instance < m_enhanced_psg.size(); ++instance)
	{
		if (m_psg_present[instance] && m_psg_shadow_valid[instance])
			m_enhanced_psg[instance].advance(delta);
	}
	m_psg_replay_sample = absolute_sample;
}

void input_vgm::apply_psg_event_outside_render(const gameaudio::vgm::command_event& event)
{
	if (event.kind != gameaudio::vgm::command_event_kind::command ||
		event.payload == nullptr || event.payload_size < 1)
		return;

	size_t instance = 0;
	bool stereo_mask = false;
	switch (event.command)
	{
	case 0x50:
		instance = 0;
		break;
	case 0x30:
		instance = 1;
		break;
	case 0x4F:
		instance = 0;
		stereo_mask = true;
		break;
	default:
		return;
	}

	if (!m_psg_present[instance] || !m_psg_shadow_valid[instance])
		return;

	if (stereo_mask)
		m_enhanced_psg[instance].write_stereo_mask(event.payload[0]);
	else
		m_enhanced_psg[instance].write(event.payload[0]);
}

void input_vgm::replay_captured_psg(uint_fast32_t rendered_samples) noexcept
{
	for (size_t instance = 0; instance < m_enhanced_psg.size(); ++instance)
	{
		if (!m_psg_present[instance] || !m_psg_shadow_valid[instance])
			continue;

		if (m_psg_capture.overflowed(instance))
		{
			// Dropping even one register write makes future source state ambiguous.
			// Disable the shadow until the next player reset instead of guessing.
			m_psg_shadow_valid[instance] = false;
			continue;
		}

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

	m_psg_replay_sample = m_psg_capture.block_start_sample() + rendered_samples;
}

bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
	configure_enhanced_psg();

	const uint_fast64_t block_start = m_vgm_player != nullptr
		? static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE))
		: m_played_sample;
	advance_psg_to(block_start);
	m_psg_capture.begin_block(block_start);
	m_psg_capture_active = true;

	bool result = false;
	try
	{
		// This is still the only audible renderer. The enhanced PSG runs in
		// shadow so its timing/state can be validated before any substitution.
		result = input_base::decode_run(p_chunk, p_abort);
	}
	catch (...)
	{
		m_psg_capture_active = false;
		throw;
	}
	m_psg_capture_active = false;

	if (!result)
		return false;

	replay_captured_psg(m_render_done);
	return true;
}

void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
	configure_enhanced_psg();
	m_psg_capture_active = false;

	// libvgm emits reset/replayed source events during the seek. The event tap
	// advances the shadow between those writes, rebuilding oscillator/LFSR state
	// without generating discarded audio.
	input_base::decode_seek(p_seconds, p_abort);

	if (m_vgm_player != nullptr)
		advance_psg_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
}
