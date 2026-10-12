// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/** @file test_sketch.cpp @brief Exercises RP2040 sketch scheduling and serial flow. */
#include "../canon_xl_controller/canon_xl_controller.ino"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <cstdlib>
#include <iostream>

#define XL_CHECK(expr)                                                                             \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            std::cerr << __LINE__ << ": " #expr "\n";                                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

arduino::USBSerial _SerialUSB;
spi_inst_t instance;
spi_inst_t* const spi0 = &instance;
uint32_t clockUs = 0;
uint32_t transactions = 0;
size_t byteIndex = 0;
uint8_t replyHeader = 1;
bool corrupt = false;
xl::Frame reply, observed;

uint32_t millis()
{
    return clockUs / 1000;
}

uint32_t micros()
{
    return clockUs;
}

uint32_t time_us_32()
{
    return clockUs++;
}

void _gpio_init(unsigned)
{
}

void gpio_put(unsigned, bool)
{
}

void gpio_set_dir(unsigned, bool)
{
}

void gpio_set_function(unsigned, unsigned)
{
}

unsigned _spi_init(spi_inst_t*, unsigned hz)
{
    return hz;
}

void spi_set_format(spi_inst_t*, unsigned, spi_cpol_t, spi_cpha_t, spi_order_t)
{
}

int spi_write_read_blocking(spi_inst_t*, const uint8_t* tx, uint8_t* rx, size_t count)
{
    XL_CHECK(count == 1);
    if (!byteIndex)
    {
        reply = xl::Frame();
        reply.bytes[0] = replyHeader;
        reply.bytes[1] = 66;
        xl::seal(reply);
        if (corrupt)
        {
            reply.bytes[6] ^= 1;
            corrupt = false;
        }
    }
    observed.bytes[byteIndex] = xl::reverseBits(*tx);
    *rx = xl::reverseBits(reply.bytes[byteIndex]);
    clockUs += 8;
    if (++byteIndex == xl::frameLength)
    {
        byteIndex = 0;
        ++transactions;
    }
    return 1;
}

void advance(uint32_t ms)
{
    const uint32_t end = millis() + ms;
    while (millis() < end)
    {
        clockUs += 1000;
        loop();
    }
}

void send(const std::string& text)
{
    usb.push(text);
    loop();
}

