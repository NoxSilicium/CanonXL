// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file Arduino.h
 * @brief Models the external hardware API for deterministic host tests.
 */
#pragma once
#include <stdint.h>
uint32_t millis();
uint32_t micros();
#if defined(ARDUINO_AVR_NANO)
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <string>

class HardwareSerial
{
    public:

    unsigned long baud = 0;
    int quota = 63;
    std::deque<uint8_t> input;
    std::string sent;

    void begin(unsigned long value)
    {
        baud = value;
    }

    int available() const
    {
        return static_cast<int>(input.size());
    }

    int read()
    {
        if (input.empty())
        {
            return -1;
        }
        const uint8_t v = input.front();
        input.pop_front();
        return v;
    }

    int availableForWrite() const
    {
        return quota;
    }

    size_t write(const uint8_t* data, size_t count)
    {
        if (count > static_cast<size_t>(std::max(quota, 0)))
        {
            std::abort();
        }
        sent.append(reinterpret_cast<const char*>(data), count);
        return count;
    }

    void push(const std::string& line)
    {
        for (unsigned char c : line)
        {
            input.push_back(c);
        }
    }
};

extern HardwareSerial Serial;
#endif
