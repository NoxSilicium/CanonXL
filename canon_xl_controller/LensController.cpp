// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file LensController.cpp
 * @brief Manages initialization, validated feedback, recovery and motion expiry.
 */
#include "LensController.hpp"

#include "Config.hpp"

namespace xl
{
    const char* phaseName(Phase phase)
    {
        switch (phase)
        {
            case Phase::Initialising:
                return "init";
            case Phase::Probing:
                return "probe";
            default:
                return "ready";
        }
    }

    bool Periodic::due(uint32_t now)
    {
        const uint32_t elapsed = now - lastAt;

        if (elapsed < period)
        {
            return false;
        }
        // Skip missed slots instead of issuing a burst of overdue SPI exchanges.
        const uint32_t slots = elapsed / period;
        lastAt += slots * period;
        skippedCount += slots - 1;

        return true;
    }

    bool LensController::set(const Controls& controls, uint32_t now)
    {
        if (currentPhase != Phase::Ready || !validControls(controls))
        {
            return false;
        }

        activeControls = controls;
        commandAtMs = now;
        hasActiveCommand = true;

        return true;
    }

    void LensController::stop()
    {
        // Stop motion without changing the last requested iris opening.
        activeControls.zoom = 0;
        activeControls.x = 0;
        activeControls.y = 0;
        hasActiveCommand = false;
    }

    void LensController::restart()
    {
        activeControls = Controls();
        currentPhase = Phase::Initialising;
        hasActiveCommand = false;
        probeCount = 0;
        badCount = 0;
    }

    void LensController::reset()
    {
        restart();
        lastResponseKind = ResponseKind::None;
        lastResponse = Frame();
    }

    void LensController::tick(uint32_t now)
    {
        if (hasActiveCommand && commandAge(now) >= config::commandTimeoutMs)
        {
            stop();
        }
    }

    void LensController::nextFrame(Frame& frame) const
    {
        if (currentPhase == Phase::Initialising)
        {
            buildInit(frame);
        }
        else
        {
            buildCommand(activeControls, frame);
        }
    }

    void LensController::receive(const Frame& frame)
    {
        lastResponse = frame;
        lastResponseKind = classify(frame);
        ++counters.exchanges;

        // Uniform replies do not establish readiness; restart the dialogue.
        if (lastResponseKind == ResponseKind::Zero || lastResponseKind == ResponseKind::AllFf)
        {
            if (lastResponseKind == ResponseKind::Zero)
            {
                ++counters.zero;
            }
            else
            {
                ++counters.allFf;
            }
            restart();

            return;
        }

        // Invalid feedback cancels motion immediately, before any retry threshold.
        if (lastResponseKind == ResponseKind::BadChecksum ||
            lastResponseKind == ResponseKind::Unknown)
        {
            if (lastResponseKind == ResponseKind::BadChecksum)
            {
                ++counters.badChecksums;
            }
            else
            {
                ++counters.unknown;
            }
            stop();

            if (currentPhase == Phase::Ready)
            {
                currentPhase = Phase::Probing;
            }

            if (++badCount >= config::badResponseLimit)
            {
                restart();
            }

            return;
        }

        // Only a checksum-valid normal response enables subsequent SET commands.
        badCount = 0;
        if (lastResponseKind == ResponseKind::Normal)
        {
            currentPhase = Phase::Ready;
            probeCount = 0;
        }
        else
        {
            if (currentPhase != Phase::Probing)
            {
                stop();
                currentPhase = Phase::Probing;
                probeCount = 0;
            }

            if (++probeCount >= config::probeLimit)
            {
                restart();
            }
        }
    }
} // namespace xl
