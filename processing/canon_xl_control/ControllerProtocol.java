// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

import java.util.function.Consumer;

/** Strict version-1 USB decoding, independent of Processing and serial hardware. */
public final class ControllerProtocol
{
    private ControllerProtocol()
    {
    }

    /** Immutable state; feedback is raw, and request fields are not measurements. */
    public static final class State
    {
        public final long deviceMs;
        public final String phase;
        public final String reply;
        public final int iris;
        public final int encodedZoom;
        public final int x;
        public final int y;
        public final int irisFeedback;
        public final int controls;
        public final int byte5;
        public final int byte16;
        public final int byte17;
        public final long exchanges;
        public final long badChecksums;
        public final long zeroReplies;
        public final long ffReplies;
        public final long unknownReplies;
        public final long skippedPeriods;
        public final long droppedOutput;
        public final long spiHz;
        public final long leaseAgeMs;

        private State(String[] fields)
        {
            deviceMs = unsigned(fields[2]);
            phase = phase(fields[3]);
            reply = reply(fields[4], true);
            iris = integer(fields[5], 0, 255);
            encodedZoom = integer(fields[6], 0, 255);
            decodeZoom(encodedZoom);
            x = integer(fields[7], -5000, 5000);
            y = integer(fields[8], -5000, 5000);
            irisFeedback = integer(fields[9], -1, 255);
            controls = integer(fields[10], -1, 255);
            byte5 = integer(fields[11], -1, 255);
            byte16 = integer(fields[12], -1, 255);
            byte17 = integer(fields[13], -1, 255);
            exchanges = unsigned(fields[14]);
            badChecksums = unsigned(fields[15]);
            zeroReplies = unsigned(fields[16]);
            ffReplies = unsigned(fields[17]);
            unknownReplies = unsigned(fields[18]);
            skippedPeriods = unsigned(fields[19]);
            droppedOutput = unsigned(fields[20]);
            spiHz = unsigned(fields[21]);
            leaseAgeMs = fields[22].equals("-1") ? -1 : unsigned(fields[22]);

            boolean hasFeedback = irisFeedback >= 0 && controls >= 0 && byte5 >= 0
                && byte16 >= 0 && byte17 >= 0;
            boolean noFeedback = irisFeedback == -1 && controls == -1 && byte5 == -1
                && byte16 == -1 && byte17 == -1;

            if (reply.equals("normal") ? !hasFeedback : !noFeedback)
            {
                throw new IllegalArgumentException("Inconsistent feedback fields");
            }
        }

        /** A valid normal lens response, not proof of physical actuator movement. */
        public boolean isReady()
        {
            return phase.equals("ready") && reply.equals("normal");
        }
    }

    /** Full response sample. Corrupt replies are retained, never interpreted as valid. */
    public static final class RawFrame
    {
        public final long deviceMs;
        public final long exchanges;
        public final String phase;
        public final String reply;
        public final boolean checksumValid;
        public final String hex;

        private RawFrame(String[] fields)
        {
            deviceMs = unsigned(fields[2]);
            exchanges = unsigned(fields[3]);
            phase = phase(fields[4]);
            reply = reply(fields[5], false);
            checksumValid = integer(fields[6], 0, 1) == 1;
            hex = fields[7];

            if (!hex.matches("[0-9A-F]{80}"))
            {
                throw new IllegalArgumentException("Expected 40 hex bytes");
            }

            int sum = 0;
            for (int index = 0; index < 80; index += 2)
            {
                sum += Integer.parseInt(hex.substring(index, index + 2), 16);
            }

            if (checksumValid != ((sum & 255) == 255))
            {
                throw new IllegalArgumentException("Incorrect raw checksum flag");
            }

            if ((reply.equals("normal") && (!checksumValid || !hex.startsWith("02")))
                || (reply.equals("init") && (!checksumValid || !hex.startsWith("01"))))
            {
                throw new IllegalArgumentException("Incorrect raw response type");
            }
        }
    }

