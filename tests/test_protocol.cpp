// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file test_protocol.cpp
 * @brief Exercises frame, dialogue and serial contracts without hardware.
 */
#include "Config.hpp"
#include "LensController.hpp"
#include "SerialProtocol.hpp"
#include "Telemetry.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define XL_CHECK(expr)                                                                             \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            std::cerr << __LINE__ << ": " #expr "\n";                                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

xl::Frame response(uint8_t header = 2)
{
    xl::Frame frame =
    {
    };
    frame.bytes[0] = header;
    frame.bytes[1] = 0x33;
    frame.bytes[3] = 0x30;
    frame.bytes[4] = 240;
    frame.bytes[15] = 6;
    frame.bytes[16] = 152;
    xl::seal(frame);
    return frame;
}

xl::LineResult line(
    xl::LineParser& parser, const std::string& text, xl::Command& command, uint32_t now = 0)
{
    xl::LineResult result = xl::LineResult::None;
    for (unsigned char byte : text)
    {
        result = parser.feed(byte, now, command);
    }
    return result;
}

std::vector<std::string> fields(const char* line)
{
    std::stringstream input(line);
    std::vector<std::string> values;
    std::string value;
    while (std::getline(input, value, ','))
    {
        values.push_back(value);
    }
    return values;
}

void protocol()
{
    xl::Frame frame;
    xl::buildInit(frame);
    XL_CHECK(frame.bytes[0] == 1 && frame.bytes[39] == 0x2b);
    XL_CHECK(xl::validChecksum(frame));
    const uint8_t command[] =
    {
        2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x50, 0, 0,
        0, 0, 0, 0, 0x18, 0, 0, 0, 5, 8, 0xa0, 0x80, 0, 0x0d, 0xff, 0xff, 0xff, 0
    };
    for (int byte = 0; byte < 256; ++byte)
    {
        const uint8_t reversed = xl::reverseBits(static_cast<uint8_t>(byte));
        XL_CHECK(xl::reverseBits(reversed) == byte);
        for (int bit = 0; bit < 8; ++bit)
        {
            XL_CHECK(((reversed >> (7 - bit)) & 1) == ((byte >> bit) & 1));
        }
        xl::Controls controls;
        controls.iris = static_cast<uint8_t>(byte);
        controls.x = -5000;
        controls.y = 5000;
        controls.zoom = 8;
        XL_CHECK(xl::buildCommand(controls, frame) && xl::validChecksum(frame));
        XL_CHECK(frame.bytes[19] == byte && frame.bytes[28] == 0x21);
        XL_CHECK(frame.bytes[20] == 0xec && frame.bytes[21] == 0x78);
        XL_CHECK(frame.bytes[22] == 0x13 && frame.bytes[23] == 0x88);
        for (size_t i = 0; i < 39; ++i)
        {
            if (i != 19 && !(i >= 20 && i <= 23) && i != 28)
            {
                XL_CHECK(frame.bytes[i] == command[i]);
            }
        }
    }
    for (int zoom = -15; zoom <= 15; ++zoom)
    {
        const uint8_t code = xl::encodeZoom(static_cast<int8_t>(zoom));
        XL_CHECK(code == (zoom == 0 ? 0 : (std::abs(zoom) << 2) | (zoom > 0 ? 1 : 2)));
    }
    XL_CHECK(xl::encodeZoom(-8) == 0x22 && xl::encodeZoom(8) == 0x21 && xl::encodeZoom(0) == 0);
    XL_CHECK(xl::encodeZoom(-128) == 0 && xl::encodeZoom(16) == 0);
    const xl::Frame before = frame;
    xl::Controls invalid;
    invalid.x = 5001;
    XL_CHECK(!xl::buildCommand(invalid, frame) && std::memcmp(&frame, &before, sizeof(frame)) == 0);
    xl::Frame zeros =
    {
    }, ff;
    std::memset(&ff, 255, sizeof(ff));
    XL_CHECK(xl::classify(zeros) == xl::ResponseKind::Zero && !xl::validChecksum(zeros));
    XL_CHECK(xl::classify(ff) == xl::ResponseKind::AllFf && !xl::validChecksum(ff));
    frame = response(3);
    XL_CHECK(xl::classify(frame) == xl::ResponseKind::Unknown);
    frame.bytes[39] ^= 1;
    XL_CHECK(xl::classify(frame) == xl::ResponseKind::BadChecksum);
}

