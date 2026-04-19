#!/usr/bin/env python3
"""Patch OTA image CRC into both ELF (.ota_image_info section) and BIN image.

CRC model matches ota_crc32() in firmware (IEEE CRC-32, reflected, polynomial
0xEDB88320, init/xor behavior equivalent to zlib.crc32).
"""

from __future__ import annotations

import argparse
import os
import struct
import subprocess
import tempfile
import zlib


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--elf", required=True)
    p.add_argument("--bin", required=True)
    p.add_argument("--objcopy", required=True)
    p.add_argument("--manifest-offset", required=True, type=lambda s: int(s, 0))
    p.add_argument("--magic", required=True, type=lambda s: int(s, 0))
    return p.parse_args()


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def main() -> int:
    args = parse_args()

    with open(args.bin, "rb") as f:
        image = bytearray(f.read())

    if len(image) < args.manifest_offset + 20:
        raise SystemExit("BIN too small to contain ota_image_info")

    manifest = image[args.manifest_offset : args.manifest_offset + 20]
    magic, fmt, hdr_size, fw_ver, image_size, image_crc = struct.unpack("<IHHIII", manifest)

    if magic != args.magic:
        raise SystemExit(f"Manifest magic mismatch: got 0x{magic:08X}, expected 0x{args.magic:08X}")

    if hdr_size < 20:
        raise SystemExit(f"Manifest header_size invalid: {hdr_size}")

    if image_size > len(image):
        raise SystemExit(f"Manifest image_size {image_size} exceeds BIN size {len(image)}")

    crc_field_abs = args.manifest_offset + 16
    crc_input = bytearray(image[:image_size])
    if crc_field_abs + 4 <= len(crc_input):
        crc_input[crc_field_abs : crc_field_abs + 4] = b"\x00\x00\x00\x00"

    crc = zlib.crc32(crc_input) & 0xFFFFFFFF

    image[crc_field_abs : crc_field_abs + 4] = struct.pack("<I", crc)
    with open(args.bin, "wb") as f:
        f.write(image)

    with tempfile.TemporaryDirectory() as td:
        sec_in = os.path.join(td, "ota_image_info.bin")
        run([args.objcopy, "--dump-section", f".ota_image_info={sec_in}", args.elf])

        with open(sec_in, "rb") as f:
            sec = bytearray(f.read())

        if len(sec) < 20:
            raise SystemExit("ELF .ota_image_info section too small")

        sec[16:20] = struct.pack("<I", crc)

        sec_out = os.path.join(td, "ota_image_info_patched.bin")
        with open(sec_out, "wb") as f:
            f.write(sec)

        run([args.objcopy, "--update-section", f".ota_image_info={sec_out}", args.elf])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
