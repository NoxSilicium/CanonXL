// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file AvrTransport.hpp
 * @brief Exposes the fixed-pin, 1 MHz ATmega328P SPI adapter.
 */
#pragma once

#include "LensProtocol.hpp"

namespace xl
{
    /** Blocking SPI transaction adapter; invoke only from the Arduino loop. */
    class AvrTransport
    {
        public:

        /** @brief Configures D10..D13 without starting a transaction. */
        void begin();

        /** @brief Transfers exactly 40 bytes using the configured enable timing. */
        void exchange(const Frame& tx, Frame& rx);

        /** @brief Returns the fixed hardware divider frequency, not a measured rate. */
        uint32_t actualHz() const
        {
            return 1000000;
        }
    };
} // namespace xl
