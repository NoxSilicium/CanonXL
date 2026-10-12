// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file canon_xl_controller.ino
 * @brief Implements Canon XL control, transport or serial telemetry.
 */
#include "BoardSupport.hpp"
#include "Config.hpp"
#include "LensController.hpp"
#include "SerialProtocol.hpp"
#include "Telemetry.hpp"

#include <Arduino.h>

namespace
{
    auto& usb = xl::serialEndpoint();
    xl::LensController controller;
    xl::LensTransport transport;
    xl::LineParser parser;
    xl::OutputQueue output;
    xl::Periodic frames(xl::config::framePeriodUs);
    xl::Periodic telemetry(xl::config::telemetryPeriodMs);
    bool stream = true;
    bool rawStream = false;
    bool flushInput = true;

    /** @brief Queues a bounded XL snapshot; output congestion drops the whole record. */
    void status()
    {
        char line[256];
        if (xl::formatTelemetry(line, sizeof(line), controller, millis(), frames.skipped(),
                output.dropped(), transport.actualHz()))
        {
            output.enqueue(line);
        }
    }

    /** @brief Queues all received response bytes, including invalid startup replies. */
    void rawStatus()
    {
        char line[160];
        if (xl::formatRawTelemetry(line, sizeof(line), controller, millis()))
        {
            output.enqueue(line);
        }
    }

    /** @brief Applies a fully validated serial command and queues its acknowledgement. */
    void command(const xl::Command& value)
    {
        switch (value.type)
        {
            case xl::CommandType::Set:
                output.enqueue(
                    controller.set(value.controls, millis()) ? "ACK,SET\n" : "ERR,not-ready\n");
                break;
            case xl::CommandType::Stop:
                controller.stop();
                output.enqueue("ACK,STOP\n");
                break;
            case xl::CommandType::Reset:
                controller.reset();
                output.enqueue("ACK,RESET\n");
                break;
            case xl::CommandType::Status:
                status();
                break;
            case xl::CommandType::Stream:
                stream = value.stream;
                output.enqueue("ACK,STREAM\n");
                break;
            case xl::CommandType::Raw:
                rawStream = value.stream;
                output.enqueue("ACK,RAW\n");
                break;
        }
    }

    /** @brief Processes at most 32 input bytes per loop and reports partial-line expiry. */
    void readSerial()
    {
        if (parser.expire(millis()))
        {
            output.enqueue("ERR,line-timeout\n");
        }

        // A flooded USB connection must leave time for the next SPI deadline.
        for (uint8_t i = 0; i < 32 && usb.available() > 0; ++i)
        {
            xl::Command value;
            const xl::LineResult result =
                parser.feed(static_cast<uint8_t>(usb.read()), millis(), value);
            if (result == xl::LineResult::Command)
            {
                command(value);
            }
            else if (result == xl::LineResult::Invalid)
            {
                output.enqueue("ERR,line\n");
            }
        }
    }

    /** @brief Drains at most 64 contiguous queued bytes without blocking SPI scheduling. */
    void writeSerial()
    {
        if (!xl::serialConnected())
        {
            output.clear();
            return;
        }

        size_t count = output.contiguous();

        if (count > 64)
        {
            count = 64;
        }

        if (count)
        {
            output.consume(xl::serialWrite(output.data(), count));
        }
    }
} // namespace

/** @brief Configures the transport, serial endpoint and periodic schedules. */
void setup()
{
    transport.begin();
    usb.begin(xl::config::serialBaud);
    frames.start(micros());
    telemetry.start(millis());
}

/** @brief Services deadlines, bounded serial work and motion-lease expiry. */
void loop()
{
    const bool connected = xl::serialConnected();
    if (!connected)
    {
        controller.stop();
        parser.reset();
        output.clear();
        flushInput = true;
        for (uint8_t i = 0; i < 32 && usb.available() > 0; ++i)
        {
            usb.read();
        }
    }

    // Motion expiry and SPI deadlines are serviced before bounded USB work.
    controller.tick(millis());

    if (frames.due(micros()))
    {
        xl::Frame tx, rx;
        controller.nextFrame(tx);
        transport.exchange(tx, rx);
        controller.receive(rx);
    }

    if (connected && flushInput)
    {
        // Discard any input left over from the previous USB session, in bounded steps.
        for (uint8_t i = 0; i < 32 && usb.available() > 0; ++i)
        {
            usb.read();
        }
        if (usb.available() == 0)
        {
            flushInput = false;
        }
    }
    else if (connected)
    {
        readSerial();
    }

    if (telemetry.due(millis()) && connected)
    {
        if (stream)
        {
            status();
        }
        if (rawStream)
        {
            rawStatus();
        }
    }

    writeSerial();
}
