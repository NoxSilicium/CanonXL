// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file Telemetry.hpp
 * @brief Formats the stable XL,1 and XLRAW,1 serial records without allocation.
 */
#pragma once
#include "LensController.hpp"

namespace xl
{
    /**
     * @brief Formats all 23 XL,1 fields; unavailable functional bytes use -1.
     * @param output Caller-owned writable character buffer.
     * @param capacity Available output bytes, including the terminating NUL.
     * @param controller Source of the last response, active controls and counters.
     * @param now Millisecond timer reading.
     * @param skipped Cumulative SPI scheduler slots missed.
     * @param dropped Cumulative complete output messages rejected by the queue.
     * @param actualHz Configured transport frequency in hertz, not a bus measurement.
     * @return False on formatting failure or insufficient capacity.
     */
    bool formatTelemetry(char* output, size_t capacity, const LensController& controller,
        uint32_t now, uint32_t skipped, uint32_t dropped, uint32_t actualHz);

    /**
     * @brief Formats the last received 40-byte response as an XLRAW,1 record.
     * @param output Caller-owned writable character buffer.
     * @param capacity Available output bytes, including the terminating NUL.
     * @param controller Source of the last received frame and transaction number.
     * @param now Millisecond timer reading; may differ from the paired XL record.
     * @return False before reception, on formatting failure or insufficient capacity.
     */
    bool formatRawTelemetry(
        char* output, size_t capacity, const LensController& controller, uint32_t now);
} // namespace xl
