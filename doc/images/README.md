# Project Illustrations

`canon-xl-lens-spi-cover.jpg` is the project cover: a lens photograph with captured
SPI signals superimposed. It is reproduced unchanged from the owner's illustration.

`canon-xl-20x-asi678mc-adapter.jpg` shows the imaging assembly: an XL 20x lens,
a 1.6x extender, a 3D-printed adapter and a ZWO ASI678MC camera. The camera model
is visible on its housing. `canon-xl-asi-camera-adapter-cutaway.jpg` is a cutaway
design view of the adapter, not a dimensioned drawing or a printable mesh itself.
The corresponding STL is available in the [printable parts guide](../../hardware/README.md).

`canon-xl-20x-lunar-imaging-setup.jpg` shows the initial lunar imaging setup:
an XL 20x lens with a 1.6x extender on a tripod and a laptop displaying the Moon.
It documents the original experiments, not a test of the revised controller.

`canon-xl-20x-moon.jpg` is a lunar image from the initial imaging experiments,
before the current controller revision. The documented setup is an XL 20x lens,
a 1.6x extender and a Baader Continuum 10 nm filter. It is not evidence of a new
astronomical test with the revised firmware.

`canon-xl-16x-logic-analyser-bench.jpg` shows the historical observation setup:
the Canon camcorder with its modified 16x lens, a logic analyser and a MacBook Air
displaying the captured signals. The camcorder is the SPI master on that bench.

`usb-logic-analyser-24mhz-8ch.jpg` shows the USB analyser used for the historical
experiments. Its case is labelled 24 MHz and 8 channels; this is not a measured
sampling-rate result or proof of the manufacturer behind the visible branding.

`canon-xl-lens-front-view.jpg` shows the front optics. The exact lens model is
not established from this view alone.

`canon-xl-lens-rear-optics-closeup.jpg` shows the rear optical opening and the
visible diaphragm. It does not establish a calibrated aperture value.

`canon-xl-20x-support-rings.jpg` shows the XL 20x held by two printed support
rings on a telescope mount. The ring models are not among the three supplied STLs.

`canon-xl-20x-bench.jpg` and `classic-nano-bench.jpg` show the standalone 20x
and classic Nano control bench. The diagrams describe that control setup, not
the observation bench, the Nano RP2040 Connect or a physical Canon connector
pinout. Harness colors are project-specific.

`canon-xl-processing-control-panel.png` is a direct rendered capture of the rebuilt
Processing interface connected to the standalone 20x/Nano bench. Read-only state
and live XL/XLRAW telemetry are visible. It is not a simulation,
an optical measurement or a claim that the disabled focus command is implemented.

`canon-xl-lens-mount-wiring.jpg` shows the opened lens mount and its internal
connections. In the pictured orientation, the soldered wires run from violet
to brown, top to bottom, matching the functions in the [wiring guide](../wiring.md).
The brown lens output is not connected to the Nano. This is a photograph of the
actual assembly, not a universal Canon connector pinout.

`ubec-lens-power-regulator.jpg` is the regulator illustration from the initial
setup. The accompanying documentation identifies a UBEC-3A-7.4V supplied from
12 V. The photograph alone does not establish its electrical ratings or qualify
a replacement supply.

The observation, imaging, adapter, wiring, regulator, front-view and analyser
JPEG illustrations retain their original resolution and any ICC profile. EXIF,
XMP and IPTC metadata were removed without recompressing their image data.
Their decoded pixels are unchanged.

The support-ring and rear-optics photographs were exported from the original
HEIC files as full-resolution JPEGs with normalized orientation and retained ICC
profiles. These conversions involve JPEG encoding, unlike the lossless metadata
removal above. Their EXIF, XMP and IPTC metadata were removed too. Preserve the
metadata-free copies for publication.

## License

The project photographs, panel capture and diagrams are covered by the
[CanonXL Noncommercial License, version 1.0](../../LICENSE). Preserve attribution
and identify changes when redistributing or translating them. Product marks visible
in the photographs identify the equipment, not an affiliation with Canon.

## Editable Diagrams

`controller-overview.svg` and `classic-nano-wiring.svg` are standalone SVG files.
They contain no scripts, external fonts or linked images. English labels remain
real `<text>` elements, not outlined paths. Both have a responsive `viewBox` and
an opaque white background for predictable display on GitHub and the future website.

Every translatable `<text>`, `<title>` and `<desc>` has a stable `id` and a
matching `data-i18n` key. Preserve these keys when revising the drawings.
Technical identifiers such as D10, MOSI and GND remain unchanged.

For translated assets, replace the keyed text and set the root `xml:lang`.
For an inline SVG on the website, the localization layer can replace each keyed
element's `textContent`. An SVG displayed through `<img>` instead needs a separate
localized asset. No localization runtime is included here.

Keep font sizes readable rather than compressing long translations. Split labels
with `<tspan>` elements or enlarge the drawing when necessary; check the rendering
after each translation. Also translate the embedding page's image alternative text.

## Electrical Scope

The wiring illustration shows signal directions and separate lens power with
common ground. It does not qualify supply voltage, bus levels or a replacement
power supply. Follow the [wiring guide](../wiring.md) and measure the actual hardware
before connecting it. The lens 4.5 V output is deliberately left unconnected.
