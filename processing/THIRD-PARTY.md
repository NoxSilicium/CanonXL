# Third-Party Dependencies

This directory contains owned panel sources, not redistributed dependency jars,
Processing runtimes or third-party source archives. The setup instructions fetch
libraries from their upstream distribution channels.

## G4P 4.3.11

Author: Peter Lager. Copyright (c) 2008+ Peter Lager.

G4P is licensed under LGPL-2.1-or-later, as stated in the source headers.
The [upstream archive](https://sourceforge.net/projects/g4p/files/G4P%20V4.3.11.zip/download)
includes both `G4P/library/G4P.jar` and its corresponding `G4P/src/` sources.
See the [GNU LGPL 2.1 text](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html).
The project uses an unmodified jar.

## jSerialComm 2.11.4

Author and copyright holder: Fazecast, Inc.

The [upstream release](https://github.com/Fazecast/jSerialComm/tree/v2.11.4)
offers Apache-2.0 and LGPL-3.0-or-later license options. CanonXL uses the Apache-2.0
option. Preserve the upstream [Apache license](https://github.com/Fazecast/jSerialComm/blob/v2.11.4/LICENSE-APACHE-2.0)
and [NOTICE](https://github.com/Fazecast/jSerialComm/blob/v2.11.4/NOTICE) when redistributing it.
Its corresponding sources are available from the upstream tag and the
[Maven source archive](https://repo.maven.apache.org/maven2/com/fazecast/jSerialComm/2.11.4/jSerialComm-2.11.4-sources.jar).
The project uses an unmodified jar.

## Processing 4

[Processing](https://processing.org/) is installed separately. Its runtime,
core library, bundled JDK and other components retain their upstream notices.
Consult the [Processing source and license files](https://github.com/processing/processing4)
before redistributing an exported application with its runtime dependencies.

## Redistribution

The CanonXL noncommercial restriction applies only to covered original project
material. It does not replace or restrict the licenses of these dependencies.
If you distribute an exported application, include the applicable notices and
meet each dependency's source and relinking requirements. Do not describe the
entire application bundle as covered solely by the CanonXL license.
