// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

import g4p_controls.*;
import com.fazecast.jSerialComm.SerialPort;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * G4P control panel. Serial polling, model updates and widgets share the draw thread.
 * Opening a port is observation only; no actuator command is sent until user arming.
 */
SerialPort serialPort;
ControllerSession session;
ControllerProtocol.Framer framer;
PrintWriter captureWriter;
final long hostStartNs = System.nanoTime();
final long portSettleMs = 3000;
long nextPortScanMs;
long nextDemoMs;
long smokeDurationMs;
long sentSetCount;
long sentStopCount;
String requestedPort = "";
String transportError = "";
String capturePath = "";
String screenshotPath = "";
String[] availablePorts = new String[0];
boolean demoMode;
boolean rawEnabled;
boolean rawActive;
boolean xyEnabled;
boolean updatingWidgets;
int demoIris = 255;
int demoZoom;
int demoX;
int demoY;
long demoExchanges;
long inputReadyAtMs;
long startupBytes;

/** Fixed desktop tool geometry; Java2D avoids the archived JavaFX dependency. */
void settings()
{
    size(700, 580, JAVA2D);
    pixelDensity(1);
}

void setup()
{
    frameRate(30);
    surface.setTitle("Canon XL Control Panel");
    surface.setResizable(false);
    readOptions();
    session = new ControllerSession(line -> sendLine(line));
    createGui();
    refreshPorts();
    registerMethod("post", this);

    if (!capturePath.isEmpty())
    {
        startRecording(capturePath);
    }
    if (demoMode || !requestedPort.isEmpty())
    {
        connectPort(demoMode ? "DEMO" : requestedPort);
    }
}

/** Poll before scheduling commands, then paint widgets from validated model state. */
void draw()
{
    long nowMs = hostMs();
    background(238, 240, 242);
    try
    {
        // Check freshness before receiving: queued bytes must not revive an expired lease.
        session.checkSafety(nowMs, focused || benchRunning);
        pollInput(nowMs);
        if (session.connected && session.state != null && rawEnabled != rawActive)
        {
            session.setRaw(rawEnabled);
            rawActive = rawEnabled;
        }
        runBench(nowMs);
        if (session.zoom != 0 && !mousePressed && !benchRunning)
        {
            session.releaseZoom(nowMs);
        }
        if (inputReadyAtMs == 0)
        {
            session.tick(nowMs, focused || benchRunning);
        }
        if (nowMs >= nextPortScanMs)
        {
            refreshPorts();
            nextPortScanMs = nowMs + 1500;
        }
    }
    catch (RuntimeException error)
    {
        failTransport(error);
    }
    updateGui(nowMs);
}

/** Post-render screenshot includes G4P controls; finite smoke runs close their port. */
public void post()
{
    if (smokeDurationMs > 0 && hostMs() >= smokeDurationMs)
    {
        if (!screenshotPath.isEmpty())
        {
            get().save(screenshotPath);
        }
        println("SUMMARY states=" + session.receivedStates + " raw=" + session.receivedRaw
            + " rejected=" + (session.rejectedLines + (framer == null ? 0 : framer.rejectedLines))
            + " SET=" + sentSetCount + " STOP=" + sentStopCount
            + " phase=" + (session.state == null ? "none" : session.state.phase));
        exit();
    }
}

/** Use monotonic host time rather than Processing's signed, wrapping millis(). */
long hostMs()
{
    return (System.nanoTime() - hostStartNs) / 1000000L;
}

/** Diagnostic/bench modes are opt-in; normal launch never connects or actuates. */
void readOptions()
{
    if (args == null)
    {
        return;
    }
    for (String argument : args)
    {
        if (argument.equals("--demo"))
        {
            demoMode = true;
        }
        else if (argument.startsWith("--port="))
        {
            requestedPort = argument.substring(7);
        }
        else if (argument.startsWith("--smoke="))
        {
            smokeDurationMs = Long.parseLong(argument.substring(8)) * 1000;
        }
        else if (argument.startsWith("--record="))
        {
            capturePath = argument.substring(9);
        }
        else if (argument.startsWith("--screenshot="))
        {
            screenshotPath = argument.substring(13);
        }
        else if (argument.startsWith("--language="))
        {
            String language = argument.substring(11);
            if (!language.equals("en") && !language.equals("fr"))
            {
                throw new IllegalArgumentException("Language must be en or fr");
            }
            french = language.equals("fr");
        }
        else if (argument.equals("--raw"))
        {
            rawEnabled = true;
        }
        else if (argument.equals("--bench-test"))
        {
            benchRequested = true;
        }
        else
        {
            throw new IllegalArgumentException("Unknown option: " + argument);
        }
    }
    if (demoMode && !requestedPort.isEmpty())
    {
        throw new IllegalArgumentException("Demo cannot open a physical port");
    }
    if (benchRequested && smokeDurationMs < 15000)
    {
        throw new IllegalArgumentException("Bench test requires --smoke=15 or longer");
    }
}

