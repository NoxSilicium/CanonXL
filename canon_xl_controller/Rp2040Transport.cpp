// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file Rp2040Transport.cpp
 * @brief Transfers LSB-first frames through the RP2040 MSB-first SPI peripheral.
 */
#include "Rp2040Transport.hpp"

#include "Config.hpp"

#include <Arduino.h>

#if defined(ARDUINO_NANO_RP2040_CONNECT)

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/timer.h"

namespace xl
{
    namespace
    {
        // Physical RP2040 GPIO numbers, not Arduino D-pin numbers.
        constexpr unsigned enable = 5; // D10
        constexpr unsigned mosi = 7; // D11
        constexpr unsigned miso = 4; // D12
        constexpr unsigned clock = 6; // D13

        /** @brief Busy-waits for microseconds using the wrap-safe RP2040 timer. */
        void waitUs(uint32_t duration)
        {
            const uint32_t start = time_us_32();
            while (time_us_32() - start < duration)
            {
            }
        }
    } // namespace

    void Rp2040Transport::begin()
    {
        _gpio_init(enable);
        gpio_put(enable, 1);
        gpio_set_dir(enable, GPIO_OUT);
        _gpio_init(mosi);
        gpio_put(mosi, 1);
        gpio_set_dir(mosi, GPIO_OUT);
        _gpio_init(clock);
        gpio_put(clock, 1);
        gpio_set_dir(clock, GPIO_OUT);

        configuredHz = _spi_init(spi0, config::spiHz);
        spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        gpio_set_function(miso, GPIO_FUNC_SPI);
        gpio_set_function(clock, GPIO_FUNC_SPI);
    }

    void Rp2040Transport::exchange(const Frame& tx, Frame& rx)
    {
        gpio_put(enable, 0);
        gpio_put(mosi, 0);
        waitUs(config::leadUs);

        // The lens lead/tail timings require GPIO control outside SPI transfers.
        gpio_set_function(mosi, GPIO_FUNC_SPI);
        const uint32_t firstByteAt = time_us_32();

        for (size_t i = 0; i < frameLength; ++i)
        {
            while (time_us_32() - firstByteAt < i * config::bytePeriodUs)
            {
            }

            // PL022 is MSB-first: reversing both directions puts LSB-first on the wire.
            const uint8_t output = reverseBits(tx.bytes[i]);
            uint8_t input;
            spi_write_read_blocking(spi0, &output, &input, 1);
            rx.bytes[i] = reverseBits(input);
        }

        gpio_put(mosi, 0);
        gpio_set_function(mosi, GPIO_FUNC_SIO);
        waitUs(config::tailUs);
        gpio_put(enable, 1);
        waitUs(config::releaseUs);
        gpio_put(mosi, 1);
    }
} // namespace xl
#endif
