// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file Telemetry.cpp
 * @brief Formats versioned state and full-response telemetry into caller buffers.
 */
#include "Telemetry.hpp"

#include <stdio.h>

namespace xl
{
    bool formatTelemetry(char* output, size_t capacity, const LensController& controller,
        uint32_t now, uint32_t skipped, uint32_t dropped, uint32_t actualHz)
    {
        const Controls& command = controller.controls();
        const Frame& response = controller.response();
        const bool normal = controller.responseKind() == ResponseKind::Normal;
        const Statistics& stats = controller.statistics();

        // Preserve the XL field order; -1 means normal feedback is unavailable.
        const int length = snprintf(output, capacity,
            "XL,1,%lu,%s,%s,%u,%u,%d,%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%ld\n",
            static_cast<unsigned long>(now), phaseName(controller.phase()),
            responseName(controller.responseKind()), static_cast<unsigned>(command.iris),
            static_cast<unsigned>(encodeZoom(command.zoom)), static_cast<int>(command.x),
            static_cast<int>(command.y), normal ? response.bytes[1] : -1,
            normal ? response.bytes[3] : -1, normal ? response.bytes[4] : -1,
            normal ? response.bytes[15] : -1, normal ? response.bytes[16] : -1,
            static_cast<unsigned long>(stats.exchanges),
            static_cast<unsigned long>(stats.badChecksums), static_cast<unsigned long>(stats.zero),
            static_cast<unsigned long>(stats.allFf), static_cast<unsigned long>(stats.unknown),
            static_cast<unsigned long>(skipped), static_cast<unsigned long>(dropped),
            static_cast<unsigned long>(actualHz),
            controller.hasCommand() ? static_cast<long>(controller.commandAge(now)) : -1L);

        return length >= 0 && static_cast<size_t>(length) < capacity;
    }

    bool formatRawTelemetry(
        char* output, size_t capacity, const LensController& controller, uint32_t now)
    {
        if (controller.responseKind() == ResponseKind::None)
        {
            return false;
        }

        const Frame& response = controller.response();
        const int length =
            snprintf(output, capacity, "XLRAW,1,%lu,%lu,%s,%s,%u,", static_cast<unsigned long>(now),
                static_cast<unsigned long>(controller.statistics().exchanges),
                phaseName(controller.phase()), responseName(controller.responseKind()),
                static_cast<unsigned>(validChecksum(response)));

        if (length < 0 || static_cast<size_t>(length) >= capacity)
        {
            return false;
        }

        size_t offset = static_cast<size_t>(length);

        // Reserve two hex characters per byte, followed by newline and terminator.
        if (capacity - offset < 2 * frameLength + 2)
        {
            return false;
        }

        const char hex[] = "0123456789ABCDEF";

        for (size_t i = 0; i < frameLength; ++i)
        {
            output[offset++] = hex[response.bytes[i] >> 4];
            output[offset++] = hex[response.bytes[i] & 0x0f];
        }

        output[offset++] = '\n';
        output[offset] = '\0';

        return true;
    }
} // namespace xl
