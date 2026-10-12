// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/** G4P layout preserves the historical focus/zoom/iris, XY pad and serial strip. */
GCustomSlider focusSlider;
GCustomSlider speedSlider;
GSlider irisSlider;
GTextField irisField;
GSlider2D xyPad;
GSlider xSlider;
GSlider ySlider;
GButton zoomMinus;
GButton zoomPlus;
GButton centerButton;
GButton connectButton;
GButton refreshButton;
GButton stopButton;
GButton applyIrisButton;
GDropList portList;
GDropList languageList;
GCheckbox armBox;
GCheckbox xyBox;
GCheckbox rawBox;
GCheckbox recordBox;
GTextArea telemetryArea;
GLabel focusLabel;
GLabel zoomLabel;
GLabel irisLabel;
GLabel xyLabel;
GLabel speedLabel;
GLabel connectionLabel;
GLabel irisValueLabel;
GLabel xyValueLabel;
GLabel stateLabel;
boolean french = true;
long nextDisplayMs;

/** English identifiers and source; localized UI strings remain separate from protocol tokens. */
String ui(String english, String translated)
{
    return french ? translated : english;
}

/** Shared unframed labels, with pixel geometry fixed by the original panel layout. */
GLabel makeLabel(float x, float y, float width, float height, String text)
{
    GLabel label = new GLabel(this, x, y, width, height, text);
    label.setTextAlign(GAlign.LEFT, GAlign.MIDDLE);
    label.setOpaque(false);
    return label;
}

void addTip(GAbstractControl control, String text)
{
    control.setTip(text, GAlign.CENTER, GAlign.BOTTOM, 6);
    control.setTipDisplayTime(5000);
}

