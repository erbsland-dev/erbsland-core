#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Regenerate committed ZIP interoperability vectors with independent writers."""

from __future__ import annotations

import argparse
import io
import os
import shutil
import struct
import subprocess
import tempfile
import unicodedata
import zipfile
from dataclasses import dataclass
from pathlib import Path

FIXED_TIME = (2024, 2, 3, 4, 5, 6)
FIXED_TIMESTAMP = "2024-02-03T04:05:06"
FIXED_EPOCH = 1_706_933_106
ARCHIVE_COMMENT = "Erbsland ZIP interoperability – UTF-8"
FULL_NAMES = (
    "empty-dir/",
    "empty.txt",
    "ascii.txt",
    "all-bytes.bin",
    "nested/repetitive.bin",
    "utf8-ä/日本語.txt",
)
ASCII_NAMES = FULL_NAMES[:-1]
METHOD_NAMES = {
    zipfile.ZIP_STORED: "stored",
    zipfile.ZIP_DEFLATED: "deflate",
    zipfile.ZIP_BZIP2: "bzip2",
    zipfile.ZIP_LZMA: "lzma",
    zipfile.ZIP_ZSTANDARD: "zstandard",
}
PAYLOAD_KEYS = {
    "empty.txt": "empty",
    "ascii.txt": "ascii",
    "all-bytes.bin": "all-bytes",
    "nested/repetitive.bin": "repetitive",
    "utf8-ä/日本語.txt": "utf8",
}
PYTHON_METHODS = tuple(METHOD_NAMES)


@dataclass(frozen=True)
class ArchiveMetadata:
    producer: str
    version: str
    variant: str


class UnseekableWriter:
    """A write-only sink that forces zipfile to emit data descriptors."""

    def __init__(self) -> None:
        self.buffer = io.BytesIO()

    def write(self, data: bytes) -> int:
        return self.buffer.write(data)

    def tell(self) -> int:
        return self.buffer.tell()

    def flush(self) -> None:
        pass


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="directory that receives manifest.tsv, entries.tsv, and archives")
    return parser.parse_args()


def write_source_tree(root: Path) -> None:
    (root / "empty-dir").mkdir(parents=True)
    (root / "nested").mkdir()
    (root / "utf8-ä").mkdir()
    (root / "empty.txt").write_bytes(b"")
    (root / "ascii.txt").write_bytes(b"The quick brown fox jumps over the lazy dog.\n")
    (root / "all-bytes.bin").write_bytes(bytes(range(256)))
    (root / "nested" / "repetitive.bin").write_bytes((b"abc123" * 1024) + bytes(range(64)))
    (root / "utf8-ä" / "日本語.txt").write_bytes("Grüezi – こんにちは\n".encode())
    for path in sorted(root.rglob("*")):
        path.chmod(0o755 if path.is_dir() else 0o644)
        os.utime(path, (FIXED_EPOCH, FIXED_EPOCH), follow_symlinks=False)


def python_info(name: str, directory: bool) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, FIXED_TIME)
    info.create_system = 3
    info.external_attr = ((0o40755 if directory else 0o100644) << 16) | (0x10 if directory else 0)
    if not directory:
        info.comment = "entry comment".encode()
    return info


def write_python_entries(archive: zipfile.ZipFile, source: Path, force_zip64: bool = False) -> None:
    file_index = 0
    for name in FULL_NAMES:
        directory = name.endswith("/")
        info = python_info(name, directory)
        if directory:
            archive.writestr(info, b"", compress_type=zipfile.ZIP_STORED)
            continue
        method = PYTHON_METHODS[file_index % len(PYTHON_METHODS)]
        info.compress_type = method
        data = (source / name).read_bytes()
        if force_zip64:
            with archive.open(info, "w", force_zip64=True) as destination:
                destination.write(data)
        else:
            archive.writestr(info, data, compress_type=method, compresslevel=6)
        file_index += 1


def generate_python(output: Path, source: Path, metadata: dict[str, ArchiveMetadata]) -> None:
    path = output / "python-classic.zip"
    with zipfile.ZipFile(path, "w") as archive:
        archive.comment = ARCHIVE_COMMENT.encode()
        write_python_entries(archive, source)
    metadata[path.name] = ArchiveMetadata("CPython zipfile", platform_python_version(), "classic-mixed-methods")

    sink = UnseekableWriter()
    with zipfile.ZipFile(sink, "w") as archive:
        archive.comment = ARCHIVE_COMMENT.encode()
        write_python_entries(archive, source)
    path = output / "python-stream.zip"
    path.write_bytes(sink.buffer.getvalue())
    metadata[path.name] = ArchiveMetadata("CPython zipfile", platform_python_version(), "stream-data-descriptors")

    path = output / "python-entry-zip64.zip"
    with zipfile.ZipFile(path, "w", allowZip64=True) as archive:
        archive.comment = ARCHIVE_COMMENT.encode()
        write_python_entries(archive, source, force_zip64=True)
    metadata[path.name] = ArchiveMetadata("CPython zipfile", platform_python_version(), "forced-entry-zip64")


