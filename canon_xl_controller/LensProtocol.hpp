// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file LensProtocol.hpp
 * @brief Defines fixed-size Canon XL frames and their validated command encoding.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace xl
{
    /** Number of bytes in each full-duplex SPI transaction. */
    constexpr size_t frameLength = 40;

    /** Zero-based MOSI iris command position; values are uncalibrated lens codes. */
    constexpr size_t irisCommandIndex = 19;

    /** Zero-based MOSI zoom direction/amplitude position. */
    constexpr size_t zoomCommandIndex = 28;

    /** Zero-based checksum position; the complete byte sum is 0xff modulo 256. */
    constexpr size_t checksumIndex = 39;

    /** Bytes in bus order, without optical interpretation. */
    struct Frame
    {
        /** Bytes in the order transmitted or received by the SPI peripheral. */
        uint8_t bytes[frameLength];
    };

    /** Raw iris code, signed zoom amplitude and raw stabilisation offsets. */
    struct Controls
    {
        /** Raw iris command; 0xff closes the tested lens, without an F-number scale. */
        uint8_t iris = 0xff;

        /** Signed encoded zoom amplitude, limited to -15..15; zero stops motion. */
        int8_t zoom = 0;

        /** Raw signed horizontal stabilisation offset, not an angular displacement. */
        int16_t x = 0;

        /** Raw signed vertical stabilisation offset, not an angular displacement. */
        int16_t y = 0;
    };

    /** Response integrity and header classification, independent of optical function. */
    enum class ResponseKind : uint8_t
    {
        None,
        Init,
        Normal,
        Zero,
        AllFf,
        BadChecksum,
        Unknown
    };

    /** @brief Reverses all eight bits for hardware that only shifts MSB first. */
    uint8_t reverseBits(uint8_t value);

    /** @brief Computes the ones-complement sum of the first 39 bytes. */
    uint8_t checksum(const Frame& frame);

    /** @brief Updates the final checksum byte in place. */
    void seal(Frame& frame);

    /** @brief Checks integrity without assigning a response type. */
    bool validChecksum(const Frame& frame);

    /** @brief Classifies zero/FF responses before checking integrity and header. */
    ResponseKind classify(const Frame& frame);

    /** @brief Returns the stable wire label used by XL and XLRAW telemetry. */
    const char* responseName(ResponseKind kind);

    /** @brief Validates zoom amplitude and raw stabilisation offsets. */
    bool validControls(const Controls& controls);

    /** @brief Encodes a signed amplitude; zero or out-of-range input returns zero. */
    uint8_t encodeZoom(int8_t zoom);

    /** @brief Copies the historical initialisation template and seals it. */
    void buildInit(Frame& frame);

    /**
     * @brief Builds a sealed command while preserving unassigned template fields.
     * @return False for invalid controls; the output frame is then unchanged.
     */
    bool buildCommand(const Controls& controls, Frame& frame);
} // namespace xl
