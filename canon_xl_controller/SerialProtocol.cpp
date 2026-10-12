// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file SerialProtocol.cpp
 * @brief Parses complete serial commands and buffers bounded nonblocking output.
 */
#include "SerialProtocol.hpp"

#include "Config.hpp"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace xl
{
    namespace
    {
        /**
         * @brief Removes surrounding ASCII spaces and tabs from a mutable field.
         * @param value NUL-terminated storage owned by the current input line.
         * @return Borrowed pointer into value; trailing whitespace is overwritten.
         */
        char* trim(char* value)
        {
            while (*value == ' ' || *value == '\t')
            {
                ++value;
            }

            char* end = value + strlen(value);

            while (end > value && (end[-1] == ' ' || end[-1] == '\t'))
            {
                --end;
            }

            *end = '\0';

            return value;
        }

        /**
         * @brief Parses one complete integer field within inclusive bounds.
         * @param text Trimmed NUL-terminated field; fractional values are rejected.
         * @param min Inclusive lower bound.
         * @param max Inclusive upper bound.
         * @param value Output changed only on successful validation.
         * @param hex Accept an explicit 0x prefix in addition to decimal notation.
         * @return False for empty, overflowing, trailing-text or out-of-range input.
         */
        bool number(const char* text, long min, long max, long& value, bool hex = false)
        {
            if (!*text)
            {
                return false;
            }

            char* end = nullptr;
            const int base = hex && text[0] == '0' && (text[1] == 'x' || text[1] == 'X') ? 16 : 10;
            errno = 0;
            const long parsed = strtol(text, &end, base);

            // Consuming the entire field rejects inputs such as "5.6" or "1e2".
            if (errno == ERANGE || end == text || *end || parsed < min || parsed > max)
            {
                return false;
            }

            value = parsed;

            return true;
        }

        /**
         * @brief Decodes a complete serial line without applying it to the lens.
         * @param line Mutable NUL-terminated input; commas become field terminators.
         * @param output Parsed command, unchanged when any field fails validation.
         * @return True for an exact supported command name and field count.
         */
        bool parse(char* line, Command& output)
        {
            // Split the caller-owned line in place; empty or excess fields are rejected.
            char* fields[5];
            size_t count = 1;
            fields[0] = line;

            for (char* p = line; *p; ++p)
            {
                if (*p != ',')
                {
                    continue;
                }

                if (count == 5)
                {
                    return false;
                }

                *p = '\0';
                fields[count++] = p + 1;
            }

            for (size_t i = 0; i < count; ++i)
            {
                fields[i] = trim(fields[i]);
                if (!*fields[i])
                {
                    return false;
                }
            }

            // Publish the command only after every field passes validation.
            Command command;

            if (strcmp(fields[0], "SET") == 0 && count == 5)
            {
                long iris, zoom, x, y;

                // Validate in wide signed storage before narrowing to protocol-sized fields.
                if (!number(fields[1], 0, 255, iris, true) || !number(fields[2], -15, 15, zoom) ||
                    !number(
                        fields[3], -config::stabilisationLimit, config::stabilisationLimit, x) ||
                    !number(fields[4], -config::stabilisationLimit, config::stabilisationLimit, y))
                {
                    return false;
                }

                command.type = CommandType::Set;
                command.controls.iris = static_cast<uint8_t>(iris);
                command.controls.zoom = static_cast<int8_t>(zoom);
                command.controls.x = static_cast<int16_t>(x);
                command.controls.y = static_cast<int16_t>(y);
            }
            else if ((strcmp(fields[0], "STREAM") == 0 || strcmp(fields[0], "RAW") == 0) &&
                count == 2)
            {
                long stream;

                if (!number(fields[1], 0, 1, stream))
                {
                    return false;
                }
                command.type =
                    strcmp(fields[0], "RAW") == 0 ? CommandType::Raw : CommandType::Stream;
                command.stream = stream == 1;
            }
            else if (count == 1 && strcmp(fields[0], "STOP") == 0)
            {
                command.type = CommandType::Stop;
            }
            else if (count == 1 && strcmp(fields[0], "RESET") == 0)
            {
                command.type = CommandType::Reset;
            }
            else if (count == 1 && strcmp(fields[0], "STATUS") == 0)
            {
                command.type = CommandType::Status;
            }
            else
            {
                return false;
            }

            output = command;

            return true;
        }
    } // namespace

    void LineParser::clear()
    {
        lengthBytes = 0;
        isDiscarding = false;
        hasReportedTimeout = false;
        hasCarriageReturn = false;
    }

    LineResult LineParser::feed(uint8_t byte, uint32_t now, Command& command)
    {
        lastByteAtMs = now;

        if (byte == '\n')
        {
            // Timeout has already emitted its error; do not report it again at newline.
            if (isDiscarding)
            {
                const bool reported = hasReportedTimeout;
                clear();
                return reported ? LineResult::None : LineResult::Invalid;
            }

            if (!lengthBytes)
            {
                clear();
                return LineResult::None;
            }

            lineBuffer[lengthBytes] = '\0';
            const bool valid = parse(lineBuffer, command);
            clear();
            return valid ? LineResult::Command : LineResult::Invalid;
        }

        // A damaged or expired line stays quarantined until its newline arrives.
        if (isDiscarding)
        {
            return LineResult::None;
        }

        // Accept LF or CRLF, but no data or second CR after a carriage return.
        if (byte == '\r' && !hasCarriageReturn)
        {
            hasCarriageReturn = true;
            return LineResult::None;
        }

        // Retain one byte for the terminator and reject non-ASCII control data.
        if (hasCarriageReturn || (byte < 32 && byte != '\t') || byte > 126 ||
            lengthBytes == sizeof(lineBuffer) - 1)
        {
            isDiscarding = true;
            return LineResult::None;
        }

        lineBuffer[lengthBytes++] = static_cast<char>(byte);

        return LineResult::None;
    }

    bool LineParser::expire(uint32_t now)
    {
        // Unsigned subtraction handles millis() rollover; expiration is reported once.
        if ((lengthBytes || isDiscarding || hasCarriageReturn) && !hasReportedTimeout &&
            now - lastByteAtMs >= config::lineTimeoutMs)
        {
            lengthBytes = 0;
            isDiscarding = true;
            hasReportedTimeout = true;
            return true;
        }

        return false;
    }

    bool OutputQueue::enqueue(const char* line)
    {
        const size_t count = strlen(line);

        // Drop the entire record on overflow, never a partial telemetry line.
        if (count > capacityBytes - sizeBytes)
        {
            ++droppedCount;
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            bytes[(headIndex + sizeBytes + i) % capacityBytes] = static_cast<uint8_t>(line[i]);
        }

        sizeBytes += count;

        return true;
    }

    size_t OutputQueue::contiguous() const
    {
        // A single serial write cannot cross the ring's physical end.
        const size_t end = capacityBytes - headIndex;

        return sizeBytes < end ? sizeBytes : end;
    }

    void OutputQueue::consume(size_t count)
    {
        if (count > sizeBytes)
        {
            count = sizeBytes;
        }

        headIndex = (headIndex + count) % capacityBytes;
        sizeBytes -= count;
    }
} // namespace xl
