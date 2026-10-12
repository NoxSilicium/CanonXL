// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file test_transport.cpp
 * @brief Exercises frame, dialogue and serial contracts without hardware.
 */
#include "Config.hpp"
#include "Rp2040Transport.hpp"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <cstdlib>
#include <iostream>
#include <vector>

#define XL_CHECK(expr)                                                                             \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            std::cerr << __LINE__ << ": " #expr "\n";                                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
spi_inst_t instance;
spi_inst_t* const spi0 = &instance;
uint32_t now;
bool levels[30] =
{
};
unsigned functions[30] =
{
};
std::vector<uint8_t> wire;
std::vector<uint32_t> starts;
uint32_t enabledAt, releasedAt, mosiHighAt;

void _gpio_init(unsigned pin)
{
    functions[pin] = GPIO_FUNC_SIO;
}

void gpio_put(unsigned pin, bool value)
{
    levels[pin] = value;
    if (pin == 5)
    {
        if (value)
        {
            releasedAt = now;
        }
        else
        {
            enabledAt = now;
        }
    }
    if (pin == 7 && value)
    {
        mosiHighAt = now;
    }
}

void gpio_set_dir(unsigned, bool)
{
}

void gpio_set_function(unsigned pin, unsigned function)
{
    functions[pin] = function;
}

uint32_t time_us_32()
{
    return now++;
}

unsigned _spi_init(spi_inst_t* spi, unsigned hz)
{
    XL_CHECK(spi == spi0 && hz == 1000000);
    return hz;
}

void spi_set_format(
    spi_inst_t* spi, unsigned bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order)
{
    XL_CHECK(spi == spi0 && bits == 8 && cpol == SPI_CPOL_1 && cpha == SPI_CPHA_1 &&
        order == SPI_MSB_FIRST);
}

int spi_write_read_blocking(spi_inst_t* spi, const uint8_t* tx, uint8_t* rx, size_t count)
{
    XL_CHECK(spi == spi0 && count == 1 && !levels[5]);
    XL_CHECK(functions[7] == GPIO_FUNC_SPI && functions[4] == GPIO_FUNC_SPI &&
        functions[6] == GPIO_FUNC_SPI);
    starts.push_back(now);
    wire.push_back(*tx);
    *rx = xl::reverseBits(static_cast<uint8_t>(wire.size() - 1));
    now += 8;
    return 1;
}

void run(uint32_t start)
{
    now = start;
    wire.clear();
    starts.clear();
    xl::Rp2040Transport transport;
    transport.begin();
    XL_CHECK(transport.actualHz() == 1000000 && levels[5] && levels[7]);
    XL_CHECK(functions[7] == GPIO_FUNC_SIO && functions[6] == GPIO_FUNC_SPI);
    xl::Frame tx, rx;
    xl::buildInit(tx);
    transport.exchange(tx, rx);
    XL_CHECK(wire.size() == 40 && starts.size() == 40);
    for (size_t i = 0; i < 40; ++i)
    {
        XL_CHECK(wire[i] == xl::reverseBits(tx.bytes[i]) && rx.bytes[i] == i);
        if (i)
        {
            XL_CHECK(starts[i] - starts[i - 1] >= 19 && starts[i] - starts[i - 1] <= 21);
        }
    }
    XL_CHECK(starts[0] - enabledAt >= xl::config::leadUs);
    XL_CHECK(releasedAt - starts.back() >= 8 + xl::config::tailUs);
    XL_CHECK(mosiHighAt - releasedAt >= xl::config::releaseUs);
    XL_CHECK(levels[5] && levels[7] && functions[7] == GPIO_FUNC_SIO);
}

int main()
{
    run(0);
    run(UINT32_MAX - 400);
    std::cout << "Transport model OK: mode 3, 1 MHz, LSB-first on both streams, 40 bytes, MOSI "
                 "lead/tail, rollover. No physical timing claim.\n";
}
