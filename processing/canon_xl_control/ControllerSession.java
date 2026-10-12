// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

import java.util.function.Consumer;

/**
 * Host control ownership and lease scheduling. Confined to the animation thread.
 * All times are host monotonic milliseconds, independent of device clock wrap.
 */
public final class ControllerSession
{
    public static final long staleMs = 350;
    public static final long renewalMs = 100;
    private final Consumer<String> sender;
    public ControllerProtocol.State state;
    public ControllerProtocol.RawFrame raw;
    public long stateAtMs = -1;
    public long rawAtMs = -1;
    public long receivedStates;
    public long receivedRaw;
    public long rejectedLines;
    public String lastMessage = "";
    public boolean connected;
    public boolean armed;
    public int iris = 255;
    public int zoom;
    public int x;
    public int y;
    private boolean hasOwnedCommand;
    private long lastSetAtMs;
    private long lastStatusAtMs;

    /** Sender accepts a line without LF; transport errors must propagate. */
    public ControllerSession(Consumer<String> sender)
    {
        this.sender = sender;
    }

    /** Opening never sends SET, STOP or RESET; initial state must come from the board. */
    public void open(long nowMs)
    {
        connected = true;
        armed = false;
        hasOwnedCommand = false;
        state = null;
        raw = null;
        stateAtMs = -1;
        rawAtMs = -1;
        receivedStates = 0;
        receivedRaw = 0;
        rejectedLines = 0;
        iris = 255;
        zoom = 0;
        x = 0;
        y = 0;
        lastMessage = "";
        lastStatusAtMs = nowMs + 1000;
    }

    /** Complete validated messages only. A fault/reboot relinquishes control ownership. */
    public void receive(String line, long nowMs)
    {
        Object message;
        try
        {
            message = ControllerProtocol.parse(line);
        }
        catch (IllegalArgumentException error)
        {
            rejectedLines++;
            return;
        }
        if (!connected)
        {
            return;
        }
        if (message instanceof ControllerProtocol.State)
        {
            ControllerProtocol.State next = (ControllerProtocol.State)message;
            boolean regressed = state != null
                && ControllerProtocol.unsignedDelta(next.deviceMs, state.deviceMs) > 0x7fffffffL;
            if (regressed || !next.isReady() || !isFresh(nowMs) && state != null)
            {
                disarm();
            }
            state = next;
            stateAtMs = nowMs;
            receivedStates++;
            if (!armed)
            {
                iris = next.iris;
            }
        }
        else if (message instanceof ControllerProtocol.RawFrame)
        {
            raw = (ControllerProtocol.RawFrame)message;
            rawAtMs = nowMs;
            receivedRaw++;
        }
        else
        {
            lastMessage = (String)message;
            if (lastMessage.startsWith("ERR,"))
            {
                disarm();
            }
        }
    }

    /** A recent ready/normal state is required; ACK and raw traffic cannot refresh it. */
    public boolean isFresh(long nowMs)
    {
        return connected && state != null && nowMs >= stateAtMs
            && nowMs - stateAtMs <= staleMs;
    }

    public boolean canArm(long nowMs)
    {
        return isFresh(nowMs) && state.isReady() && state.encodedZoom == 0
            && state.x == 0 && state.y == 0;
    }

    /** Adopt only the existing iris request; arming alone does not change the lens. */
    public boolean arm(long nowMs)
    {
        if (!canArm(nowMs))
        {
            return false;
        }
        iris = state.iris;
        zoom = 0;
        x = 0;
        y = 0;
        armed = true;
        return true;
    }

    /** Zero owned motion while retaining iris. Reading an unrelated session never stops it. */
    public void disarm()
    {
        boolean mustStop = connected && hasOwnedCommand;
        armed = false;
        hasOwnedCommand = false;
        zoom = 0;
        x = 0;
        y = 0;
        if (mustStop)
        {
            sender.accept("STOP");
        }
    }

    /** Always invalidate ownership, even if an unplugged transport rejects STOP. */
    public void close()
    {
        try
        {
            disarm();
        }
        finally
        {
            connected = false;
            state = null;
            raw = null;
        }
    }

    /** Explicit user change only; validate atomically before updating local requests. */
    public boolean setControls(int iris, int zoom, int x, int y, long nowMs)
    {
        String command = ControllerProtocol.set(iris, zoom, x, y);
        if (!armed || !isFresh(nowMs) || !state.isReady())
        {
            disarm();
            return false;
        }
        sender.accept(command);
        this.iris = iris;
        this.zoom = zoom;
        this.x = x;
        this.y = y;
        hasOwnedCommand = true;
        lastSetAtMs = nowMs;
        return true;
    }

    /** Release zoom; retain explicitly enabled XY offsets, otherwise send STOP. */
    public void releaseZoom(long nowMs)
    {
        if (zoom == 0)
        {
            return;
        }
        zoom = 0;
        if (x != 0 || y != 0)
        {
            setControls(iris, 0, x, y, nowMs);
        }
        else if (hasOwnedCommand)
        {
            hasOwnedCommand = false;
            sender.accept("STOP");
        }
    }

    /** Guard before consuming queued input; a host stall cannot revive expired ownership. */
    public void checkSafety(long nowMs, boolean windowFocused)
    {
        if (!connected)
        {
            return;
        }
        boolean motion = zoom != 0 || x != 0 || y != 0;
        if (armed && (!windowFocused || !isFresh(nowMs)
            || motion && nowMs - lastSetAtMs >= 400))
        {
            disarm();
        }
    }

    /** Schedule after polling. Frame scheduling cannot restart disarmed motion. */
    public void tick(long nowMs, boolean windowFocused)
    {
        if (!connected)
        {
            return;
        }
        checkSafety(nowMs, windowFocused);
        boolean motion = zoom != 0 || x != 0 || y != 0;
        if (armed && motion && nowMs - lastSetAtMs >= renewalMs)
        {
            setControls(iris, zoom, x, y, nowMs);
        }
        if (!isFresh(nowMs) && nowMs - lastStatusAtMs >= 1000)
        {
            sender.accept("STATUS");
            lastStatusAtMs = nowMs;
        }
    }

    /** Raw sampling changes no actuator request. The caller restores it when disconnecting. */
    public void setRaw(boolean enabled)
    {
        if (connected)
        {
            sender.accept(enabled ? "RAW,1" : "RAW,0");
        }
        if (!enabled)
        {
            raw = null;
            rawAtMs = -1;
        }
    }
}
