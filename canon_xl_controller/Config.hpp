// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file Config.hpp
 * @brief Defines bus timing, bounded parsing and motion-lease configuration.
 */
#pragma once

#include <stdint.h>

namespace xl
{
    namespace config
    {
        /** Requested SPI frequency in hertz. */
        constexpr uint32_t spiHz = 1000000;

        /** Nominal interval between transactions in microseconds. */
        constexpr uint32_t framePeriodUs = 20000;

        /** Enable-low interval before the first byte, in microseconds. */
        constexpr uint32_t leadUs = 350;

        /** Nominal byte-start spacing in microseconds. */
        constexpr uint32_t bytePeriodUs = 20;

        /** MOSI-low interval after the final byte, in microseconds. */
        constexpr uint32_t tailUs = 52;

        /** Enable-high interval before restoring MOSI high, in microseconds. */
        constexpr uint32_t releaseUs = 4;

        /** Maximum motion lease in milliseconds; expiry preserves the iris command. */
        constexpr uint32_t commandTimeoutMs = 500;

        /** Partial serial-line timeout in milliseconds. */
        constexpr uint32_t lineTimeoutMs = 250;

        /** Output sampling interval in milliseconds, not the SPI transaction interval. */
        constexpr uint32_t telemetryPeriodMs = 100;

        /** Maximum absolute raw stabilisation offset; not an angular calibration. */
        constexpr int16_t stabilisationLimit = 5000;

        /** Consecutive invalid responses before restarting initialisation. */
        constexpr uint8_t badResponseLimit = 5;

        /** Maximum number of initialisation responses before restarting the probe. */
        constexpr uint16_t probeLimit = 100;

        /** UART baud rate; serial format is 8N1. */
        constexpr uint32_t serialBaud = 115200;
    } // namespace config
} // namespace xl
