// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/** Deterministic USB framing and host-lease tests; never open serial hardware. */
public final class ControllerTests
{
    private static int checks;

    private static void check(boolean condition, String message)
    {
        checks++;
        if (!condition)
        {
            throw new AssertionError(message);
        }
    }

    private static void rejects(Runnable action, String message)
    {
        boolean rejected = false;
        try
        {
            action.run();
        }
        catch (IllegalArgumentException expected)
        {
            rejected = true;
        }
        check(rejected, message);
    }

    /** Real stationary Nano/20x shape, with a variable unsigned board clock. */
    private static String state(long deviceMs)
    {
        return "XL,1," + deviceMs
            + ",ready,normal,255,0,0,0,255,32,208,4,119,15370,0,106,0,0,0,0,1000000,-1";
    }

    private static String replaceField(String line, int index, String value)
    {
        String[] fields = line.split(",", -1);
        fields[index] = value;
        return String.join(",", fields);
    }

    private static void protocolTests()
    {
        ControllerProtocol.State decoded =
            (ControllerProtocol.State)ControllerProtocol.parse(state(100));
        check(decoded.isReady(), "Ready normal response");
        check(decoded.irisFeedback == 255 && decoded.controls == 32, "Raw feedback bytes");
        check(decoded.zeroReplies == 106 && decoded.leaseAgeMs == -1,
            "Unsigned counters and lease");
        check(ControllerProtocol.set(112, -8, -5000, 5000).equals("SET,112,-8,-5000,5000"),
            "Atomic SET");
        check(ControllerProtocol.decodeZoom(33) == 8, "Positive encoded zoom");
        check(ControllerProtocol.decodeZoom(34) == -8, "Negative encoded zoom");
        check(ControllerProtocol.decodeZoom(0) == 0, "Stopped encoded zoom");
        check(ControllerProtocol.unsignedDelta(25, 0xfffffff0L) == 41, "Board clock wrap");
        check(((ControllerProtocol.State)ControllerProtocol.parse(state(0xffffffffL))).deviceMs
            == 0xffffffffL, "Unsigned maximum");

        rejects(() -> ControllerProtocol.set(256, 0, 0, 0), "Iris upper bound");
        rejects(() -> ControllerProtocol.set(-1, 0, 0, 0), "Iris lower bound");
        rejects(() -> ControllerProtocol.set(0, 16, 0, 0), "Zoom bound");
        rejects(() -> ControllerProtocol.set(0, 0, -5001, 0), "X bound");
        rejects(() -> ControllerProtocol.set(0, 0, 0, 5001), "Y bound");
        rejects(() -> ControllerProtocol.decodeZoom(1), "Zero amplitude direction");
        rejects(() -> ControllerProtocol.decodeZoom(63), "Invalid direction bits");
        rejects(() -> ControllerProtocol.parse(state(100) + ",0"), "Extra fields");
        rejects(() -> ControllerProtocol.parse("XL,1,100"), "Incomplete state");
        rejects(() -> ControllerProtocol.parse(replaceField(state(100), 1, "2")),
            "Unsupported version");
        rejects(() -> ControllerProtocol.parse(replaceField(state(100), 5, "1.6")),
            "Legacy F-number");
        rejects(() -> ControllerProtocol.parse(replaceField(state(100), 2, "4294967296")),
            "Clock overflow");
        rejects(() -> ControllerProtocol.parse(replaceField(state(100), 14, "-1")),
            "Negative counter");
        rejects(() -> ControllerProtocol.parse(replaceField(state(100), 9, "-1")),
            "Missing normal feedback");
        rejects(() -> ControllerProtocol.parse(replaceField(state(100), 4, "zero")),
            "Stale fault feedback");
        rejects(() -> ControllerProtocol.parse(state(100) + "\u0000"), "Control character");
        rejects(() -> ControllerProtocol.parse("ACK,FOCUS"), "Unknown ACK");
        check(ControllerProtocol.parse("ERR,not-ready").equals("ERR,not-ready"), "Error token");

        String fault = replaceField(state(100), 3, "init");
        fault = replaceField(fault, 4, "zero");
        for (int index = 9; index <= 13; index++)
        {
            fault = replaceField(fault, index, "-1");
        }
        check(!((ControllerProtocol.State)ControllerProtocol.parse(fault)).isReady(),
            "Fault clears feedback");
        String raw = "XLRAW,1,100,10,ready,normal,1,02" + "00".repeat(38) + "FD";
        ControllerProtocol.RawFrame frame =
            (ControllerProtocol.RawFrame)ControllerProtocol.parse(raw);
        check(frame.checksumValid && frame.hex.length() == 80, "Raw 40-byte checksum");
        rejects(() -> ControllerProtocol.parse(raw.substring(0, raw.length() - 2)),
            "Short raw hex");
        rejects(() -> ControllerProtocol.parse(raw.replace("FD", "FE")),
            "False checksum claim");
        rejects(() -> ControllerProtocol.parse(raw.replace("FD", "fd")),
            "Lowercase raw hex");
        rejects(() -> ControllerProtocol.parse(raw.replace("normal", "init")),
            "Raw header mismatch");
        ControllerProtocol.RawFrame bad = (ControllerProtocol.RawFrame)ControllerProtocol.parse(
            "XLRAW,1,100,10,init,zero,0," + "00".repeat(40));
        check(!bad.checksumValid, "Invalid raw replies preserved");
    }

