// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file LensController.hpp
 * @brief Exposes the nonblocking lens state machine and wrap-safe scheduling.
 */
#pragma once

#include "LensProtocol.hpp"

namespace xl
{
    /** Current dialogue phase; Ready requires a normal, valid response. */
    enum class Phase : uint8_t
    {
        Initialising,
        Probing,
        Ready
    };

    /** @brief Returns the stable phase label used by serial telemetry. */
    const char* phaseName(Phase phase);

    // Unsigned elapsed-time arithmetic keeps scheduling valid across timer rollover.
    /** Periodic deadline tracker; callers use the same timer unit throughout. */
    class Periodic
    {
        public:

        /** @param period Nonzero interval in the caller's timer units. */
        explicit Periodic(uint32_t period) : period(period)
        {
        }

        /** @brief Starts scheduling from the given timer reading. */
        void start(uint32_t now)
        {
            lastAt = now;
            skippedCount = 0;
        }

        /** @brief Advances to the latest due slot and counts skipped earlier slots. */
        bool due(uint32_t now);

        /** @brief Returns the cumulative count of skipped slots. */
        uint32_t skipped() const
        {
            return skippedCount;
        }

        private:

        /** Nonzero scheduling interval in the timer unit chosen by the caller. */
        uint32_t period;

        /** Last scheduled slot's timer reading, not the last loop invocation. */
        uint32_t lastAt = 0;

        /** Slots deliberately skipped when a caller services a late deadline. */
        uint32_t skippedCount = 0;
    };

    /** Cumulative dialogue counters, retained when the state machine restarts. */
    struct Statistics
    {
        /** Total complete SPI transactions received by the state machine. */
        uint32_t exchanges = 0;

        /** Responses rejected by checksum validation. */
        uint32_t badChecksums = 0;

        /** Complete all-zero responses, including possible startup replies. */
        uint32_t zero = 0;

        /** Complete all-FF responses. */
        uint32_t allFf = 0;

        /** Checksum-valid responses with an unrecognised header. */
        uint32_t unknown = 0;
    };

    /** Single-threaded controller; all methods must run in the Arduino loop context. */
    class LensController
    {
        public:

        /** @brief Returns the current initialisation, probing or ready phase. */
        Phase phase() const
        {
            return currentPhase;
        }

        /** @brief Returns the last response classification, or None before reception. */
        ResponseKind responseKind() const
        {
            return lastResponseKind;
        }

        /** @return Borrowed last response, valid until the next receive or reset. */
        const Frame& response() const
        {
            return lastResponse;
        }

        /** @return Borrowed active controls, valid until the next state change. */
        const Controls& controls() const
        {
            return activeControls;
        }

        /** @return Borrowed cumulative counters owned by this controller. */
        const Statistics& statistics() const
        {
            return counters;
        }

        /** @brief Reports whether an accepted SET lease is still active. */
        bool hasCommand() const
        {
            return hasActiveCommand;
        }

        /** @brief Returns unsigned elapsed milliseconds since the last accepted SET. */
        uint32_t commandAge(uint32_t now) const
        {
            return now - commandAtMs;
        }

        /** @return True only when ready with valid controls; renews the motion lease. */
        bool set(const Controls& controls, uint32_t now);

        /** @brief Clears zoom/X/Y and the lease without changing the iris code. */
        void stop();

        /** @brief Restarts dialogue and clears the last response, retaining counters. */
        void reset();

        /** @brief Expires the motion lease using a millisecond timer reading. */
        void tick(uint32_t now);

        /** @brief Writes the next sealed outgoing frame without advancing dialogue. */
        void nextFrame(Frame& frame) const;

        /** @brief Records a received frame and updates counters and dialogue state. */
        void receive(const Frame& frame);

        private:

        /** @brief Resets controls and dialogue while preserving response evidence. */
        void restart();

        /** Latest accepted raw controls; recovery restores default closed-iris controls. */
        Controls activeControls;

        /** Complete last SPI response, retained as evidence even when invalid. */
        Frame lastResponse =
        {
        };

        /** Cumulative counts retained across reset/recovery, not per-phase counters. */
        Statistics counters;

        /** Ready is entered only after a checksum-valid normal response. */
        Phase currentPhase = Phase::Initialising;

        /** Classification of lastResponse, including uniform or corrupt replies. */
        ResponseKind lastResponseKind = ResponseKind::None;

        /** Millisecond reading of the last accepted SET, used for motion-lease expiry. */
        uint32_t commandAtMs = 0;

        /** Initialisation replies seen during the current probing phase. */
        uint16_t probeCount = 0;

        /** Consecutive bad-checksum or unknown replies since the last accepted response. */
        uint8_t badCount = 0;

        /** True while a SET lease is active; stop/reset/recovery clear it. */
        bool hasActiveCommand = false;
    };
} // namespace xl
