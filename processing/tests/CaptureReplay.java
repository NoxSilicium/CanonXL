// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/** Replay a recorded panel session with the production parser, without hardware access. */
public final class CaptureReplay
{
    public static void main(String[] args) throws Exception
    {
        Map<Long, ControllerProtocol.State> states = new HashMap<>();
        Map<Long, ControllerProtocol.RawFrame> rawFrames = new HashMap<>();
        Set<Integer> irisRequests = new TreeSet<>();
        Set<Integer> irisFeedback = new TreeSet<>();
        Set<Integer> zoomRequests = new TreeSet<>();
        Set<Integer> xRequests = new TreeSet<>();
        Set<Integer> yRequests = new TreeSet<>();
        Set<String> ringFeedback = new HashSet<>();
        ControllerProtocol.State last = null;
        boolean completed = false;
        int errors = 0;
        int matched = 0;

        for (String line : Files.readAllLines(Path.of(args[0])))
        {
            String[] record = line.split("\t", 3);
            if (record.length != 3)
            {
                throw new AssertionError("Malformed capture record");
            }
            if (record[1].equals("NOTE") && record[2].contains("BENCH complete"))
            {
                completed = true;
            }
            if (!record[1].equals("IN"))
            {
                continue;
            }
            Object message = ControllerProtocol.parse(record[2]);
            if (message instanceof ControllerProtocol.State)
            {
                last = (ControllerProtocol.State)message;
                states.put(last.exchanges, last);
                irisRequests.add(last.iris);
                irisFeedback.add(last.irisFeedback);
                zoomRequests.add(ControllerProtocol.decodeZoom(last.encodedZoom));
                xRequests.add(last.x);
                yRequests.add(last.y);
                ringFeedback.add(last.byte5 + "/" + last.byte16 + "/" + last.byte17);
            }
            else if (message instanceof ControllerProtocol.RawFrame)
            {
                ControllerProtocol.RawFrame raw = (ControllerProtocol.RawFrame)message;
                rawFrames.put(raw.exchanges, raw);
                if (!raw.checksumValid)
                {
                    errors++;
                }
            }
            else if (((String)message).startsWith("ERR,"))
            {
                errors++;
            }
        }
        for (ControllerProtocol.RawFrame raw : rawFrames.values())
        {
            ControllerProtocol.State state = states.get(raw.exchanges);
            if (state == null)
            {
                continue;
            }
            if (!raw.phase.equals(state.phase) || !raw.reply.equals(state.reply)
                || state.irisFeedback != byteAt(raw, 2)
                || state.controls != byteAt(raw, 4)
                || state.byte5 != byteAt(raw, 5)
                || state.byte16 != byteAt(raw, 16)
                || state.byte17 != byteAt(raw, 17))
            {
                throw new AssertionError("XL/XLRAW mismatch for exchange " + raw.exchanges);
            }
            matched++;
        }
        if (!completed || last == null || !last.isReady() || last.iris != 255
            || last.encodedZoom != 0 || last.x != 0 || last.y != 0 || errors != 0)
        {
            throw new AssertionError("Bench did not end cleanly");
        }
        if (!irisRequests.containsAll(Set.of(112, 255))
            || !zoomRequests.containsAll(Set.of(-2, 0, 2))
            || !xRequests.containsAll(Set.of(0, 1000)) || !yRequests.containsAll(Set.of(0, 1000))
            || matched < 20)
        {
            throw new AssertionError("Missing tested requests or paired replies");
        }
        System.out.println("PASS capture: " + states.size() + " states, " + rawFrames.size()
            + " raw frames, " + matched + " matching XL/XLRAW pairs");
        System.out.println("Iris requests " + irisRequests + "; raw MISO2 " + irisFeedback);
        System.out.println("Zoom requests " + zoomRequests
            + "; MISO5/16/17 variants " + ringFeedback.size());
        System.out.println("X requests " + xRequests + "; Y requests " + yRequests);
        System.out.println("Final request: iris 255, zoom/X/Y 0; no physical calibration inferred");
    }

    /** Bus byte numbers are one-based; hex string offsets are zero-based. */
    private static int byteAt(ControllerProtocol.RawFrame raw, int busByte)
    {
        int offset = (busByte - 1) * 2;
        return Integer.parseInt(raw.hex.substring(offset, offset + 2), 16);
    }
}