    private static void framingTests()
    {
        ControllerProtocol.Framer framer = new ControllerProtocol.Framer();
        List<String> received = new ArrayList<>();
        String line = state(100);
        for (byte value : (line + "\r\n").getBytes(StandardCharsets.US_ASCII))
        {
            framer.feed(new byte[]
            {
                value
            }, received::add);
        }
        check(received.equals(List.of(line)), "Every-byte fragmentation and CRLF");
        received.clear();
        framer.feed(("\nACK,SET\n" + state(200) + "\n").getBytes(StandardCharsets.US_ASCII),
            received::add);
        check(received.size() == 2, "Combined lines and empty line");
        received.clear();
        framer.feed(("X".repeat(600) + "\nACK,STOP\n").getBytes(StandardCharsets.US_ASCII),
            received::add);
        check(received.equals(List.of("ACK,STOP")) && framer.rejectedLines == 1,
            "Oversize resynchronization");
        received.clear();
        framer.feed(new byte[]
        {
            (byte)0xff, 'X', 'L', '\n', 'A', 'C', 'K', ',', 'R', 'A', 'W', '\n'
        }, received::add);
        check(received.equals(List.of("ACK,RAW")) && framer.rejectedLines == 2,
            "Binary noise discarded");
    }

    private static void sessionTests()
    {
        List<String> sent = new ArrayList<>();
        ControllerSession session = new ControllerSession(sent::add);
        session.open(0);
        check(sent.isEmpty() && !session.arm(0), "Opening cannot actuate");
        session.receive("garbage", 0);
        session.receive(state(100), 0);
        check(session.rejectedLines == 1 && session.canArm(0), "Valid state after noise");
        check(session.arm(0) && sent.isEmpty(), "Arming adopts iris without SET");
        session.tick(100, true);
        check(sent.isEmpty(), "Armed idle sends no SET");
        session.setControls(112, -8, 0, 0, 100);
        check(sent.equals(List.of("SET,112,-8,0,0")), "Explicit action");
        session.receive(state(200), 150);
        session.tick(199, true);
        check(sent.size() == 1, "No early renewal");
        session.tick(200, true);
        check(sent.size() == 2, "100 ms renewal");
        session.releaseZoom(210);
        check(sent.get(2).equals("STOP") && session.iris == 112 && session.zoom == 0,
            "Release retains iris");
        session.tick(300, true);
        check(sent.size() == 3, "No motion after release");
        session.setControls(112, 8, 50, -50, 310);
        session.releaseZoom(320);
        check(sent.get(sent.size() - 1).equals("SET,112,0,50,-50"),
            "Release preserves explicit XY");
        session.tick(330, false);
        check(!session.armed && session.x == 0 && session.y == 0
            && sent.get(sent.size() - 1).equals("STOP"), "Focus loss stops all owned motion");

        session.receive(state(500), 500);
        check(session.arm(500), "Explicit rearming");
        session.setControls(112, 8, 0, 0, 500);
        int count = sent.size();
        session.receive("ACK,SET", 900);
        session.tick(901, true);
        check(!session.armed && sent.size() == count + 1, "ACK cannot keep stale control alive");
        session.receive(state(900), 902);
        session.tick(1000, true);
        check(!session.armed && session.zoom == 0 && sent.size() == count + 1, "No auto-resume");
        session.arm(1000);
        session.setControls(112, 8, 0, 0, 1000);
        session.receive(state(1000), 1399);
        check(!session.armed, "A telemetry gap disarms even before scheduling");

        session.arm(1399);
        session.setControls(112, 8, 0, 0, 1399);
        session.receive(state(10), 1400);
        check(!session.armed, "Device restart disarms");
        session.receive(state(0xfffffff0L), 1500);
        session.arm(1500);
        session.receive(state(25), 1550);
        check(session.armed, "Unsigned wrap is not a restart");
        session.setControls(112, 8, 0, 0, 1550);
        session.receive("ERR,not-ready", 1560);
        check(!session.armed && session.lastMessage.equals("ERR,not-ready"),
            "Command error disarms");

        session.receive(replaceField(state(100), 6, "33"), 1600);
        check(!session.canArm(1600), "Cannot seize existing motion");
        session.receive(state(110), 1610);
        session.arm(1610);
        int previousIris = session.iris;
        rejects(() -> session.setControls(300, 0, 0, 0, 1610),
            "Invalid SET rejected atomically");
        check(session.iris == previousIris, "Invalid SET does not update requests");
        session.setRaw(true);
        check(sent.get(sent.size() - 1).equals("RAW,1"), "Raw opt-in");
        session.close();
        check(!session.connected && session.state == null && !session.armed,
            "Close clears display");
        count = sent.size();
        session.open(2000);
        session.close();
        check(sent.size() == count, "Observation disconnect does not STOP");

        ControllerSession broken = new ControllerSession(line ->
        {
            if (line.equals("STOP"))
            {
                throw new IllegalStateException("Unplugged");
            }
        });
        broken.open(0);
        broken.receive(state(0), 0);
        broken.arm(0);
        broken.setControls(112, 8, 0, 0, 0);
        try
        {
            broken.close();
        }
        catch (IllegalStateException expected)
        {
            check(!broken.connected && !broken.armed, "Failed STOP cannot retain ownership");
        }
    }

    public static void main(String[] args)
    {
        protocolTests();
        framingTests();
        sessionTests();
        System.out.println("PASS " + checks + " checks; no hardware accessed");
    }
}