void controller()
{
    xl::LensController controller;
    xl::Frame frame;
    xl::Controls controls;
    controls.iris = 0x30;
    controls.zoom = 8;
    controls.x = -5000;
    controls.y = 5000;
    XL_CHECK(!controller.set(controls, 0));
    const xl::Frame zeros =
    {
    };
    for (const xl::Frame& rx :
    {
        zeros, response(1), response(1), response(2)
    })
    {
        controller.nextFrame(frame);
        XL_CHECK(frame.bytes[0] == (controller.phase() == xl::Phase::Initialising ? 1 : 2));
        controller.receive(rx);
    }
    XL_CHECK(controller.phase() == xl::Phase::Ready);
    XL_CHECK(controller.set(controls, 100));
    controller.nextFrame(frame);
    XL_CHECK(frame.bytes[19] == 0x30 && frame.bytes[28] == 0x21);
    controller.tick(599);
    XL_CHECK(controller.hasCommand());
    controller.tick(600);
    XL_CHECK(!controller.hasCommand());
    controller.nextFrame(frame);
    XL_CHECK(frame.bytes[28] == 0 && frame.bytes[20] == 0 && frame.bytes[23] == 0 &&
        frame.bytes[19] == 0x30);
    XL_CHECK(controller.set(controls, 700));
    xl::Frame bad = response();
    bad.bytes[39] ^= 1;
    controller.receive(bad);
    XL_CHECK(controller.phase() == xl::Phase::Probing && !controller.hasCommand());
    XL_CHECK(!controller.set(controls, 701));
    controller.receive(response());
    XL_CHECK(controller.phase() == xl::Phase::Ready);
    controller.nextFrame(frame);
    XL_CHECK(frame.bytes[28] == 0);
    XL_CHECK(controller.set(controls, 800));
    controller.receive(response(1));
    XL_CHECK(controller.phase() == xl::Phase::Probing && !controller.hasCommand());
    for (int i = 1; i < xl::config::probeLimit; ++i)
    {
        controller.receive(response(1));
    }
    XL_CHECK(controller.phase() == xl::Phase::Initialising && controller.controls().iris == 255);
    controller.receive(response());
    XL_CHECK(controller.set(controls, 900));
    for (int i = 0; i < xl::config::badResponseLimit; ++i)
    {
        controller.receive(bad);
    }
    XL_CHECK(controller.phase() == xl::Phase::Initialising && !controller.hasCommand());
    controller.receive(response());
    XL_CHECK(controller.set(controls, 1000));
    xl::Frame ff;
    std::memset(&ff, 255, sizeof(ff));
    controller.receive(ff);
    XL_CHECK(controller.phase() == xl::Phase::Initialising && controller.controls().iris == 255);
    controller.receive(response());
    XL_CHECK(controller.set(controls, 1100));
    controller.receive(zeros);
    XL_CHECK(controller.phase() == xl::Phase::Initialising && controller.controls().zoom == 0);
    controller.receive(response());
    XL_CHECK(controller.set(controls, UINT32_MAX - 250));
    controller.tick(248);
    XL_CHECK(controller.hasCommand());
    controller.tick(249);
    XL_CHECK(!controller.hasCommand());
    controls.zoom = 16;
    XL_CHECK(!controller.set(controls, 1500));
    XL_CHECK(controller.statistics().exchanges > 100 && controller.statistics().allFf == 1);
}

void scheduling()
{
    xl::Periodic timer(20000);
    timer.start(0);
    XL_CHECK(!timer.due(19999));
    XL_CHECK(timer.due(20000));
    XL_CHECK(!timer.due(20001));
    XL_CHECK(timer.due(105000) && timer.skipped() == 3);
    XL_CHECK(!timer.due(119999));
    XL_CHECK(timer.due(120000));
    timer.start(UINT32_MAX - 9999);
    XL_CHECK(!timer.due(9999));
    XL_CHECK(timer.due(10000));
    XL_CHECK(!timer.due(10001) && timer.skipped() == 0);
}

