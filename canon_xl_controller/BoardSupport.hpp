// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file BoardSupport.hpp
 * @brief Selects the board transport and bounded serial endpoint implementation.
 */
#pragma once

#include <Arduino.h>

#if defined(ARDUINO_NANO_RP2040_CONNECT)
#include "Rp2040Transport.hpp"
#include <USB/PluggableUSBSerial.h>

namespace xl
{
    /** Board-selected blocking lens bus adapter. */
    using LensTransport = Rp2040Transport;

    /** Returns the core-owned USB CDC endpoint; callers must not retain ownership. */
    inline arduino::USBSerial& serialEndpoint()
    {
        return _SerialUSB;
    }

    /** Reports the host connection state exposed by the USB core. */
    inline bool serialConnected()
    {
        return _SerialUSB.connected();
    }

    /** Writes at most size bytes without waiting; returns the accepted byte count. */
    inline size_t serialWrite(uint8_t* data, size_t size)
    {
        uint32_t written = 0;
        _SerialUSB.send_nb(data, static_cast<uint32_t>(size), &written);
        return written;
    }
} // namespace xl
#elif defined(ARDUINO_AVR_NANO) && defined(__AVR_ATmega328P__) && F_CPU == 16000000UL
#include "AvrTransport.hpp"

namespace xl
{
    /** Board-selected blocking lens bus adapter. */
    using LensTransport = AvrTransport;

    /** Returns the core-owned UART endpoint; callers must not retain ownership. */
    inline HardwareSerial& serialEndpoint()
    {
        return Serial;
    }

    /** Always true: the bridge does not expose DTR; the command lease is mandatory. */
    inline bool serialConnected()
    {
        return true;
    }

    /** Writes at most available UART capacity without waiting; returns bytes accepted. */
    inline size_t serialWrite(uint8_t* data, size_t size)
    {
        const int free = Serial.availableForWrite();
        if (free <= 0)
        {
            return 0;
        }
        if (size > static_cast<size_t>(free))
        {
            size = static_cast<size_t>(free);
        }
        return Serial.write(data, size);
    }
} // namespace xl
#else
#error "Select Nano ATmega328P 16 MHz or Nano RP2040 Connect."
#endif
