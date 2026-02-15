#!/usr/bin/env python3
"""Parse AR-KD2 register map from HM-60506E manual chapter 8.

Expected source PDF: HM-60506E manual.
The parser extracts pages 227..242 (register address list) using `mutool` and
emits:
- CSV: dec, hex, name
- C++ header/source with lookup helpers.
"""

from __future__ import annotations

import argparse
import csv
import re
import subprocess
from pathlib import Path

NUM_RE = re.compile(r"^\d{1,5}$")
HEX_RE = re.compile(r"^[0-9A-F]{4}h$", re.IGNORECASE)


def _is_header(line: str) -> bool:
    return line in {
        "Register address list",
        "Register address",
        "Name",
        "Description",
        "READ/",
        "WRITE",
        "Setting range",
        "Initial value",
        "Update",
        "Dec",
        "Hex",
        "6 Method of control via Modbus RTU (RS-485 communication)",
    } or line.startswith("8-") or line.startswith("z ") or line.startswith("\ufffd")


def _is_setting_line(line: str) -> bool:
    if line in {"R", "W", "R/W", "A", "B", "C"}:
        return True
    if line.startswith("Refer to") or line.startswith("\u201cSetting range"):
        return True
    if re.search(r"\bto\b", line) and re.search(r"\d", line):
        return True
    if re.match(r"^[+\u2212\-]?\d", line):
        return True
    if ":" in line and re.search(r"\d", line):
        return True
    if line.startswith("* "):
        return True
    return False


def parse_registers_from_text(lines: list[str]) -> list[tuple[int, int, str]]:
    entries: list[tuple[int, int, str]] = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i].strip()
        if not NUM_RE.match(line):
            i += 1
            continue
        dec = int(line)
        if i + 1 >= n:
            break
        hex_line = lines[i + 1].strip()
        if not HEX_RE.match(hex_line):
            i += 1
            continue

        i += 2
        name_parts: list[str] = []
        while i < n:
            t = lines[i].strip()
            if not t:
                i += 1
                if name_parts:
                    break
                continue
            if NUM_RE.match(t):
                break
            if _is_header(t):
                i += 1
                continue
            if _is_setting_line(t) and name_parts:
                break
            if re.fullmatch(r"\d{3}", t):
                i += 1
                continue

            name_parts.append(t)
            i += 1

            if t.endswith("(upper)") or t.endswith("(lower)"):
                break

        name = " ".join(" ".join(name_parts).split())
        if not name:
            name = f"UNKNOWN_{hex_line.upper()}"

        entries.append((dec, int(hex_line[:-1], 16), name))

    by_dec: dict[int, tuple[int, str]] = {}
    for dec, addr, name in entries:
        prev = by_dec.get(dec)
        if prev is None or len(name) > len(prev[1]):
            by_dec[dec] = (addr, name)

    return [(dec, by_dec[dec][0], by_dec[dec][1]) for dec in sorted(by_dec)]


def emit_csv(rows: list[tuple[int, int, str]], out_csv: Path) -> None:
    out_csv.parent.mkdir(parents=True, exist_ok=True)
    with out_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["dec", "hex", "name"])
        for dec, addr, name in rows:
            writer.writerow([dec, f"{addr:04X}H", name])


def emit_cpp(rows: list[tuple[int, int, str]], out_hpp: Path, out_cpp: Path) -> None:
    out_hpp.parent.mkdir(parents=True, exist_ok=True)
    out_cpp.parent.mkdir(parents=True, exist_ok=True)

    out_hpp.write_text(
        """#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

struct ArKd2RegisterEntry {
  std::uint16_t address;
  std::string_view name;
};

// Full AR-KD2 register-address list parsed from HM-60506E manual chapter 8
// (pages 227-242, \"Register address list\").
std::span<const ArKd2RegisterEntry> arKd2FullRegisterMap();

std::optional<std::string_view> arKd2RegisterName(std::uint16_t address);
""",
        encoding="utf-8",
    )

    lines: list[str] = []
    lines.append("#include <ArKd2FullRegisterMap.hpp>\n")
    lines.append("#include <array>\n\n")
    lines.append("namespace {\n")
    lines.append(f"constexpr std::array<ArKd2RegisterEntry, {len(rows)}> kRegisters{{{{\n")
    for dec, addr, name in rows:
        escaped = name.replace("\\", "\\\\").replace('"', '\\"')
        lines.append(
            f"    ArKd2RegisterEntry{{0x{addr:04X}, \"{escaped}\"}}, // {dec}\n"
        )
    lines.append("}};\n")
    lines.append("}  // namespace\n\n")
    lines.append("std::span<const ArKd2RegisterEntry> arKd2FullRegisterMap() {\n")
    lines.append("  return kRegisters;\n")
    lines.append("}\n\n")
    lines.append(
        "std::optional<std::string_view> arKd2RegisterName(std::uint16_t address) {\n"
    )
    lines.append("  for (const auto& entry : kRegisters) {\n")
    lines.append("    if (entry.address == address) {\n")
    lines.append("      return entry.name;\n")
    lines.append("    }\n")
    lines.append("  }\n")
    lines.append("  return std::nullopt;\n")
    lines.append("}\n")

    out_cpp.write_text("".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pdf", required=True, type=Path)
    parser.add_argument("--pages", default="227-242")
    parser.add_argument(
        "--csv-out",
        default="Server/resources/ArKd2RegisterMap.csv",
        type=Path,
    )
    parser.add_argument(
        "--hpp-out",
        default="Server/include/ArKd2FullRegisterMap.hpp",
        type=Path,
    )
    parser.add_argument(
        "--cpp-out",
        default="Server/src/ArKd2FullRegisterMap.cpp",
        type=Path,
    )
    args = parser.parse_args()

    txt_path = Path("/tmp/ar_kd2_register_pages.txt")
    subprocess.run(
        [
            "mutool",
            "draw",
            "-F",
            "txt",
            "-o",
            str(txt_path),
            str(args.pdf),
            args.pages,
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    lines = txt_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    rows = parse_registers_from_text(lines)

    emit_csv(rows, args.csv_out)
    emit_cpp(rows, args.hpp_out, args.cpp_out)

    print(f"Parsed {len(rows)} register rows from pages {args.pages}.")
    print(f"CSV: {args.csv_out}")
    print(f"HPP: {args.hpp_out}")
    print(f"CPP: {args.cpp_out}")


if __name__ == "__main__":
    main()
