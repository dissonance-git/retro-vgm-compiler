#include "input_vgm.h"

void input_vgm::reset_qsound_consumer_source_path(bool source_observer_available) noexcept
{
	m_qsound_consumer_source_configured = false;
	m_qsound_consumer_source_shadow_valid = m_qsound_present && source_observer_available;
	m_qsound_consumer_source_window.reset();
	m_qsound_consumer_source_storage.reset();
}

void input_vgm::project_qsound_consumer_sources(
	uint_fast64_t block_start,
	uint_fast32_t rendered_samples) noexcept
{
	m_qsound_consumer_source_storage.reset();

	if (!m_qsound_consumer_source_shadow_valid)
		return;

	// The consumer projection is downstream of the exact native source observer.
	// Once that observer fails, do not revive this path until an explicit source
	// reset/seek re-establishes the timeline.
	if (!m_qsound_audio_shadow_valid)
	{
		m_qsound_consumer_source_shadow_valid = false;
		return;
	}

	const size_t native_count = m_qsound_audio_capture.count();
	if (!m_qsound_consumer_source_configured)
	{
		// An empty observer block contains no native-rate evidence. Keep the path
		// eligible, but do not guess a rate merely because QSound is present.
		if (native_count == 0)
			return;

		if (!m_qsound_consumer_time_map.configure(
			m_qsound_audio_capture.native_sample_rate(),
			static_cast<std::uint32_t>(m_sample_rate)))
		{
			m_qsound_consumer_source_shadow_valid = false;
			return;
		}
		m_qsound_consumer_source_configured = true;
	}
	else if (m_qsound_audio_capture.native_sample_rate() != 0 &&
		(m_qsound_audio_capture.native_sample_rate() != m_qsound_consumer_time_map.native_rate() ||
		 static_cast<std::uint32_t>(m_sample_rate) != m_qsound_consumer_time_map.output_rate()))
	{
		// Rate changes are structural events. This first consumer contract does
		// not splice two time maps together inside one source epoch.
		m_qsound_consumer_source_shadow_valid = false;
		return;
	}

	if (!m_qsound_consumer_source_window.begin_block(
		m_qsound_audio_capture.frames(), native_count))
	{
		m_qsound_consumer_source_shadow_valid = false;
		return;
	}

	const gameaudio::vgm::qsound_consumer_source_result projected =
		m_qsound_consumer_source_storage.render(
			m_qsound_consumer_time_map,
			m_qsound_consumer_source_window,
			static_cast<std::uint64_t>(block_start),
			static_cast<std::size_t>(rendered_samples));

	const bool window_closed = m_qsound_consumer_source_window.end_block();
	if (!projected.structurally_valid || !window_closed)
	{
		m_qsound_consumer_source_shadow_valid = false;
		m_qsound_consumer_source_storage.reset();
	}

	// Missing source brackets are intentionally not a structural failure. The
	// storage availability mask distinguishes unavailable evidence from genuine
	// source silence, including the known startup pre-sample gap in libvgm's
	// linear-upsample initialization path.
}
