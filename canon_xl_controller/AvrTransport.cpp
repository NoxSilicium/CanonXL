// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file AvrTransport.cpp
 * @brief Transfers frames through native AVR SPI with explicit idle MOSI timing.
 */
#include "AvrTransport.hpp"

#include "Config.hpp"

#include <Arduino.h>
#if defined(ARDUINO_AVR_NANO) && defined(__AVR_ATmega328P__) && F_CPU == 16000000UL
#include <avr/io.h>

namespace xl
{
    static_assert(config::spiHz == 1000000, "AVR adapter requires the fixed 1 MHz SPI divider.");

    namespace
    {
        constexpr uint8_t enable = _BV(PB2); // D10
        constexpr uint8_t mosi = _BV(PB3); // D11
        constexpr uint8_t miso = _BV(PB4); // D12
        constexpr uint8_t clock = _BV(PB5); // D13

        /** @brief Busy-waits for microseconds using wrap-safe elapsed-time arithmetic. */
        void waitUs(uint32_t duration)
        {
            const uint32_t start = micros();
            while (static_cast<uint32_t>(micros() - start) < duration)
            {
            }
        }
    } // namespace

    void AvrTransport::begin()
    {
        PORTB |= enable | mosi | clock;
        PORTB &= static_cast<uint8_t>(~miso);
        DDRB |= enable | mosi | clock;
        DDRB &= static_cast<uint8_t>(~miso);

        // 16 MHz / 16 = 1 MHz. SPI disabled between frames gives MOSI back to GPIO.
        SPCR = _BV(MSTR) | _BV(DORD) | _BV(CPOL) | _BV(CPHA) | _BV(SPR0);
        SPSR = 0;
    }

    void AvrTransport::exchange(const Frame& tx, Frame& rx)
    {
        PORTB &= static_cast<uint8_t>(~(enable | mosi));
        waitUs(config::leadUs);

        // Hand MOSI to SPI only during the frame; GPIO owns the lead and tail levels.
        SPCR |= _BV(SPE);
        const uint32_t firstByteAt = micros();

        for (size_t i = 0; i < frameLength; ++i)
        {
            while (static_cast<uint32_t>(micros() - firstByteAt) < i * config::bytePeriodUs)
            {
            }

            SPDR = tx.bytes[i];
            while (!(SPSR & _BV(SPIF)))
            {
            }
            rx.bytes[i] = SPDR;
        }

        // Preserve the MOSI-low tail before releasing enable, then restore MOSI high.
        SPCR &= static_cast<uint8_t>(~_BV(SPE));
        PORTB &= static_cast<uint8_t>(~mosi);
        waitUs(config::tailUs);
        PORTB |= enable;
        waitUs(config::releaseUs);
        PORTB |= mosi;
    }
} // namespace xl
#endif