int main()
{
    setup();
    XL_CHECK(usb.baud == 115200);
    advance(25);
    XL_CHECK(controller.phase() == xl::Phase::Probing);
    replyHeader = 2;
    advance(25);
    XL_CHECK(controller.phase() == xl::Phase::Ready);
    XL_CHECK(transactions == 2 && observed.bytes[0] == 2);
    XL_CHECK(controller.controls().iris == 255 && controller.controls().zoom == 0);

    usb.online = true;
    loop();
    send("STREAM,0\n");
    XL_CHECK(usb.sent == "ACK,STREAM\n");
    usb.sent.clear();
    send("SET,0x70,8,-500,1000\n");
    XL_CHECK(controller.controls().zoom == 8);
    XL_CHECK(usb.sent == "ACK,SET\n");
    usb.sent.clear();
    advance(25);
    XL_CHECK(observed.bytes[28] == 0x21 && xl::validChecksum(observed));
    const xl::Controls before = controller.controls();
    send("SET,1,8,2,garbage\n");
    XL_CHECK(controller.controls().iris == before.iris && controller.controls().x == before.x);
    XL_CHECK(usb.sent == "ERR,line\n");
    usb.sent.clear();
    send("SET,1,-8,0,");
    XL_CHECK(controller.controls().zoom == 8);
    advance(251);
    XL_CHECK(usb.sent == "ERR,line-timeout\n");
    usb.sent.clear();
    send("0\n");
    XL_CHECK(controller.controls().zoom == 8 && usb.sent.empty());
    send("STATUS\n");
    XL_CHECK(usb.sent.find("XL,1,") == 0);
    usb.sent.clear();
    advance(230);
    XL_CHECK(controller.controls().zoom == 0 && controller.controls().iris == 0x70);
    XL_CHECK(controller.controls().x == 0 && controller.controls().y == 0);

    send("SET,0x70,8,0,0\n");
    XL_CHECK(controller.controls().zoom == 8);
    usb.sent.clear();
    usb.online = false;
    send("SET,1,-8,0,0\n");
    XL_CHECK(controller.controls().zoom == 0 && usb.input.empty() && usb.sent.empty());
    for (int i = 0; i < 12; ++i)
    {
        usb.push("SET,1,-8,0,0\n");
    }
    loop();
    XL_CHECK(!usb.input.empty());
    usb.online = true;
    for (int i = 0; i < 10; ++i)
    {
        loop();
    }
    XL_CHECK(usb.input.empty() && controller.controls().zoom == 0 && usb.sent.empty());
    advance(25);
    XL_CHECK(observed.bytes[28] == 0);
    send("SET,0x70,-8,0,0\n");
    usb.sent.clear();
    advance(25);
    XL_CHECK(observed.bytes[28] == 0x22);
    corrupt = true;
    advance(25);
    XL_CHECK(controller.controls().zoom == 0 && controller.phase() == xl::Phase::Probing);
    advance(25);
    XL_CHECK(controller.phase() == xl::Phase::Ready && observed.bytes[28] == 0);

    send("RESET\nSTATUS\n");
    XL_CHECK(controller.phase() == xl::Phase::Initialising);
    XL_CHECK(usb.sent.find("ACK,RESET\nXL,1,") == 0 &&
        usb.sent.find(",init,none,") != std::string::npos);
    XL_CHECK(usb.sent.find(",-1,-1,-1,-1,-1,") != std::string::npos);
    usb.sent.clear();
    advance(25);

    XL_CHECK(!rawStream && usb.sent.find("XLRAW,1,") == std::string::npos);
    usb.sent.clear();
    send("RAW,1\n");
    XL_CHECK(usb.sent == "ACK,RAW\n");
    usb.sent.clear();
    const uint32_t rawStart = transactions;
    advance(150);
    XL_CHECK(usb.sent.find("XLRAW,1,") == 0 && usb.sent.find("XL,1,") == std::string::npos);
    XL_CHECK(usb.sent.back() == '\n' && transactions - rawStart >= 7);
    XL_CHECK(controller.controls().iris == 255 && !controller.hasCommand());
    usb.sent.clear();
    send("RAW,0\n");
    XL_CHECK(usb.sent == "ACK,RAW\n");
    usb.sent.clear();
    advance(150);
    XL_CHECK(usb.sent.empty());
    send("SET,0x70,8,0,0\n");
    usb.sent.clear();
    send("RAW,1\n");
    usb.sent.clear();
    advance(501);
    XL_CHECK(controller.controls().zoom == 0 && controller.controls().iris == 0x70);
    usb.sent.clear();
    send("STREAM,1\n");
    usb.sent.clear();
    advance(150);
    XL_CHECK(usb.sent.find("XL,1,") != std::string::npos &&
        usb.sent.find("XLRAW,1,") != std::string::npos);
    XL_CHECK(output.dropped() == 0);

    usb.sent.clear();
    usb.quota = 0;
    const uint32_t start = transactions;
    advance(2000);
    XL_CHECK(transactions - start >= 99 && usb.sent.empty() && output.dropped() > 0);
    usb.quota = 3;
    loop();
    XL_CHECK(usb.sent.size() == 3);
    usb.quota = 64;
    send("STREAM,0\nRAW,0\n");
    advance(25);
    XL_CHECK(usb.sent.find("XL,1,") == 0 && usb.sent.back() == '\n');

    usb.sent.clear();
    send("SET,0x70,8,0,0\n");
    usb.sent.clear();
    clockUs += 100000;
    const uint32_t skippedBefore = frames.skipped();
    const uint32_t exchangesBefore = transactions;
    loop();
    XL_CHECK(transactions == exchangesBefore + 1 && frames.skipped() > skippedBefore);
    send("STOP\n");
    XL_CHECK(controller.controls().zoom == 0 && controller.controls().iris == 0x70);
    std::cout << "Sketch model OK: USB backpressure/partial writes, disconnect, atomic input, "
                 "watchdog, recovery, raw status, no catch-up bursts.\n";
}