def platform_python_version() -> str:
    import platform

    return platform.python_version()


def run(command: list[str], *, cwd: Path | None = None, input_text: str | None = None) -> str:
    environment = os.environ.copy()
    environment["TZ"] = "UTC"
    result = subprocess.run(
        command,
        cwd=cwd,
        env=environment,
        input=input_text,
        text=True,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout


def first_version_line(command: list[str]) -> str:
    environment = os.environ.copy()
    environment["TZ"] = "UTC"
    result = subprocess.run(
        command,
        check=True,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return next(line.strip() for line in result.stdout.splitlines() if line.strip())


def generate_info_zip(output: Path, source: Path, metadata: dict[str, ArchiveMetadata]) -> None:
    executable = shutil.which("zip")
    if executable is None:
        raise RuntimeError("Info-ZIP 'zip' was not found")
    version = next(line.strip() for line in run([executable, "-v"]).splitlines() if line.startswith("This is Zip "))
    for method in ("store", "deflate"):
        path = output / f"infozip-{method}.zip"
        command = [executable, "-q", "-r", "-X", "-Z", method, str(path), *ASCII_NAMES, "-z"]
        run(command, cwd=source, input_text="Info-ZIP compatibility vector\n")
        metadata[path.name] = ArchiveMetadata("Info-ZIP", version, f"{method};stripped-extra-fields")


def generate_libarchive(output: Path, source: Path, metadata: dict[str, ArchiveMetadata]) -> None:
    executable = shutil.which("bsdtar")
    if executable is None:
        raise RuntimeError("libarchive 'bsdtar' was not found")
    version = first_version_line([executable, "--version"])
    for method in ("store", "deflate"):
        path = output / f"libarchive-{method}.zip"
        command = [
            executable,
            "-cf",
            str(path),
            "--format",
            "zip",
            "--options",
            f"zip:compression={method},zip:hdrcharset=UTF-8",
            *FULL_NAMES,
        ]
        run(command, cwd=source)
        normalize_libarchive_metadata(path)
        metadata[path.name] = ArchiveMetadata("libarchive bsdtar", version, f"{method};normalized-volatile-metadata")


def normalize_libarchive_metadata(archive_path: Path) -> None:
    data = bytearray(archive_path.read_bytes())

    def normalize_extra(offset: int, length: int) -> None:
        end = offset + length
        while offset < end:
            field_id, field_length = struct.unpack_from("<HH", data, offset)
            field_start = offset + 4
            field_end = field_start + field_length
            if field_end > end:
                raise RuntimeError(f"{archive_path.name}: malformed extra field")
            if field_id == 0x5455 and field_length >= 1:
                flags = data[field_start]
                value_offset = field_start + 1
                for bit in range(3):
                    if flags & (1 << bit):
                        if value_offset + 4 > field_end:
                            raise RuntimeError(f"{archive_path.name}: malformed extended timestamp")
                        struct.pack_into("<I", data, value_offset, FIXED_EPOCH)
                        value_offset += 4
            elif field_id == 0x7875 and field_length >= 3:
                value_offset = field_start + 1
                uid_length = data[value_offset]
                value_offset += 1
                data[value_offset : value_offset + uid_length] = bytes(uid_length)
                value_offset += uid_length
                gid_length = data[value_offset]
                value_offset += 1
                data[value_offset : value_offset + gid_length] = bytes(gid_length)
            offset = field_end

    with zipfile.ZipFile(archive_path) as archive:
        for info in archive.infolist():
            name_length, extra_length = struct.unpack_from("<HH", data, info.header_offset + 26)
            normalize_extra(info.header_offset + 30 + name_length, extra_length)

        end_offset = data.rfind(b"PK\x05\x06")
        if end_offset < 0:
            raise RuntimeError(f"{archive_path.name}: end record is missing")
        entry_count = struct.unpack_from("<H", data, end_offset + 10)[0]
        central_offset = struct.unpack_from("<I", data, end_offset + 16)[0]
        for _ in range(entry_count):
            if data[central_offset : central_offset + 4] != b"PK\x01\x02":
                raise RuntimeError(f"{archive_path.name}: central entry is missing")
            name_length, extra_length, comment_length = struct.unpack_from("<HHH", data, central_offset + 28)
            normalize_extra(central_offset + 46 + name_length, extra_length)
            central_offset += 46 + name_length + extra_length + comment_length

    archive_path.write_bytes(data)


def generate_rust(output: Path, source: Path, metadata: dict[str, ArchiveMetadata]) -> None:
    script_root = Path(__file__).resolve().parent
    manifest = script_root / "rust" / "Cargo.toml"
    run(
        [
            "cargo",
            "run",
            "--release",
            "--locked",
            "--manifest-path",
            str(manifest),
            "--",
            "generate-vectors",
            str(output),
            str(source),
        ]
    )
    metadata["rust-classic.zip"] = ArchiveMetadata("Rust zip crate", "8.6.0", "classic-mixed-methods")
    metadata["rust-stream.zip"] = ArchiveMetadata("Rust zip crate", "8.6.0", "stream-data-descriptors")
    metadata["rust-zip64.zip"] = ArchiveMetadata("Rust zip crate", "8.6.0", "forced-entry-zip64")


def normalized_name(name: str) -> str:
    return unicodedata.normalize("NFC", name.rstrip("/"))


def contains_zip64(archive_path: Path) -> bool:
    data = archive_path.read_bytes()
    if b"PK\x06\x06" in data:
        return True
    offset = 0
    while (offset := data.find(b"PK\x01\x02", offset)) >= 0:
        if offset + 46 > len(data):
            return False
        compressed, uncompressed = struct.unpack_from("<II", data, offset + 20)
        disk_start = struct.unpack_from("<H", data, offset + 34)[0]
        local_offset = struct.unpack_from("<I", data, offset + 42)[0]
        if compressed == 0xFFFFFFFF or uncompressed == 0xFFFFFFFF or disk_start == 0xFFFF or local_offset == 0xFFFFFFFF:
            return True
        offset += 4
    return False


def write_manifests(output: Path, metadata: dict[str, ArchiveMetadata]) -> None:
    manifest_lines = [
        "# ZIP compatibility archives generated by generate_vectors.py.",
        "# archive\tproducer\tversion\tvariant\tarchive-comment\tzip64",
    ]
    entry_lines = [
        "# Expected entries for the committed ZIP compatibility archives.",
        "# archive\tpath\tdirectory\tmethod\tcomment\ttimestamp\tpayload-case",
    ]
    for archive_name in sorted(metadata):
        archive_path = output / archive_name
        with zipfile.ZipFile(archive_path) as archive:
            archive_comment = archive.comment.decode("utf-8") if archive.comment else "-"
            producer = metadata[archive_name]
            manifest_lines.append(
                "\t".join(
                    (
                        archive_name,
                        producer.producer,
                        producer.version,
                        producer.variant,
                        archive_comment,
                        "yes" if contains_zip64(archive_path) else "no",
                    )
                )
            )
            for info in archive.infolist():
                if any(ord(character) > 127 for character in info.filename) and not info.flag_bits & 0x0800:
                    raise RuntimeError(f"{archive_name}:{info.filename} is not marked as UTF-8")
                directory = info.is_dir()
                if not directory:
                    data = archive.read(info)
                    source_name = unicodedata.normalize("NFC", info.filename)
                    payload_case = PAYLOAD_KEYS[source_name]
                    if data != expected_payload(payload_case):
                        raise RuntimeError(f"{archive_name}:{info.filename} payload differs from the source case")
                else:
                    payload_case = "directory"
                timestamp = f"{info.date_time[0]:04}-{info.date_time[1]:02}-{info.date_time[2]:02}T{info.date_time[3]:02}:{info.date_time[4]:02}:{info.date_time[5]:02}"
                entry_lines.append(
                    "\t".join(
                        (
                            archive_name,
                            normalized_name(info.filename),
                            "yes" if directory else "no",
                            METHOD_NAMES[info.compress_type],
                            info.comment.decode("utf-8") if info.comment else "-",
                            timestamp,
                            payload_case,
                        )
                    )
                )
    (output / "manifest.tsv").write_text("\n".join(manifest_lines) + "\n")
    (output / "entries.tsv").write_text("\n".join(entry_lines) + "\n")


def expected_payload(payload_case: str) -> bytes:
    if payload_case == "empty":
        return b""
    if payload_case == "ascii":
        return b"The quick brown fox jumps over the lazy dog.\n"
    if payload_case == "all-bytes":
        return bytes(range(256))
    if payload_case == "repetitive":
        return (b"abc123" * 1024) + bytes(range(64))
    if payload_case == "utf8":
        return "Grüezi – こんにちは\n".encode()
    raise RuntimeError(f"unknown payload case: {payload_case}")


def main() -> None:
    arguments = parse_arguments()
    output = arguments.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    for old_archive in output.glob("*.zip"):
        old_archive.unlink()
    metadata: dict[str, ArchiveMetadata] = {}
    with tempfile.TemporaryDirectory(prefix="erbsland-zip-vectors-") as temporary:
        source = Path(temporary) / "source"
        source.mkdir()
        write_source_tree(source)
        generate_python(output, source, metadata)
        generate_info_zip(output, source, metadata)
        generate_libarchive(output, source, metadata)
        generate_rust(output, source, metadata)
    write_manifests(output, metadata)


if __name__ == "__main__":
    main()
