// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * Explicit finite CLI bench sequence, never entered from normal GUI startup.
 * Timed scripted movements do not depend on mouse/window focus; stale state still stops them.
 */
boolean benchRequested;
boolean benchRunning;
boolean benchFinished;
int benchStage;
long benchNextMs;

void runBench(long nowMs)
{
    if (!benchRequested || benchFinished || !session.connected)
    {
        return;
    }
    if (!benchRunning)
    {
        if (!session.canArm(nowMs))
        {
            return;
        }
        session.arm(nowMs);
        benchRunning = true;
        benchStage = 0;
        benchNextMs = nowMs;
        note("BENCH begin: iris, two short zoom pulses, X/Y offsets, close iris");
    }
    if (!session.armed || !session.isFresh(nowMs) || !session.state.isReady())
    {
        cancelBench();
        return;
    }
    if (nowMs < benchNextMs)
    {
        return;
    }

    switch (benchStage)
    {
        case 0:
            session.setControls(112, 0, 0, 0, nowMs);
            note("BENCH iris 0x70");
            benchNextMs = nowMs + 1500;
            break;
        case 1:
            session.setControls(112, 2, 0, 0, nowMs);
            note("BENCH zoom +2, 250 ms");
            benchNextMs = nowMs + 250;
            break;
        case 2:
            session.releaseZoom(nowMs);
            benchNextMs = nowMs + 400;
            break;
        case 3:
            session.setControls(112, -2, 0, 0, nowMs);
            note("BENCH zoom -2, 250 ms");
            benchNextMs = nowMs + 250;
            break;
        case 4:
            session.releaseZoom(nowMs);
            benchNextMs = nowMs + 400;
            break;
        case 5:
            session.setControls(112, 0, 1000, 0, nowMs);
            note("BENCH X +1000, 350 ms");
            benchNextMs = nowMs + 350;
            break;
        case 6:
            session.setControls(112, 0, 0, 1000, nowMs);
            note("BENCH Y +1000, 350 ms");
            benchNextMs = nowMs + 350;
            break;
        case 7:
            session.setControls(255, 0, 0, 0, nowMs);
            irisField.setText("255");
            irisSlider.setValue(255);
            note("BENCH iris 0xFF, zoom/X/Y zero");
            benchNextMs = nowMs + 500;
            break;
        default:
            session.disarm();
            benchRunning = false;
            benchFinished = true;
            note("BENCH complete; observation only");
            break;
    }
    benchStage++;
}

/** User STOP cancels the sequence. Restore a closed iris only with fresh, ready state. */
void cancelBench()
{
    benchFinished = true;
    benchRunning = false;
    if (session.armed && session.isFresh(hostMs()) && session.state.isReady())
    {
        session.setControls(255, 0, 0, 0, hostMs());
    }
    session.disarm();
    note("BENCH cancelled; check physical iris if transport was lost");
}