    /** Parse one complete line. Unknown versions, malformed values and shapes throw. */
    public static Object parse(String line)
    {
        if (line.length() > 512 || !line.matches("[ -~]+"))
        {
            throw new IllegalArgumentException("Non-ASCII or oversized line");
        }

        String[] fields = line.split(",", -1);
        if (fields.length == 23 && fields[0].equals("XL") && fields[1].equals("1"))
        {
            return new State(fields);
        }
        if (fields.length == 8 && fields[0].equals("XLRAW") && fields[1].equals("1"))
        {
            return new RawFrame(fields);
        }
        if (fields.length == 2 && fields[0].equals("ACK")
            && fields[1].matches("SET|STOP|RESET|STREAM|RAW"))
        {
            return line;
        }
        if (fields.length == 2 && fields[0].equals("ERR")
            && fields[1].matches("[a-z-]+"))
        {
            return line;
        }
        throw new IllegalArgumentException("Unknown message or field count");
    }

    /** Validate every command field before constructing an atomic SET line. */
    public static String set(int iris, int zoom, int x, int y)
    {
        range(iris, 0, 255);
        range(zoom, -15, 15);
        range(x, -5000, 5000);
        range(y, -5000, 5000);
        return "SET," + iris + "," + zoom + "," + x + "," + y;
    }

    /** Decode firmware amplitude/direction bits; this is not a calibrated speed. */
    public static int decodeZoom(int encoded)
    {
        if (encoded == 0)
        {
            return 0;
        }
        int magnitude = encoded >> 2;
        int direction = encoded & 3;
        if (magnitude < 1 || magnitude > 15 || (direction != 1 && direction != 2))
        {
            throw new IllegalArgumentException("Invalid encoded zoom");
        }
        return direction == 1 ? magnitude : -magnitude;
    }

    /** Forward unsigned-32 delta, including wrap. Large deltas signal regression. */
    public static long unsignedDelta(long current, long previous)
    {
        return (current - previous) & 0xffffffffL;
    }

    private static long unsigned(String value)
    {
        if (!value.matches("[0-9]{1,10}"))
        {
            throw new IllegalArgumentException("Expected unsigned decimal");
        }
        long number = Long.parseLong(value);
        if (number > 0xffffffffL)
        {
            throw new IllegalArgumentException("Unsigned overflow");
        }
        return number;
    }

    private static int integer(String value, int minimum, int maximum)
    {
        if (!value.matches("-?[0-9]{1,6}"))
        {
            throw new IllegalArgumentException("Expected signed decimal");
        }
        int number = Integer.parseInt(value);
        range(number, minimum, maximum);
        return number;
    }

    private static void range(int value, int minimum, int maximum)
    {
        if (value < minimum || value > maximum)
        {
            throw new IllegalArgumentException("Out-of-range control");
        }
    }

    private static String phase(String value)
    {
        if (!value.matches("init|probe|ready"))
        {
            throw new IllegalArgumentException("Unknown phase");
        }
        return value;
    }

    private static String reply(String value, boolean allowNone)
    {
        if (!value.matches("init|normal|zero|ff|checksum|unknown")
            && !(allowNone && value.equals("none")))
        {
            throw new IllegalArgumentException("Unknown reply");
        }
        return value;
    }

    /** Bounded LF framer. An invalid/oversized line is discarded through its LF. */
    public static final class Framer
    {
        private final StringBuilder buffer = new StringBuilder();
        private boolean discarding;
        public long rejectedLines;

        /** Consume a byte fragment; callbacks receive complete lines without CR/LF. */
        public void feed(byte[] bytes, Consumer<String> receiver)
        {
            for (byte value : bytes)
            {
                int character = value & 255;
                if (character == '\n')
                {
                    if (!discarding && buffer.length() > 0)
                    {
                        String line = buffer.toString();
                        if (line.endsWith("\r"))
                        {
                            line = line.substring(0, line.length() - 1);
                        }
                        receiver.accept(line);
                    }
                    buffer.setLength(0);
                    discarding = false;
                }
                else if (!discarding)
                {
                    if ((character < 32 && character != '\r') || character > 126
                        || buffer.length() >= 512)
                    {
                        rejectedLines++;
                        discarding = true;
                        buffer.setLength(0);
                    }
                    else
                    {
                        buffer.append((char)character);
                    }
                }
            }
        }
    }
}
