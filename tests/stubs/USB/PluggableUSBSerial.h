// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file PluggableUSBSerial.h
 * @brief Models the external hardware API for deterministic host tests.
 */
#pragma once
#include <algorithm>
#include <deque>
#include <stdint.h>
#include <string>

namespace arduino
{
    class USBSerial
    {
        public:

        bool online = false;
        uint32_t quota = 64;
        uint32_t baud = 0;
        std::deque<uint8_t> input;
        std::string sent;

        void begin(unsigned long value)
        {
            baud = static_cast<uint32_t>(value);
        }

        bool connected() const
        {
            return online;
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
            const uint8_t value = input.front();
            input.pop_front();
            return value;
        }

        void send_nb(uint8_t* bytes, uint32_t count, uint32_t* actual, bool = true)
        {
            *actual = online ? std::min(count, quota) : 0;
            sent.append(reinterpret_cast<const char*>(bytes), *actual);
        }

        void push(const std::string& line)
        {
            for (unsigned char value : line)
            {
                input.push_back(value);
            }
        }
    };
} // namespace arduino

extern arduino::USBSerial _SerialUSB;
