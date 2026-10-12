#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
# Copyright (c) 2026 Sebastien Marchand (NoxSilicium)
"""Maintain the standalone Arduino controller without laboratory dependencies."""

import argparse
import shutil
import subprocess
from pathlib import Path

rootPath = Path(__file__).resolve().parents[1]
cppPaths = sorted(
    path
    for directory in ["canon_xl_controller", "tests"]
    for path in (rootPath / directory).rglob("*")
    if path.suffix in {".cpp", ".hpp", ".h", ".ino"}
)


def run(command: list[str]) -> None:
    """Run one checked command in the repository root, inheriting output."""
    print("+ " + " ".join(command), flush=True)
    subprocess.run(command, cwd=rootPath, check=True)


def tool(name: str) -> str:
    """Resolve a configured executable, otherwise fail with a setup instruction."""
    executable = shutil.which(name)
    if executable is None:
        raise RuntimeError(
            f"{name} is missing; follow doc/development.md to install the pinned tools."
        )
    return executable


def expandInitializers(source: bytes) -> bytes:
    """Put aggregate braces on their own lines; clang-format keeps these compact."""
    import re

    import tree_sitter_cpp
    from tree_sitter import Language, Parser

    parser = Parser(Language(tree_sitter_cpp.language()))
    edits = {}
    bodies = []
    literals = []
    stack = [(parser.parse(source).root_node, None)]

    while stack:
        node, parentIndent = stack.pop()
        currentIndent = parentIndent

        if node.type in {"string_literal", "raw_string_literal", "char_literal"}:
            literals.append((node.start_byte, node.end_byte))

        if node.type == "initializer_list":
            if parentIndent is not None:
                currentIndent = parentIndent + 4
            else:
                statement = node.parent
                while statement.parent is not None and statement.type not in {
                    "declaration",
                    "field_declaration",
                    "expression_statement",
                    "return_statement",
                    "for_range_loop",
                }:
                    statement = statement.parent
                lineStart = source.rfind(b"\n", 0, statement.start_byte) + 1
                currentIndent = len(re.match(rb" *", source[lineStart:]).group())

            opening = node.start_byte
            closing = node.end_byte - 1
            before = opening
            while before > 0 and source[before - 1 : before] in {b" ", b"\t", b"\n"}:
                before -= 1
            edits[before, opening] = b"\n" + b" " * currentIndent

            first = opening + 1
            while first < closing and source[first : first + 1].isspace():
                first += 1
            if first == closing:
                edits[opening + 1, closing] = b"\n" + b" " * currentIndent
            else:
                edits[opening + 1, first] = b"\n" + b" " * (currentIndent + 4)
                last = closing
                while source[last - 1 : last].isspace():
                    last -= 1
                edits[last, closing] = b"\n" + b" " * currentIndent
                bodies.append((first, last, currentIndent + 4))

        stack.extend((child, currentIndent) for child in node.children)

    # Align continuation rows without touching token text or string contents.
    for match in re.finditer(rb"\n( +)(?=\S)", source):
        start, end = match.span(1)
        containing = [body for body in bodies if body[0] < start < body[1]]
        if (
            containing
            and not any(a <= start and end <= b for a, b in edits)
            and not any(a <= start < b for a, b in literals)
        ):
            innermost = min(containing, key=lambda body: body[1] - body[0])
            edits[start, end] = b" " * innermost[2]

    for (start, end), replacement in sorted(edits.items(), reverse=True):
        source = source[:start] + replacement + source[end:]

    return source


def formatCpp(isCheck: bool) -> None:
    """Apply clang-format plus the project's stricter aggregate-brace layout."""
    executable = tool("clang-format")
    errors = []

    for path in cppPaths:
        formatted = subprocess.run(
            [executable, str(path)], cwd=rootPath, check=True, capture_output=True
        ).stdout
        formatted = expandInitializers(formatted)
        if formatted != path.read_bytes():
            if isCheck:
                errors.append(str(path.relative_to(rootPath)))
            else:
                path.write_bytes(formatted)

    if errors:
        raise RuntimeError("C++ formatting differs: " + ", ".join(errors))
    print("C++ brace layout and indentation checked." if isCheck else "C++ sources formatted.")


def formatCode(isCheck: bool) -> None:
    """Format only maintained sources, or verify them without modification."""
    formatCpp(isCheck)
    run([tool("black"), *(["--check"] if isCheck else []), "tools"])


def checkNaming() -> None:
    """Check owned Python names and C++ declarations, allowing external API names."""
    import ast

    import tree_sitter_cpp
    from tree_sitter import Language, Parser

    errors = []
    for path in sorted((rootPath / "tools").glob("*.py")):
        for node in ast.walk(ast.parse(path.read_text())):
            name = (
                node.id
                if isinstance(node, ast.Name) and isinstance(node.ctx, ast.Store)
                else (
                    node.arg
                    if isinstance(node, ast.arg)
                    else (
                        node.name
                        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
                        else None
                    )
                )
            )
            if name and "_" in name and not name.startswith("__"):
                errors.append(f"{path.relative_to(rootPath)}:{node.lineno}: {name}")
    parser = Parser(Language(tree_sitter_cpp.language()))
    for path in cppPaths:
        source = path.read_bytes()
        if b"@file" not in source:
            errors.append(f"{path.relative_to(rootPath)}: missing Doxygen file description")
        if path.parent.name == "canon_xl_controller" and path.suffix == ".h":
            errors.append(f"{path.relative_to(rootPath)}: owned headers must use .hpp")
        stack = [parser.parse(source).root_node]
        while stack:
            node = stack.pop()
            stack.extend(node.children)
            if node.type in {"field_identifier", "identifier"} and node.parent is not None:
                name = source[node.start_byte : node.end_byte].decode()
                if node.parent.type in {"field_declaration", "init_declarator"}:
                    if name.endswith("_") or (
                        name.startswith("k") and len(name) > 1 and name[1].isupper()
                    ):
                        errors.append(
                            f"{path.relative_to(rootPath)}:{node.start_point.row + 1}: {name}"
                        )
    if errors:
        raise RuntimeError("\n".join(errors))
    print("Naming and Doxygen file checks passed.")


def analyse() -> None:
    """Check controller lint and naming without running hardware operations."""
    run([tool("ruff"), "check", "tools"])
    checkNaming()


def testCode() -> None:
    """Build warning-clean host models without requiring captures or PHP."""
    run([tool("python"), "tools/test_style.py"])
    run([tool("cmake"), "-S", ".", "-B", "build/host", "-DXL_SANITIZERS=ON"])
    run([tool("cmake"), "--build", "build/host", "--parallel", "4"])
    run([tool("ctest"), "--test-dir", "build/host", "--output-on-failure"])


def main() -> None:
    """Dispatch one explicit maintenance action; never enumerate or open USB devices."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["format", "check", "analyse", "test", "docs"])
    action = parser.parse_args().action
    if action == "format":
        formatCode(False)
    elif action == "check":
        formatCode(True)
        analyse()
    elif action == "analyse":
        analyse()
    elif action == "test":
        testCode()
    else:
        run([tool("doxygen"), "Doxyfile"])


if __name__ == "__main__":
    main()