void parser()
{
    xl::LineParser parser;
    xl::Command command;
    for (const std::string& text :
    {
        "SET,0x50,8,-100,100\n", " SET , 80 , 8 , -100 , 100 \r\n"
    })
    {
        XL_CHECK(line(parser, text, command) == xl::LineResult::Command);
        XL_CHECK(command.type == xl::CommandType::Set && command.controls.iris == 0x50);
        XL_CHECK(
            command.controls.zoom == 8 && command.controls.x == -100 && command.controls.y == 100);
    }
    for (const std::string& text :
    {
        "SET,5.6,8,0,0\n", "SET,256,8,0,0\n", "SET,0xGG,8,0,0\n", "SET,0x50,16,0,0\n",
        "SET,0x50,-16,0,0\n", "SET,0x50,8,5001,0\n", "SET,0x50,8,0,-5001\n", "SET,0x50,8,,0\n",
        "SET,0x50,8,0,0,1\n", "SET,0x50,8,0\n", "SET,0x50,8,0,1e2\n", "SET,0x50,8,nan,0\n",
        "SET,0x50,8,0,999999999999999999999999999999\n", "STATUS,0\n", "STREAM,2\n",
        "STOP\rX\n", "RAW\n", "RAW,2\n", "RAW,-1\n", "RAW,0x01\n", "RAW,1,0\n", "RAW,\n"
    })
    {
        command.type = xl::CommandType::Stop;
        command.controls.zoom = -8;
        XL_CHECK(line(parser, text, command) == xl::LineResult::Invalid);
        XL_CHECK(command.type == xl::CommandType::Stop && command.controls.zoom == -8);
    }
    XL_CHECK(line(parser, std::string(120, 'X') + "\n", command) == xl::LineResult::Invalid);
    XL_CHECK(line(parser, std::string("SET,80,") + '\0' + "8,0,0\n", command) ==
        xl::LineResult::Invalid);
    for (const std::string& text :
    {
        "STOP\n", "RESET\n", "STATUS\n", "STREAM,0\n", "STREAM,1\n"
    })
    {
        XL_CHECK(line(parser, text, command) == xl::LineResult::Command);
    }
    XL_CHECK(line(parser, " RAW , 1 \r\n", command) == xl::LineResult::Command);
    XL_CHECK(command.type == xl::CommandType::Raw && command.stream);
    XL_CHECK(line(parser, "RAW,0\n", command) == xl::LineResult::Command);
    XL_CHECK(command.type == xl::CommandType::Raw && !command.stream);
    XL_CHECK(line(parser, "SET,80,8", command, 10) == xl::LineResult::None);
    XL_CHECK(!parser.expire(259) && parser.expire(260) && !parser.expire(261));
    XL_CHECK(line(parser, ",0,0\n", command, 261) == xl::LineResult::None);
    XL_CHECK(line(parser, "STOP\n", command, 262) == xl::LineResult::Command);
    XL_CHECK(line(parser, "SET,80,", command, UINT32_MAX - 100) == xl::LineResult::None);
    XL_CHECK(parser.expire(149));
    XL_CHECK(line(parser, "8,0,0\n", command, 150) == xl::LineResult::None);
    uint32_t random = 1234567;
    for (int n = 0; n < 10000; ++n)
    {
        random = random * 1664525U + 1013904223U;
        const size_t length = random % 150;
        for (size_t i = 0; i < length; ++i)
        {
            random = random * 1664525U + 1013904223U;
            const xl::LineResult result =
                parser.feed(static_cast<uint8_t>(random >> 24), n, command);
            if (result == xl::LineResult::Command)
            {
                XL_CHECK(xl::validControls(command.controls));
            }
        }
        parser.feed('\n', n, command);
        XL_CHECK(line(parser, "SET,0x50,8,-100,100\n", command, n) == xl::LineResult::Command);
    }
}

