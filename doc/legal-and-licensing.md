# Legal and Licensing Reference

Scope: the CanonXL repository, including the controller, control panel and printable models.
This France/EU reference explains the legal framework; it is not a legal opinion
or a patent freedom-to-operate certification. The project license is [LICENSE](../LICENSE).

## Independent Controller

CanonXL implements external lens communications using request templates and
response decoding derived from the project's observations and earlier prototype.
Bus messages are not executable Canon firmware. The repository distributes neither
a Canon firmware dump nor a copied manufacturer manual. Its host hardware models
are test implementations, not bundled Arduino/Pico SDK implementations.

## French Reverse-Engineering Law

### Product Observation and Trade Secrets

[Commercial Code Article L151-3](https://www.legifrance.gouv.fr/codes/article_lc/LEGIARTI000037266559)
recognizes independent discovery and observation, study, disassembly or testing
of a publicly available or lawfully possessed product as lawful ways of obtaining
a trade secret, subject to the contractual restrictions specified in that article.
Lawful acquisition does not itself authorize publishing confidential third-party
documents or copying protected material.

### Software Observation and Interoperability

[Intellectual Property Code Article L122-6-1](https://www.legifrance.gouv.fr/codes/article_lc/LEGIARTI000044365559)
distinguishes observation from reproduction/translation of software code.
Paragraph III permits a lawful user to observe, study or test software during
operations they are entitled to perform. Paragraph IV permits code reproduction
or translation indispensable to independently developed software interoperability
only under its conditions: authorized user, necessary information not already
readily available, and work limited to the necessary parts. It also limits use
and disclosure of the resulting information. Contrary contractual provisions
for paragraphs II, III and IV are void under paragraph VIII. The article must
not be interpreted as permitting interference with normal software exploitation
or unjustified harm to the author's legitimate interests.

CanonXL observes external bus exchanges and implements a controller. Observation
of those exchanges must not be confused with decompilation of Canon software.
An interoperability exception is not general permission to redistribute Canon
firmware or unrelated documentation.

### Functional Information Versus Protected Expression

The [CJEU judgment in SAS Institute, C-406/10](https://eur-lex.europa.eu/legal-content/FR/ALL/?uri=CELEX%3A62010CJ0406)
distinguishes software functionality, programming languages and data-file formats
from protected software expression. Original source code and original manual
expression may still be protected; other applicable rights are not eliminated.
This supports an independently written interoperability implementation, not a
blanket claim that everything about a protocol is free of rights.

### Protection Mechanisms

[Article L331-5](https://www.legifrance.gouv.fr/codes/article_lc/LEGIARTI000044259273)
states that a protocol, format or encryption method is not, by itself, a technical
protection measure as defined there. Its regime concerns protected works other
than software. Reading SPI traffic does not by itself establish circumvention
of such a measure. Future access-control or encrypted-firmware work requires a
separate legal assessment.

## Canon Name and Product Marks

Canon names identify the equipment concerned, not the publisher or an endorsement.
[Article L713-6](https://www.legifrance.gouv.fr/codes/article_lc/LEGIARTI000039381590)
allows certain fair commercial references to a trademark, including identifying
the trademark owner's products and necessary intended-use references.
Keep the independent-project statement visible. Do not reuse a Canon logo as the
project logo or imply that Canon has validated the controller or wiring.

## Patents and Physical Products

No patent freedom-to-operate clearance is claimed. A lens's age or a noncommercial
project license cannot prove that all relevant claims in all territories are
inapplicable.

[Articles L613-2 to L613-6](https://www.legifrance.gouv.fr/codes/id/LEGISCTA000006179056)
address claim scope, prohibited exploitation, supply of essential means,
private/noncommercial and experimental exceptions, and exhaustion. The private
exception is not automatically an exception for public distribution. An experimental
use must be assessed against the invention concerned.

For a commercial controller or paid service, have a patent professional examine
the actual implementation, claims, families, territories and current legal status.
The [EPO legal-status guidance](https://www.epo.org/en/service-support/faq/searching-patents/about-patent-information/how-can-i-find-out-if-particular)
explains where to check whether particular rights remain valid. This reference
does not provide a complete claim chart or national-register clearance.

## External Software Licenses

The repository does not vendor the Arduino board packages. The pinned builds in
[development.md](development.md) use external components with their own licenses:

| Inspected upstream file | Version | License stated in the file |
| --- | --- | --- |
| [AVR Arduino.h](https://raw.githubusercontent.com/arduino/ArduinoCore-avr/1.8.8/cores/arduino/Arduino.h) | 1.8.8 | LGPL-2.1-or-later |
| [Mbed Arduino.h](https://raw.githubusercontent.com/arduino/ArduinoCore-mbed/4.6.0/cores/arduino/Arduino.h) | 4.6.0 | LGPL-2.1-or-later |
| [Mbed PluggableUSBSerial.h](https://raw.githubusercontent.com/arduino/ArduinoCore-mbed/4.6.0/cores/arduino/USB/PluggableUSBSerial.h) | 4.6.0 | Apache-2.0 |

These spot checks are not a complete inventory of every linked core, runtime,
SDK, bootloader or toolchain file. The Mbed package must not be described as having
one uniform license. A CanonXL license cannot replace upstream terms.

Before distributing compiled firmware, inspect the actual linked components and
meet their notice, source and relinking obligations where applicable. An own-code
noncommercial restriction must not remove recipients' rights in LGPL components.
Check combined-work compatibility before distributing firmware; source-only
publication does not settle every binary-distribution obligation.

## CanonXL License

The [CanonXL Noncommercial License, version 1.0](../LICENSE) authorizes source
inspection, execution, modification and redistribution for its noncommercial
personal, educational and scientific purposes. Institutional funding, tuition and
salaries alone do not make permitted teaching or research commercial. Commercial
development, products and paid services require separate written permission.
The [license guide](licensing.md) details permitted uses, redistribution requirements
and commercial permission.

This is a source-available license, not a GPLv3 or open-source license. A restriction
on commercial uses is inconsistent with the business-use freedom in the
[Open Source Definition](https://opensource.org/osd). [GPLv3](https://spdx.org/licenses/GPL-3.0-only.html)
allows sale and does not permit adding that restriction.

The license covers only rights the Licensors control. It does not license Canon
rights or replace upstream licenses. Commercial permission for CanonXL code is
not patent clearance or permission from Canon. Electrical precautions remain in
[wiring.md](wiring.md); a legal notice does not qualify hardware safety.
