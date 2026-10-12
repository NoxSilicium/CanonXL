# Controller USB Protocol, Version 1

UART through the classic Nano USB bridge, or USB CDC on RP2040 Connect.
Open RP2040 with DTR active, not the 1200-baud bootloader trigger. Pending input
is purged on startup and RP2040 reconnection: wait for periodic state before
commands. If streaming is disabled, allow about 100 ms then retry STATUS until
state arrives. AVR port opening may reset through DTR. Application speed:
115200 baud. ASCII lines end LF; CRLF and surrounding spaces/tabs are accepted.
Commands are uppercase. Old Processing `F-number,zoom,x,y` lines are rejected;
no implicit physical-aperture conversion exists.

## Commands

| Line | Effect | Reply |
| --- | --- | --- |
| SET,iris,zoom,x,y | Atomically apply controls and renew lease | ACK,SET or ERR,not-ready |
| STOP | Zero zoom/X/Y, retain iris, cancel active expiry | ACK,STOP |
| RESET | Iris FF, zero motion, restart initialization, clear displayed feedback | ACK,RESET |
| STATUS | Request state | XL,1,... |
| STREAM,0 / STREAM,1 | Disable/enable 100 ms state stream | ACK,STREAM |
| RAW,0 / RAW,1 | Disable/enable 100 ms full MISO stream | ACK,RAW |

STREAM starts enabled, RAW disabled. Streams are independent. Only valid SET
renews the lease; STATUS/STREAM/RAW/invalid lines do not. RESET retains counters;
microcontroller reboot clears them.

| Control | Input | MOSI byte, 1-based |
| --- | --- | --- |
| Iris | Integer 0..255, decimal or 0xNN | 20, raw; FF closes |
| Zoom | Signed decimal -15..15, zero stops | 29, amplitude bits 2..5; positive direction 01, negative 10 |
| X | Signed decimal -5000..5000 | 21/22, signed 16-bit, high byte first |
| Y | Signed decimal -5000..5000 | 23/24, signed 16-bit, high byte first |

Amplitude +8 encodes hex21, -8 hex22, associated with increasing/decreasing
zoom feedback in available captures. Physical speeds and X/Y units are not
calibrated. Iris is not an F-number; feedback need not equal command.
Renew SET about every 100 ms to sustain motion. After 500 ms without renewal,
zero zoom/X/Y and retain iris. RP2040 USB closure stops sooner; AVR cannot
observe bridge disconnection. A connected but idle host still expires.

Maximum 95 characters before LF, excluding final CR. Complete-line validation
rejects floats, missing/extra fields, range errors and control characters with
ERR,line. A partial line stalled 250 ms produces ERR,line-timeout and ignores
its remainder through the next LF. Send a new complete line afterward.
ACK,SET acknowledges local acceptance, not lens receipt/physical position;
the next scheduled SPI exchange sends it. Bounded output can drop ACK/state
without blocking SPI; dropped counts this. Missing ACK does not prove rejection.

## XL,1 State

Exactly 23 fields, indexed from zero (unlike bus byte numbers):

| Index | Meaning |
| --- | --- |
| 0 | XL marker |
| 1 | Version 1 |
| 2 | Unsigned 32-bit milliseconds since startup |
| 3 | Phase init/probe/ready |
| 4 | Last reply none/init/normal/zero/ff/checksum/unknown |
| 5 | Requested raw iris, decimal |
| 6 | Requested encoded zoom byte, not signed input |
| 7 / 8 | Requested signed X / Y |
| 9 | Raw MISO 2 iris, or -1 |
| 10 | Raw MISO 4 controls, or -1 |
| 11 / 12 / 13 | Raw MISO 5 / 16 / 17, or -1 |
| 14 | Cumulative SPI exchanges |
| 15 | Bad checksums, excluding zero/ff replies |
| 16 / 17 | All-zero / all-FF replies |
| 18 | Valid checksum but header neither 01 nor 02 |
| 19 | Skipped SPI periods |
| 20 | Outgoing messages dropped for lack of space |
| 21 | SPI frequency obtained at peripheral initialization |
| 22 | Milliseconds since active SET, or -1 |

Fields 9..13 are exposed only for the latest valid normal reply; otherwise -1,
never stale measurements. Fields 5..8 are requests, not measured positions.
Unsigned 32-bit counters/time can wrap. Init sends 01; a valid 01 reply enters
probe sending 02; a valid 02 reply enters ready. After 100 init replies in probe,
restart initialization. Invalid feedback stops motion and revalidates dialogue;
no old motion resumes automatically.

## XLRAW,1 Full Response

Installed/read-validated on the Nano/20x; XL retains its 23 fields.
Exactly eight fields:

| Index | Meaning |
| --- | --- |
| 0 / 1 | XLRAW / version 1 |
| 2 | Unsigned 32-bit enqueue time in milliseconds |
| 3 | Cumulative exchange number, matching XL field 14 |
| 4 | Phase init/probe/ready |
| 5 | Reply init/normal/zero/ff/checksum/unknown |
| 6 | 1 if all 40 bytes sum to FF modulo 256, otherwise 0 |
| 7 | Exactly 80 uppercase hex characters: 40 MISO bytes in bus order |

First pair is MISO1, last MISO40 checksum. Preserve invalid/zero/FF/unknown
responses but check type/integrity before interpreting them. No raw output before
the first reply or between RESET and the next reply. This is latest-response
sampling at 10 Hz, not exhaustive 50 Hz SPI recording. Intermediate transactions
and brief events may be missed; enqueue time is not an electrical edge timestamp.
Both streams share a 512-byte nonblocking queue; raw mode changes no MOSI controls
or expiry. Same-cycle XL/XLRAW describe the same exchange but enqueue times may differ.

```text
STREAM,1
RAW,1
```

Stop raw with RAW,0; queued lines may still arrive. Identify markers XL/XLRAW/ACK/ERR,
not arrival order. End each capture with RAW,0 and restore STREAM as required.

## Stationary Example

```text
STREAM,0
STATUS
```

Wait for ready, then raw iris only:

```text
SET,0x70,0,0,0
STOP
STATUS
```

STOP retains iris `0x70` (112); RESET restarts with closed iris. Verify the actual lens
response before interpreting raw feedback as a physical setting.