/** Port identity is explicit. Never choose an attached Mega or analyser automatically. */
void refreshPorts()
{
    if (demoMode)
    {
        portList.setItems(new String[]
        {
            "DEMO"
        }, 0);
        return;
    }
    ArrayList<String> ports = new ArrayList<String>();
    for (SerialPort candidate : SerialPort.getCommPorts())
    {
        String port = candidate.getSystemPortPath();
        if (!port.contains("Bluetooth") && !port.contains("debug-console")
            && !port.startsWith("/dev/tty."))
        {
            ports.add(port);
        }
    }
    String[] scanned = ports.toArray(new String[0]);
    if (!Arrays.equals(scanned, availablePorts))
    {
        String selected = portList.getSelectedText();
        availablePorts = scanned;
        int selectedIndex = Math.max(0, ports.indexOf(selected));
        String[] items = scanned;
        if (items.length == 0)
        {
            items = new String[]
            {
                "--"
            };
        }
        portList.setItems(items, selectedIndex);
    }
    if (session.connected && !demoMode && !ports.contains(requestedPort))
    {
        failTransport(new IllegalStateException("Serial port disappeared"));
    }
}

/** DTR may reset a classic Nano. Await fresh XL,1 state before enabling ownership. */
void connectPort(String port)
{
    try
    {
        transportError = "";
        framer = new ControllerProtocol.Framer();
        if (!demoMode)
        {
            if (!Arrays.asList(availablePorts).contains(port))
            {
                throw new IllegalArgumentException("Select an available serial port");
            }
            serialPort = SerialPort.getCommPort(port);
            boolean configured = serialPort.setComPortParameters(115200, 8,
                SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
            configured &= serialPort.setComPortTimeouts(SerialPort.TIMEOUT_NONBLOCKING, 0, 0);
            configured &= serialPort.setFlowControl(SerialPort.FLOW_CONTROL_DISABLED);
            if (!configured)
            {
                throw new IllegalStateException("Cannot configure " + port);
            }
            if (!serialPort.openPort())
            {
                throw new IllegalStateException("Cannot open " + port);
            }
            requestedPort = port;
            portList.setSelected(Arrays.asList(availablePorts).indexOf(port));
        }
        session.open(hostMs());
        rawActive = false;
        startupBytes = 0;
        inputReadyAtMs = demoMode ? 0 : hostMs() + portSettleMs;
        note("Connected " + port + " at 115200; observation only");
    }
    catch (RuntimeException error)
    {
        failTransport(error);
    }
}

/** Bounded polling; no serial callback updates widgets from the native I/O thread. */
void pollInput(long nowMs)
{
    if (!session.connected)
    {
        return;
    }
    if (demoMode)
    {
        if (nowMs >= nextDemoMs)
        {
            demoExchanges += 5;
            int encoded = demoZoom == 0 ? 0 : (Math.abs(demoZoom) << 2)
                | (demoZoom > 0 ? 1 : 2);
            receiveLine("XL,1," + nowMs + ",ready,normal," + demoIris + "," + encoded
                + "," + demoX + "," + demoY + "," + demoIris
                + ",32,208,4,119," + demoExchanges + ",0,0,0,0,0,0,1000000,-1", nowMs);
            if (rawActive)
            {
                receiveLine(demoRawLine(nowMs), nowMs);
            }
            nextDemoMs = nowMs + 100;
        }
        return;
    }
    if (inputReadyAtMs != 0 && nowMs >= inputReadyAtMs)
    {
        framer = new ControllerProtocol.Framer();
        inputReadyAtMs = 0;
        note("Port settled; discarded " + startupBytes + " startup bytes; awaiting fresh XL state");
    }
    int available = serialPort.bytesAvailable();
    if (available < 0)
    {
        throw new IllegalStateException("Serial read failed");
    }
    if (inputReadyAtMs == 0 && available > 8192)
    {
        throw new IllegalStateException("Serial input backlog");
    }
    int count = Math.min(available, 4096);
    if (count > 0)
    {
        byte[] bytes = new byte[count];
        int received = serialPort.readBytes(bytes, count);
        if (received < 0)
        {
            throw new IllegalStateException("Serial read failed");
        }
        if (inputReadyAtMs != 0)
        {
            startupBytes += received;
        }
        else
        {
            framer.feed(Arrays.copyOf(bytes, received), line -> receiveLine(line, nowMs));
        }
    }
}

/** Synthetic full response exists only in explicitly labeled simulation mode. */
String demoRawLine(long nowMs)
{
    int[] bytes = new int[40];
    bytes[0] = 2;
    bytes[1] = demoIris;
    bytes[3] = 32;
    bytes[4] = 208;
    bytes[15] = 4;
    bytes[16] = 119;
    int sum = 0;
    for (int value : bytes)
    {
        sum += value;
    }
    bytes[39] = (255 - sum) & 255;
    StringBuilder hexBytes = new StringBuilder();
    for (int value : bytes)
    {
        hexBytes.append(hex(value, 2));
    }
    return "XLRAW,1," + nowMs + "," + demoExchanges + ",ready,normal,1," + hexBytes;
}

/** Record before parsing so malformed complete lines remain visible in capture evidence. */
void receiveLine(String line, long nowMs)
{
    record("IN", line);
    session.receive(line, nowMs);
}

/** Transport adds exactly one LF. Simulation never opens serial hardware. */
void sendLine(String line)
{
    record("OUT", line);
    if (line.startsWith("SET,"))
    {
        sentSetCount++;
    }
    if (line.equals("STOP"))
    {
        sentStopCount++;
    }
    if (demoMode)
    {
        if (line.startsWith("SET,"))
        {
            String[] fields = line.split(",");
            demoIris = Integer.parseInt(fields[1]);
            demoZoom = Integer.parseInt(fields[2]);
            demoX = Integer.parseInt(fields[3]);
            demoY = Integer.parseInt(fields[4]);
        }
        else if (line.equals("STOP"))
        {
            demoZoom = 0;
            demoX = 0;
            demoY = 0;
        }
        return;
    }
    if (serialPort == null)
    {
        throw new IllegalStateException("Serial port is closed");
    }
    byte[] bytes = (line + "\n").getBytes(StandardCharsets.US_ASCII);
    if (serialPort.writeBytes(bytes, bytes.length) != bytes.length)
    {
        throw new IllegalStateException("Incomplete serial write");
    }
}

/** Restore only raw streaming enabled by this panel; leave board STREAM configuration intact. */
void disconnectPort()
{
    try
    {
        if (session != null && session.connected)
        {
            if (benchRunning)
            {
                cancelBench();
            }
            session.close();
            if (rawActive)
            {
                sendLine("RAW,0");
            }
        }
    }
    finally
    {
        if (serialPort != null)
        {
            serialPort.closePort();
            serialPort = null;
        }
        xyEnabled = false;
        rawActive = false;
    }
}

void failTransport(RuntimeException error)
{
    transportError = error.getMessage() == null ? error.toString() : error.getMessage();
    note("Transport error: " + transportError);
    try
    {
        disconnectPort();
    }
    catch (RuntimeException closeError)
    {
        note("Close error: " + closeError.toString());
    }
}

/** Explicit recording preserves IN/OUT lines, host time and a header naming the bench. */
void startRecording(String path)
{
    try
    {
        Path destination = Path.of(path).toAbsolutePath();
        Files.createDirectories(destination.getParent());
        captureWriter = new PrintWriter(Files.newBufferedWriter(destination,
            StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE));
        capturePath = destination.toString();
        note("Canon XL panel; " + (demoMode ? "simulation" : "Nano / standalone 20x bench"));
    }
    catch (java.io.IOException error)
    {
        transportError = "Capture: " + error.getMessage();
    }
}

void record(String direction, String line)
{
    if (captureWriter != null)
    {
        captureWriter.println(hostMs() + "\t" + direction + "\t" + line);
        captureWriter.flush();
        if (captureWriter.checkError())
        {
            captureWriter.close();
            captureWriter = null;
            transportError = "Capture write failed";
        }
    }
}

void note(String message)
{
    println(message);
    record("NOTE", message);
}

/** Closing the window stops owned motion and releases the port, including smoke runs. */
@Override
public void exit()
{
    try
    {
        disconnectPort();
    }
    catch (RuntimeException error)
    {
        note("Exit: " + error.toString());
    }
    finally
    {
        if (captureWriter != null)
        {
            captureWriter.close();
            captureWriter = null;
        }
        super.exit();
    }
}