/** Build controls once; model/locale updates never recreate widgets or acquire a port. */
void createGui()
{
    G4P.messagesEnabled(false);
    G4P.setGlobalColorScheme(GCScheme.CYAN_SCHEME);
    G4P.setDisplayFont("SansSerif", java.awt.Font.PLAIN, 13);
    G4P.setInputFont("Monospaced", java.awt.Font.PLAIN, 12);
    G4P.setMouseOverEnabled(true);
    makeLabel(20, 12, 360, 32, "CANON XL / " + (demoMode ? "DEMO" : "20x"));
    languageList = new GDropList(this, 565, 18, 115, 74, 2);
    languageList.setItems(new String[]
    {
        "English", "Fran\u00e7ais"
    }, french ? 1 : 0);
    languageList.addEventHandler(this, "languageChanged");

    focusLabel = makeLabel(20, 65, 100, 30, "");
    focusSlider = new GCustomSlider(this, 130, 58, 280, 42, "grey_blue");
    focusSlider.setLimits(0, 0, 100);
    focusSlider.setEnabled(false);

    zoomLabel = makeLabel(20, 123, 100, 30, "Zoom");
    zoomMinus = new GButton(this, 130, 124, 40, 30, "-");
    zoomPlus = new GButton(this, 370, 124, 40, 30, "+");
    zoomMinus.fireAllEvents(true);
    zoomPlus.fireAllEvents(true);
    zoomMinus.addEventHandler(this, "zoomChanged");
    zoomPlus.addEventHandler(this, "zoomChanged");
    speedLabel = makeLabel(190, 99, 150, 20, "");
    speedSlider = new GCustomSlider(this, 190, 117, 160, 45, "grey_blue");
    speedSlider.setLimits(8, 1, 15);
    speedSlider.setNumberFormat(G4P.INTEGER);
    speedSlider.setNbrTicks(15);
    speedSlider.setStickToTicks(true);
    speedSlider.setShowValue(true);
    speedSlider.addEventHandler(this, "speedChanged");

    irisLabel = makeLabel(20, 180, 100, 30, "");
    irisSlider = new GSlider(this, 130, 175, 280, 45, 12);
    irisSlider.setLimits(255, 0, 255);
    irisSlider.setNumberFormat(G4P.INTEGER);
    irisSlider.setEasing(1);
    irisSlider.setShowValue(true);
    irisSlider.addEventHandler(this, "irisSliderChanged");
    irisField = new GTextField(this, 130, 220, 75, 28);
    irisField.setNumeric(255, 0, 255);
    irisField.setText("255");
    irisField.addEventHandler(this, "irisFieldChanged");
    irisValueLabel = makeLabel(215, 218, 85, 28, "0xFF");
    applyIrisButton = new GButton(this, 310, 220, 100, 28, "");
    applyIrisButton.addEventHandler(this, "applyIris");

    xyLabel = makeLabel(465, 60, 140, 24, "");
    centerButton = new GButton(this, 620, 60, 60, 26, "+");
    centerButton.addEventHandler(this, "centerClicked");
    xyPad = new GSlider2D(this, 465, 95, 185, 145);
    xyPad.setLimitsX(0, -5000, 5000);
    xyPad.setLimitsY(0, -5000, 5000);
    xyPad.setNumberFormat(G4P.INTEGER);
    xyPad.setEasing(1);
    xyPad.addEventHandler(this, "padChanged");
    xSlider = new GSlider(this, 465, 242, 185, 30, 10);
    xSlider.setLimits(0, -5000, 5000);
    xSlider.setNumberFormat(G4P.INTEGER);
    xSlider.setEasing(1);
    xSlider.addEventHandler(this, "axisChanged");
    ySlider = new GSlider(this, 685, 95, 145, 28, 10);
    ySlider.setRotation(HALF_PI, GControlMode.CORNER);
    ySlider.setLimits(0, -5000, 5000);
    ySlider.setNumberFormat(G4P.INTEGER);
    ySlider.setEasing(1);
    ySlider.addEventHandler(this, "axisChanged");
    xyValueLabel = makeLabel(465, 272, 220, 22, "X 0 / Y 0");
    xyBox = new GCheckbox(this, 465, 297, 170, 26, "X/Y");
    xyBox.addEventHandler(this, "xyToggled");

    armBox = new GCheckbox(this, 20, 275, 270, 28, "");
    armBox.addEventHandler(this, "armToggled");
    stopButton = new GButton(this, 310, 275, 100, 28, "\u25a0");
    stopButton.addEventHandler(this, "stopClicked");
    portList = new GDropList(this, 20, 342, 420, 100, 3);
    portList.setItems(new String[]
    {
        "--"
    }, 0);
    refreshButton = new GButton(this, 450, 342, 35, 30, "\u21bb");
    refreshButton.addEventHandler(this, "refreshClicked");
    connectButton = new GButton(this, 495, 342, 185, 30, "");
    connectButton.addEventHandler(this, "connectClicked");
    connectionLabel = makeLabel(20, 376, 660, 22, "");
    stateLabel = makeLabel(20, 411, 325, 25, "");
    rawBox = new GCheckbox(this, 365, 411, 125, 25, "MISO 40");
    rawBox.addEventHandler(this, "rawToggled");
    recordBox = new GCheckbox(this, 525, 411, 155, 25, "");
    recordBox.addEventHandler(this, "recordToggled");
    telemetryArea = new GTextArea(this, 20, 445, 660, 115, G4P.SCROLLBARS_VERTICAL_ONLY);
    telemetryArea.setTextEditEnabled(false);
    telemetryArea.setOpaque(true);
    localizeGui();
}

