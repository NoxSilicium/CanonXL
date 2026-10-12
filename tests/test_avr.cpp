// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/** @file test_avr.cpp @brief Exercises the AVR sketch through host peripheral models. */
#include "../canon_xl_controller/canon_xl_controller.ino"
#include "avr/io.h"
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
HardwareSerial Serial;
volatile uint8_t PORTB = 0, DDRB = 0, SPCR = 0, SPSR = 0;
FakeDataRegister SPDR;
uint32_t now = 0, transactions = 0;
size_t byteIndex = 0;
uint8_t header = 1, input = 0;
xl::Frame reply, observed;
std::vector<uint32_t> starts;

uint32_t millis()
{
    return now / 1000;
}

uint32_t micros()
{
    const uint32_t result = now;
    now += 4;
    return result;
}

void FakeDataRegister::operator=(uint8_t value)
{
    XL_CHECK(SPCR == (_BV(MSTR) | _BV(DORD) | _BV(CPOL) | _BV(CPHA) | _BV(SPR0) | _BV(SPE)));
    XL_CHECK(!(PORTB & _BV(PB2)) && !(PORTB & _BV(PB3)) && (PORTB & _BV(PB5)));
    if (!byteIndex)
    {
        reply = xl::Frame();
        reply.bytes[0] = header;
        reply.bytes[1] = 50;
        xl::seal(reply);
        starts.clear();
    }
    starts.push_back(now);
    observed.bytes[byteIndex] = value;
    input = reply.bytes[byteIndex];
    now += 8;
    SPSR |= _BV(SPIF);
    if (++byteIndex == xl::frameLength)
    {
        byteIndex = 0;
        ++transactions;
    }
}

FakeDataRegister::operator uint8_t() const
{
    SPSR &= static_cast<uint8_t>(~_BV(SPIF));
    return input;
}

void advance(uint32_t ms)
{
    const uint32_t end = millis() + ms;
    while (millis() < end)
    {
        now += 1000;
        loop();
    }
}

void send(const char* line)
{
    usb.push(line);
    loop();
}

int main()
{
    setup();
    loop();
    XL_CHECK(usb.baud == 115200 && xl::serialConnected());
    XL_CHECK(transport.actualHz() == 1000000);
    XL_CHECK((DDRB & 0x3c) == (_BV(PB2) | _BV(PB3) | _BV(PB5)));
    XL_CHECK((PORTB & 0x3c) == (_BV(PB2) | _BV(PB3) | _BV(PB5)) && !(SPCR & _BV(SPE)));
    advance(25);
    XL_CHECK(controller.phase() == xl::Phase::Probing && starts.size() == 40);
    XL_CHECK(observed.bytes[0] == 1 && observed.bytes[39] == 0x2b && xl::validChecksum(observed));
    for (size_t i = 1; i < starts.size(); ++i)
    {
        XL_CHECK(starts[i] - starts[i - 1] >= 16 && starts[i] - starts[i - 1] <= 24);
    }
    XL_CHECK(!(SPCR & _BV(SPE)) && (PORTB & _BV(PB2)) && (PORTB & _BV(PB3)));
    header = 2;
    advance(25);
    XL_CHECK(controller.phase() == xl::Phase::Ready);
    send("STREAM,0\n");
    usb.sent.clear();
    send("SET,0x18,8,100,-200\n");
    XL_CHECK(usb.sent == "ACK,SET\n");
    advance(25);
    XL_CHECK(
        observed.bytes[19] == 0x18 && observed.bytes[28] == 0x21 && xl::validChecksum(observed));
    XL_CHECK(controller.response().bytes[1] == 50);
    advance(501);
    XL_CHECK(controller.controls().zoom == 0 && controller.controls().x == 0 &&
        controller.controls().y == 0);
    XL_CHECK(controller.controls().iris == 0x18);
    usb.sent.clear();
    send("RAW,1\n");
    XL_CHECK(usb.sent == "ACK,RAW\n");
    usb.sent.clear();
    advance(150);
    XL_CHECK(usb.sent.find("XLRAW,1,") == 0 && usb.sent.back() == '\n');
    XL_CHECK(controller.controls().iris == 0x18 && controller.controls().zoom == 0);
    usb.sent.clear();
    usb.quota = 0;
    send("STREAM,1\n");
    const uint32_t before = transactions;
    advance(2000);
    XL_CHECK(usb.sent.empty() && transactions - before >= 99 && output.dropped() > 0);
    usb.quota = 3;
    loop();
    XL_CHECK(usb.sent.size() == 3);
    usb.quota = 63;
    send("STREAM,0\nRAW,0\n");
    advance(25);
    XL_CHECK(usb.sent.back() == '\n');
    send("SET,0xFF,0,0,0\n");
    advance(25);
    XL_CHECK(observed.bytes[19] == 255 && observed.bytes[28] == 0);
    std::cout << "AVR sketch/transport model OK: 1 MHz mode 3 native LSB-first, GPIO idle, 40 "
                 "bytes, bounded UART writes and 500 ms lease.\n";
}