void output()
{
    xl::OutputQueue queue;
    XL_CHECK(queue.contiguous() == 0 && queue.enqueue(std::string(400, 'A').c_str()));
    queue.consume(350);
    XL_CHECK(queue.enqueue(std::string(450, 'B').c_str()));
    XL_CHECK(!queue.enqueue(std::string(20, 'C').c_str()) && queue.dropped() == 1);
    std::string received;
    while (queue.contiguous())
    {
        const size_t count = queue.contiguous();
        received.append(reinterpret_cast<const char*>(queue.data()), count);
        queue.consume(count);
    }
    XL_CHECK(received == std::string(50, 'A') + std::string(450, 'B'));
    XL_CHECK(queue.enqueue("ACK,STOP\n"));
    queue.consume(1000);
    XL_CHECK(queue.contiguous() == 0);
    XL_CHECK(!queue.enqueue(std::string(600, 'X').c_str()));
    queue.clear();
    xl::LensController controller;
    char buffer[256];
    XL_CHECK(xl::formatTelemetry(buffer, sizeof(buffer), controller, 0, 0, 0, 1000000));
    auto values = fields(buffer);
    XL_CHECK(values.size() == 23 && values[9] == "-1");
    controller.receive(response());
    xl::Controls controls;
    controls.iris = 0x30;
    controls.zoom = 8;
    XL_CHECK(controller.set(controls, 100));
    XL_CHECK(xl::formatTelemetry(
        buffer, sizeof(buffer), controller, 110, UINT32_MAX, UINT32_MAX, 1000000));
    values = fields(buffer);
    XL_CHECK(
        values[0] == "XL" && values[1] == "1" && values[3] == "ready" && values[4] == "normal");
    XL_CHECK(values[5] == "48" && values[6] == "33" && values[9] == "51" && values[11] == "240");
    XL_CHECK(values[12] == "6" && values[13] == "152" && values[22] == "10\n");
    XL_CHECK(!xl::formatTelemetry(buffer, 4, controller, 0, 0, 0, 0));
    xl::Frame bad = response();
    bad.bytes[39] ^= 1;
    controller.receive(bad);
    XL_CHECK(xl::formatTelemetry(buffer, sizeof(buffer), controller, 120, 0, 0, 1000000));
    values = fields(buffer);
    XL_CHECK(values[9] == "-1" && values[6] == "0" && values[4] == "checksum");
}

void rawTelemetry()
{
    xl::LensController controller;
    char buffer[160];
    XL_CHECK(!xl::formatRawTelemetry(buffer, sizeof(buffer), controller, 0));
    xl::Frame patterned =
    {
    };
    for (size_t i = 0; i < xl::frameLength; ++i)
    {
        patterned.bytes[i] = static_cast<uint8_t>(i * 7);
    }
    patterned.bytes[0] = 2;
    xl::seal(patterned);
    xl::Frame zero =
    {
    }, ff;
    std::memset(&ff, 255, sizeof(ff));
    xl::Frame bad = patterned;
    bad.bytes[39] ^= 1;
    xl::Frame init = patterned;
    init.bytes[0] = 1;
    xl::seal(init);
    xl::Frame unknown = patterned;
    unknown.bytes[0] = 3;
    xl::seal(unknown);
    for (const xl::Frame& rx :
    {
        patterned, init, bad, zero, ff, unknown
    })
    {
        controller.receive(rx);
        XL_CHECK(xl::formatRawTelemetry(buffer, sizeof(buffer), controller, UINT32_MAX));
        const auto values = fields(buffer);
        XL_CHECK(values.size() == 8 && values[0] == "XLRAW" && values[1] == "1");
        XL_CHECK(values[2] == "4294967295" &&
            values[3] == std::to_string(controller.statistics().exchanges));
        XL_CHECK(values[4] == xl::phaseName(controller.phase()));
        XL_CHECK(values[5] == xl::responseName(xl::classify(rx)));
        XL_CHECK(values[6] == (xl::validChecksum(rx) ? "1" : "0"));
        XL_CHECK(values[7].size() == 81 && values[7].back() == '\n');
        for (size_t i = 0; i < xl::frameLength; ++i)
        {
            XL_CHECK(std::stoul(values[7].substr(i * 2, 2), nullptr, 16) == rx.bytes[i]);
        }
        const std::string expected(buffer);
        const size_t length = expected.size();
        for (size_t capacity :
        {
            size_t(0), size_t(4), length
        })
        {
            std::memset(buffer, '!', sizeof(buffer));
            XL_CHECK(!xl::formatRawTelemetry(buffer, capacity, controller, UINT32_MAX));
            XL_CHECK(buffer[capacity] == '!');
        }
        XL_CHECK(xl::formatRawTelemetry(buffer, length + 1, controller, UINT32_MAX));
        XL_CHECK(buffer == expected);
    }
    controller.reset();
    XL_CHECK(!xl::formatRawTelemetry(buffer, sizeof(buffer), controller, 1));
}

int main()
{
    protocol();
    controller();
    scheduling();
    parser();
    output();
    rawTelemetry();
    std::cout << "Core OK: frames, 256 bit reversals, controls, handshake, recovery, timeouts, "
                 "rollover, 10000 parser fuzz cases, output queue, telemetry.\n";
}
