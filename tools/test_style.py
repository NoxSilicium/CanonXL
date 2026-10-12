#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
# Copyright (c) 2026 Sebastien Marchand (NoxSilicium)
"""Verify strict aggregate layout without changing C++ tokens or literal contents."""

import re
import subprocess
import unittest

from style import expandInitializers, rootPath, tool


class InitializerLayoutTests(unittest.TestCase):
    """Exercise nested, empty and literal-containing aggregate initializer lists."""

    def testNestedAggregates(self) -> None:
        """Every nested opening brace adds exactly one four-space indentation level."""
        source = b"namespace xl\n{\n    const Frame value = {{1, 2}, {3, 4}};\n}\n"
        formatted = expandInitializers(source)
        self.assertIn(b"    const Frame value =\n    {\n        {\n            1, 2", formatted)
        self.assertEqual(re.sub(rb"\s", b"", source), re.sub(rb"\s", b"", formatted))

    def testEmptyAggregate(self) -> None:
        """An empty aggregate still has opening and closing braces on separate lines."""
        self.assertEqual(expandInitializers(b"Frame value = {};\n"), b"Frame value =\n{\n};\n")

    def testLiteralContents(self) -> None:
        """Raw-string indentation and braces inside strings are data, not formatting."""
        source = b'const char* values[] = {R"(first\n    second)", "{ untouched }"};\n'
        formatted = expandInitializers(source)
        self.assertIn(b'R"(first\n    second)"', formatted)
        self.assertIn(b'"{ untouched }"', formatted)

    def testFormattingIsIdempotent(self) -> None:
        """Running clang-format plus strict brace expansion twice must be stable."""
        source = b"namespace xl { const Frame value = {{1, 2, 3}, {}}; }\n"

        def formatSource(value: bytes) -> bytes:
            """Use the same clang configuration and post-pass as the maintenance command."""
            result = subprocess.run(
                [tool("clang-format"), "--assume-filename=canon_xl_controller/test.cpp"],
                input=value,
                capture_output=True,
                check=True,
                cwd=rootPath,
            )
            return expandInitializers(result.stdout)

        formatted = formatSource(source)
        self.assertEqual(formatSource(formatted), formatted)


if __name__ == "__main__":
    unittest.main()
