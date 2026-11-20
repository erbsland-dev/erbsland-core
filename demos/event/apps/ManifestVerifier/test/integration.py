# Copyright (c) 2026 Tobias Erbsland - https:#erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Verify the report survives cancellation and file errors."""

from hashlib import sha256
from pathlib import Path
from subprocess import run
from tempfile import TemporaryDirectory
import sys


def main() -> None:
    binary = Path(sys.argv[1]).resolve()
    with TemporaryDirectory() as temp:
        directory = Path(temp)
        manifest = directory / "batch.sha256"
        data = b"shipment\n" * 20000
        digest = sha256(data).hexdigest().upper()
        for name in ("first", "second shipment", "third"):
            (directory / name).write_bytes(data)
        manifest.write_text("".join(f"{digest}  {name}\n" for name in ("first", "second shipment", "third")))

        def verify(flags, status, rows):
            result = run([binary, *flags, manifest], capture_output=True, text=True, timeout=5)
            assert result.returncode == status, result
            assert result.stdout.splitlines() == rows, result

        first, second, third = "OK        first", "OK        second shipment", "OK        third"
        verify([], 0, [first, second, third, "Checked: 3, mismatches: 0, unchecked: 0"])
        (directory / "second shipment").write_bytes(b"changed")
        second = "MISMATCH  second shipment"
        verify([], 1, [first, second, third, "Checked: 3, mismatches: 1, unchecked: 0"])
        verify(["--fail-fast"], 1, [first, second, "Checked: 2, mismatches: 1, unchecked: 1"])
        (directory / "first").unlink()
        result = run([binary, manifest], capture_output=True, text=True, timeout=5)
        assert result.returncode != 0 and "Checked: 0, mismatches: 0, unchecked: 3" in result.stdout, result
        manifest.write_text("bad checksum\n")
        result = run([binary, manifest], capture_output=True, text=True, timeout=5)
        assert result.returncode != 0 and "line 1" in result.stdout + result.stderr, result


if __name__ == "__main__":
    main()
