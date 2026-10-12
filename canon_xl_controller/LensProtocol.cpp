// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file LensProtocol.cpp
 * @brief Encodes fixed-length lens requests and validates raw response frames.
 */
#include "LensProtocol.hpp"

#include "Config.hpp"

#include <string.h>

namespace xl
{
    namespace
    {
        // Preserve the unassigned fields of the historical working templates.
        const Frame init =
        {
            {
                0x01, 0x03, 0x0d, 0x01, 0x01, 0x7e, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0x8c, 0x8a, 0x8c, 0x8a, 0x8c, 0x8a, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0
            }
        };
        const Frame command =
        {
            {
                0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff,
                0, 0, 0, 0, 0, 0, 0x18, 0, 0, 0, 0x05, 0x08, 0xa0, 0x80, 0, 0x0d, 0xff, 0xff, 0xff,
                0
            }
        };

        /**
         * @brief Writes a signed 16-bit raw offset in big-endian byte order.
         * @param frame Caller-owned outgoing frame, modified in place.
         * @param index Zero-based high-byte position; index + 1 must fit in the frame.
         * @param value Signed raw offset, without an angular calibration.
         */
        void putSigned(Frame& frame, size_t index, int16_t value)
        {
            const uint16_t bits = static_cast<uint16_t>(value);
            frame.bytes[index] = static_cast<uint8_t>(bits >> 8);
            frame.bytes[index + 1] = static_cast<uint8_t>(bits);
        }
    } // namespace

    uint8_t reverseBits(uint8_t value)
    {
        value = static_cast<uint8_t>((value >> 4) | (value << 4));
        value = static_cast<uint8_t>(((value & 0xcc) >> 2) | ((value & 0x33) << 2));
        return static_cast<uint8_t>(((value & 0xaa) >> 1) | ((value & 0x55) << 1));
    }

    uint8_t checksum(const Frame& frame)
    {
        // The final byte makes the sum of all 40 bytes equal 0xff modulo 256.
        uint16_t sum = 0;

        for (size_t i = 0; i < checksumIndex; ++i)
        {
            sum += frame.bytes[i];
        }

        return static_cast<uint8_t>(~sum);
    }

    void seal(Frame& frame)
    {
        frame.bytes[checksumIndex] = checksum(frame);
    }

    bool validChecksum(const Frame& frame)
    {
        return frame.bytes[checksumIndex] == checksum(frame);
    }

    ResponseKind classify(const Frame& frame)
    {
        // Preserve uniform replies as distinct evidence, even with invalid checksums.
        bool zero = true, allFf = true;

        for (uint8_t byte : frame.bytes)
        {
            zero = zero && byte == 0;
            allFf = allFf && byte == 0xff;
        }

        if (zero)
        {
            return ResponseKind::Zero;
        }

        if (allFf)
        {
            return ResponseKind::AllFf;
        }

        if (!validChecksum(frame))
        {
            return ResponseKind::BadChecksum;
        }

        if (frame.bytes[0] == 1)
        {
            return ResponseKind::Init;
        }

        if (frame.bytes[0] == 2)
        {
            return ResponseKind::Normal;
        }

        return ResponseKind::Unknown;
    }

    const char* responseName(ResponseKind kind)
    {
        switch (kind)
        {
            case ResponseKind::Init:
                return "init";
            case ResponseKind::Normal:
                return "normal";
            case ResponseKind::Zero:
                return "zero";
            case ResponseKind::AllFf:
                return "ff";
            case ResponseKind::BadChecksum:
                return "checksum";
            case ResponseKind::Unknown:
                return "unknown";
            default:
                return "none";
        }
    }

    bool validControls(const Controls& controls)
    {
        return controls.zoom >= -15 && controls.zoom <= 15 &&
            controls.x >= -config::stabilisationLimit && controls.x <= config::stabilisationLimit &&
            controls.y >= -config::stabilisationLimit && controls.y <= config::stabilisationLimit;
    }

    uint8_t encodeZoom(int8_t zoom)
    {
        if (zoom == 0 || zoom < -15 || zoom > 15)
        {
            return 0;
        }

        // Bits 0..1 select direction; bits 2..5 carry the unsigned amplitude.
        const uint8_t amplitude =
            static_cast<uint8_t>(zoom < 0 ? -static_cast<int16_t>(zoom) : zoom);

        return static_cast<uint8_t>((amplitude << 2) | (zoom > 0 ? 1 : 2));
    }

    void buildInit(Frame& frame)
    {
        memcpy(&frame, &init, sizeof(frame));
        seal(frame);
    }

    bool buildCommand(const Controls& controls, Frame& frame)
    {
        if (!validControls(controls))
        {
            return false;
        }

        memcpy(&frame, &command, sizeof(frame));
        frame.bytes[irisCommandIndex] = controls.iris;
        putSigned(frame, 20, controls.x);
        putSigned(frame, 22, controls.y);
        frame.bytes[zoomCommandIndex] = encodeZoom(controls.zoom);
        seal(frame);

        return true;
    }
} // namespace xl