/** Re-label widgets without recreating them or changing any actuator state. */
void localizeGui()
{
    focusLabel.setText(ui("Focus", "Mise au point"));
    irisLabel.setText(ui("Iris (raw)", "Iris (brut)"));
    xyLabel.setText(ui("Stabilization X/Y", "Stabilisation X/Y"));
    speedLabel.setText(ui("Amplitude", "Amplitude"));
    armBox.setText(ui("Enable commands", "Activer les commandes"));
    xyBox.setText(ui("Enable X/Y", "Activer X/Y"));
    applyIrisButton.setText(ui("Apply", "Appliquer"));
    recordBox.setText(ui("Record", "Enregistrer"));
    addTip(focusSlider, ui("Electronic focus is not implemented",
        "Commande de focus non impl\u00e9ment\u00e9e"));
    addTip(zoomMinus, ui("Hold for negative zoom", "Maintenir : zoom n\u00e9gatif"));
    addTip(zoomPlus, ui("Hold for positive zoom", "Maintenir : zoom positif"));
    addTip(centerButton, ui("Center X/Y; does not reset the Nano",
        "Centrer X/Y sans r\u00e9initialiser le Nano"));
    addTip(stopButton, ui("Stop motion and disable commands; retain iris",
        "Arr\u00eater et d\u00e9sactiver les commandes ; conserver l'iris"));
    addTip(refreshButton, ui("Refresh serial ports", "Actualiser les ports s\u00e9rie"));
    addTip(irisSlider, ui("Raw request 0..255; 255 closes. Apply explicitly.",
        "Consigne brute 0..255 ; 255 ferme. Appliquer explicitement."));
    addTip(rawBox, ui("Latest 40-byte MISO sample at 10 Hz",
        "Dernier \u00e9chantillon MISO de 40 octets \u00e0 10 Hz"));
    addTip(recordBox, ui("Record received lines and outgoing commands",
        "Enregistrer les lignes re\u00e7ues et les commandes \u00e9mises"));
    nextDisplayMs = 0;
}

/** Requests and feedback remain separate. Stale values are not shown as live measurements. */
void updateGui(long nowMs)
{
    updatingWidgets = true;
    boolean ready = session.isFresh(nowMs) && session.state.isReady();
    boolean enabled = ready && session.armed;
    armBox.setSelected(session.armed);
    armBox.setEnabled(session.armed || session.canArm(nowMs));
    zoomMinus.setEnabled(enabled);
    zoomPlus.setEnabled(enabled);
    speedSlider.setEnabled(enabled);
    irisSlider.setEnabled(enabled);
    irisField.setEnabled(enabled);
    applyIrisButton.setEnabled(enabled);
    xyBox.setEnabled(enabled);
    if (!enabled)
    {
        xyEnabled = false;
        syncAxes(0, 0);
        irisSlider.setValue(session.iris);
        irisField.setText(str(session.iris));
    }
    xyBox.setSelected(xyEnabled);
    xyPad.setEnabled(enabled && xyEnabled);
    xSlider.setEnabled(enabled && xyEnabled);
    ySlider.setEnabled(enabled && xyEnabled);
    centerButton.setEnabled(enabled && xyEnabled);
    stopButton.setEnabled(enabled);
    rawBox.setEnabled(session.connected);
    rawBox.setSelected(rawEnabled);
    recordBox.setSelected(captureWriter != null);
    portList.setEnabled(!session.connected && !demoMode);
    refreshButton.setEnabled(!demoMode);
    connectButton.setText(session.connected ? ui("Disconnect", "D\u00e9connecter")
        : ui("Connect", "Connecter"));
    irisValueLabel.setText(irisField.isValid() ? "0x" + hex(irisField.getValueI(), 2) : "--");
    xyValueLabel.setText("X " + session.x + " / Y " + session.y);
    String connection = !transportError.isEmpty() ? transportError : session.connected
        ? (demoMode ? "DEMO" : requestedPort + " / 115200")
        : ui("Disconnected", "D\u00e9connect\u00e9");
    connectionLabel.setText(connection);
    String status = !session.connected ? ui("Offline", "Hors ligne") : !session.isFresh(nowMs)
        ? ui("Waiting for state", "En attente de t\u00e9l\u00e9m\u00e9trie")
        : session.state.phase + " / " + session.state.reply;
    stateLabel.setText(status + " / " + ui(session.armed ? "CONTROL" : "READ ONLY",
        session.armed ? "COMMANDES" : "LECTURE SEULE"));
    updatingWidgets = false;

    stroke(199, 205, 209);
    line(20, 48, 680, 48);
    line(440, 60, 440, 320);
    line(20, 328, 680, 328);
    line(20, 402, 680, 402);
    if (nowMs >= nextDisplayMs)
    {
        telemetryArea.setText(telemetryText(nowMs));
        nextDisplayMs = nowMs + 100;
    }
}

