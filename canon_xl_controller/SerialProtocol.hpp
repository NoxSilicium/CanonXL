// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file SerialProtocol.hpp
 * @brief Defines bounded serial command parsing and nonblocking output storage.
 */
#pragma once

#include "LensProtocol.hpp"

namespace xl
{
    /** Supported wire commands; their serial spellings remain unchanged. */
    enum class CommandType : uint8_t
    {
        Set,
        Stop,
        Reset,
        Status,
        Stream,
        Raw
    };

    /** Parsed command; stream selects the enable flag for STREAM and RAW. */
    struct Command
    {
        /** Operation selected by the parsed wire command. */
        CommandType type = CommandType::Status;

        /** Requested raw controls for SET; ignored by other operations. */
        Controls controls;

        /** Enable flag for STREAM or RAW, interpreted according to type. */
        bool stream = true;
    };

    /** Parser outcome; None includes an incomplete or discarded line. */
    enum class LineResult : uint8_t
    {
        None,
        Command,
        Invalid
    };

    /** Incremental ASCII parser; callers supply a millisecond timer. */
    class LineParser
    {
        public:

        /**
         * @brief Consumes one byte of LF- or CRLF-terminated ASCII input.
         * @param byte Next serial byte; partial commands never reach the controller.
         * @param now Current millisecond timer reading, including rollover.
         * @param command Output changed only for LineResult::Command.
         * @return Command, Invalid once per bad line, or None while accumulating/discarding.
         */
        LineResult feed(uint8_t byte, uint32_t now, Command& command);

        /**
         * @brief Reports a timed-out partial line once, discarding until newline.
         * @param now Current millisecond timer reading.
         * @return True only on the first expiration of the current partial line.
         */
        bool expire(uint32_t now);

        /** @brief Discards partial input, including timeout and CR state. */
        void reset()
        {
            clear();
        }

        private:

        /** @brief Clears parsing state; callers decide whether a command/error is emitted. */
        void clear();

        /** Fixed ASCII line storage, including one byte reserved for the NUL terminator. */
        char lineBuffer[96] =
        {
        };

        /** Number of accumulated payload bytes, excluding the terminator. */
        size_t lengthBytes = 0;

        /** Last received byte's millisecond timestamp, used for partial-line expiration. */
        uint32_t lastByteAtMs = 0;

        /** Invalid lines remain quarantined until LF, including after a timeout. */
        bool isDiscarding = false;

        /** Prevents a second error when LF eventually terminates an expired line. */
        bool hasReportedTimeout = false;

        /** Only LF may follow an accepted CR. */
        bool hasCarriageReturn = false;
    };

    // Bounded output storage; congestion can never stall the SPI scheduler.
    /** Single-threaded ring buffer; dropped counts whole messages, not bytes. */
    class OutputQueue
    {
        public:

        /**
         * @brief Copies a whole message without blocking or allocating storage.
         * @param line NUL-terminated record; the terminator is not queued.
         * @return False when the whole record cannot fit; increments the drop counter.
         */
        bool enqueue(const char* line);

        /** @brief Returns the contiguous readable byte count from data(). */
        size_t contiguous() const;

        /** @return Borrowed readable storage; invalidated by queue mutation. */
        uint8_t* data()
        {
            return bytes + headIndex;
        }

        /**
         * @brief Removes bytes accepted by the serial endpoint.
         * @param count Requested removal count; clamped to the currently queued size.
         */
        void consume(size_t count);

        /** @brief Discards queued bytes while retaining the cumulative drop count. */
        void clear()
        {
            headIndex = 0;
            sizeBytes = 0;
        }

        /** @brief Returns the cumulative count of rejected complete messages. */
        uint32_t dropped() const
        {
            return droppedCount;
        }

        private:

        /** Fixed byte capacity; no allocation occurs in the serial scheduler. */
        static constexpr size_t capacityBytes = 512;

        /** Owned circular storage; data()/contiguous() expose only the readable region. */
        uint8_t bytes[capacityBytes] =
        {
        };

        /** Index of the first queued byte; wraps modulo capacityBytes. */
        size_t headIndex = 0;

        /** Total queued bytes, including bytes beyond the physical wrap point. */
        size_t sizeBytes = 0;

        /** Cumulative complete-message rejections, retained by clear(). */
        uint32_t droppedCount = 0;
    };
} // namespace xl
