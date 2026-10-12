// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file Rp2040Transport.hpp
 * @brief Exposes the Nano RP2040 Connect SPI adapter with software bit reversal.
 */
#pragma once

#include "LensProtocol.hpp"

namespace xl
{
    /** Blocking SPI adapter; the PL022 peripheral requires per-byte bit reversal. */
    class Rp2040Transport
    {
        public:

        /** @brief Configures physical GPIOs and remembers the selected SPI frequency. */
        void begin();

        /** @brief Transfers one complete frame with LSB-first ordering on the wire. */
        void exchange(const Frame& tx, Frame& rx);

        /** @brief Returns the SDK-selected frequency in hertz, not a measured rate. */
        uint32_t actualHz() const
        {
            return configuredHz;
        }

        private:

        /** SDK-selected SPI frequency in hertz; zero until begin() configures the bus. */
        uint32_t configuredHz = 0;
    };
} // namespace xl