/** Non-normal replies clear raw byte displays; matching raw exchange IDs stay explicit. */
String telemetryText(long nowMs)
{
    if (!session.isFresh(nowMs))
    {
        return ui("No fresh telemetry", "Aucune t\u00e9l\u00e9m\u00e9trie r\u00e9cente")
            + "\n" + session.lastMessage;
    }
    ControllerProtocol.State state = session.state;
    String text = ui("Request", "Consigne") + ": iris " + state.iris
        + " / zoom " + ControllerProtocol.decodeZoom(state.encodedZoom)
        + " / X " + state.x + " / Y " + state.y + "\n";
    text += "MISO: 2=" + rawByte(state.irisFeedback) + " 4=" + rawByte(state.controls)
        + " 5=" + rawByte(state.byte5) + " 16=" + rawByte(state.byte16)
        + " 17=" + rawByte(state.byte17) + "\n";
    text += "SPI " + state.spiHz + " Hz / #" + state.exchanges
        + " / checksum " + state.badChecksums + " / zero " + state.zeroReplies
        + " / FF " + state.ffReplies + "\n";
    text += "unknown " + state.unknownReplies + " / skipped " + state.skippedPeriods
        + " / dropped " + state.droppedOutput + " / RX rejected "
        + (session.rejectedLines + (framer == null ? 0 : framer.rejectedLines))
        + " / " + session.lastMessage;
    if (rawEnabled && session.raw != null && nowMs - session.rawAtMs <= ControllerSession.staleMs)
    {
        ControllerProtocol.RawFrame raw = session.raw;
        text += "\nMISO40 #" + raw.exchanges + " / " + raw.reply
            + " / checksum " + (raw.checksumValid ? "OK" : "BAD");
        for (int start = 0; start < 80; start += 40)
        {
            text += "\n";
            for (int index = start; index < start + 40; index += 2)
            {
                text += raw.hex.substring(index, index + 2) + " ";
            }
        }
    }
    return text;
}

String rawByte(int value)
{
    return value < 0 ? "--" : "0x" + hex(value, 2);
}

/** Guard callbacks generated by synchronized widgets, not just user input. */
public void languageChanged(GDropList source, GEvent event)
{
    french = source.getSelectedIndex() == 1;
    localizeGui();
}

/** Toggle explicit port ownership; labels never determine connection state. */
public void connectClicked(GButton source, GEvent event)
{
    if (event != GEvent.CLICKED)
    {
        return;
    }
    if (session.connected)
    {
        try
        {
            disconnectPort();
        }
        catch (RuntimeException error)
        {
            failTransport(error);
        }
    }
    else
    {
        connectPort(demoMode ? "DEMO" : portList.getSelectedText());
    }
}

public void refreshClicked(GButton source, GEvent event)
{
    nextPortScanMs = 0;
}

/** Activation adopts the current iris request without sending an actuator command. */
public void armToggled(GCheckbox source, GEvent event)
{
    if (updatingWidgets)
    {
        return;
    }
    if (source.isSelected())
    {
        session.arm(hostMs());
        irisSlider.setValue(session.iris);
        irisField.setText(str(session.iris));
    }
    else
    {
        session.disarm();
    }
}

public void applyIris(GButton source, GEvent event)
{
    if (event == GEvent.CLICKED && session.armed && irisField.isValid())
    {
        session.setControls(irisField.getValueI(), session.zoom, session.x, session.y, hostMs());
    }
}

/** Editing stages a raw request only; Apply is the sole iris actuator action. */
public void irisFieldChanged(GTextField source, GEvent event)
{
    if (!updatingWidgets && source.isValid())
    {
        irisSlider.setValue(source.getValueI());
    }
}

public void irisSliderChanged(GSlider source, GEvent event)
{
    if (!updatingWidgets && source.hasFocus())
    {
        irisField.setText(str(source.getValueI()));
    }
}

/** Both release and completed-click events end held zoom; draw also catches off-button release. */
public void zoomChanged(GButton source, GEvent event)
{
    if (!session.armed)
    {
        return;
    }
    if (event == GEvent.PRESSED)
    {
        int direction = source == zoomPlus ? 1 : -1;
        session.setControls(session.iris, direction * speedSlider.getValueI(),
            session.x, session.y, hostMs());
    }
    else if (event == GEvent.RELEASED || event == GEvent.CLICKED)
    {
        session.releaseZoom(hostMs());
    }
}

public void speedChanged(GCustomSlider source, GEvent event)
{
    if (!updatingWidgets && session.armed && session.zoom != 0)
    {
        int direction = session.zoom > 0 ? 1 : -1;
        session.setControls(session.iris, direction * source.getValueI(),
            session.x, session.y, hostMs());
    }
}

public void stopClicked(GButton source, GEvent event)
{
    if (event == GEvent.CLICKED)
    {
        if (benchRunning)
        {
            cancelBench();
        }
        session.disarm();
        xyEnabled = false;
        syncAxes(0, 0);
    }
}

/** Enter XY mode centered; leaving it sends a zero-offset request and retains iris/zoom. */
public void xyToggled(GCheckbox source, GEvent event)
{
    if (updatingWidgets)
    {
        return;
    }
    xyEnabled = source.isSelected() && session.armed;
    syncAxes(0, 0);
    if (!xyEnabled && session.armed)
    {
        session.setControls(session.iris, session.zoom, 0, 0, hostMs());
    }
}

/** Preserve the archived pad orientation: positive screen Y is negative command Y. */
public void padChanged(GSlider2D source, GEvent event)
{
    if (!updatingWidgets && xyEnabled && source.hasFocus())
    {
        int x = source.getValueXI();
        int y = -source.getValueYI();
        session.setControls(session.iris, session.zoom, x, y, hostMs());
        syncAxes(x, y);
    }
}

public void axisChanged(GSlider source, GEvent event)
{
    if (!updatingWidgets && xyEnabled && source.hasFocus())
    {
        int x = source == xSlider ? source.getValueI() : session.x;
        int y = source == ySlider ? -source.getValueI() : session.y;
        session.setControls(session.iris, session.zoom, x, y, hostMs());
        syncAxes(x, y);
    }
}

public void centerClicked(GButton source, GEvent event)
{
    if (event == GEvent.CLICKED && xyEnabled)
    {
        session.setControls(session.iris, session.zoom, 0, 0, hostMs());
        syncAxes(0, 0);
    }
}

void syncAxes(int x, int y)
{
    boolean wasUpdating = updatingWidgets;
    updatingWidgets = true;
    xyPad.setValueXY(x, -y);
    xSlider.setValue(x);
    ySlider.setValue(-y);
    updatingWidgets = wasUpdating;
}

public void rawToggled(GCheckbox source, GEvent event)
{
    if (!updatingWidgets)
    {
        rawEnabled = source.isSelected();
    }
}

/** Each recording gets a new path; stopping recording has no serial side effect. */
public void recordToggled(GCheckbox source, GEvent event)
{
    if (updatingWidgets)
    {
        return;
    }
    if (source.isSelected())
    {
        String name = "panel-" + Instant.now().toString().replace(':', '-') + ".txt";
        startRecording(sketchPath("../build/logs/" + name));
    }
    else if (captureWriter != null)
    {
        captureWriter.close();
        captureWriter = null;
    }
}
