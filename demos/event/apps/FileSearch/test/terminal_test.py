# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Exercise the real terminal protocol on POSIX without adding a scripted mode to the app."""

import errno
import fcntl
import os
from pathlib import Path
import pty
import re
import select
import struct
import subprocess
import sys
import tempfile
import termios
import time


class TerminalRun:
    def __init__(self, executable: Path, root: Path):
        self.master, self.slave = pty.openpty()
        fcntl.ioctl(self.slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
        self.original = termios.tcgetattr(self.slave)
        self.output = b""
        environment = dict(os.environ, TERM="xterm-256color")
        environment.pop("ERBSLAND_VIRTUAL_TTY", None)
        self.process = subprocess.Popen(
            [str(executable), str(root)],
            stdin=self.slave,
            stdout=self.slave,
            stderr=self.slave,
            env=environment,
        )

    def read(self, timeout: float = 0.05):
        if select.select([self.master], [], [], timeout)[0]:
            try:
                self.output += os.read(self.master, 65536)
            except OSError as error:
                if error.errno != errno.EIO:
                    raise

    def wait_for(self, text: bytes):
        deadline = time.monotonic() + 5
        while text not in self.output:
            self.read()
            if self.process.poll() is not None or time.monotonic() >= deadline:
                raise AssertionError(f"Missing {text!r}: {self.output[-3000:]!r}")

    def send(self, text: bytes):
        os.write(self.master, text)

    def finish(self, key: bytes, expected: str = ""):
        self.send(key)
        deadline = time.monotonic() + 5
        while self.process.poll() is None and time.monotonic() < deadline:
            self.read()
        assert self.process.poll() == 0, self.output[-3000:]
        self.read()
        restored = termios.tcgetattr(self.slave)
        # macOS can set PENDIN while returning buffered key input to canonical mode.
        # It is kernel bookkeeping, not an input setting owned by the application.
        restored[3] &= ~getattr(termios, "PENDIN", 0)
        original = self.original.copy()
        original[3] &= ~getattr(termios, "PENDIN", 0)
        assert restored == original, "Terminal input settings were not restored"
        assert b"\x1b[?1049l" in self.output, "Alternate screen was not left"
        tail = self.output.split(b"\x1b[?1049l", 1)[1]
        plain = re.sub(rb"\x1b\[[0-?]*[ -/]*[@-~]", b"", tail).replace(b"\r", b"")
        # restoreScreen() finishes the restored terminal line before optional path output.
        expected_output = "\n"
        if expected:
            expected_output += expected + "\n"
        assert plain.decode() == expected_output, (plain, expected_output)

    def close(self):
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait()
        os.close(self.master)
        os.close(self.slave)


def main():
    executable = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="file-search-terminal-") as temporary:
        root = Path(temporary)
        (root / "a.cpp").write_text("a")
        (root / "b.cpp").write_text("b")
        (root / "界.hpp").write_text("unicode")
        (root / "nested").mkdir()
        (root / "nested" / "deep.cpp").write_text("deep")
        (root / "shortcut").symlink_to(root / "nested", target_is_directory=True)
        run = TerminalRun(executable, root)
        try:
            run.wait_for(b"Search root")
            run.send(b"*.cpp")
            run.wait_for(b"Search complete.")
            assert b"Results: 2" in run.output
            run.send(b"\x1b[B")
            run.finish(b"\r", str(root / "b.cpp"))
        finally:
            run.close()

        run = TerminalRun(executable, root)
        try:
            run.wait_for(b"Search root")
            run.send(b"[")
            run.wait_for(b"Invalid glob:")
            run.send(b"\x7f" + "**/*.hpp".encode())
            run.wait_for("界.hpp".encode())
            run.finish(b"\x1b")
        finally:
            run.close()

        run = TerminalRun(executable, root)
        try:
            run.wait_for(b"Search root")
            run.send(b"**")
            run.wait_for(b"Search complete.")
            assert b"Results: 6" in run.output, "Symlink traversal duplicated results"
            fcntl.ioctl(run.slave, termios.TIOCSWINSZ, struct.pack("HHHH", 8, 40, 0, 0))
            run.wait_for(b"Resize to at least")
            run.finish(b"\x1b")
        finally:
            run.close()

        run = TerminalRun(executable, root)
        try:
            run.wait_for(b"Search root")
            run.finish(b"\r")
        finally:
            run.close()

        # A long traversal permits exit after progressive results, before the final outcome.
        for index in range(20000):
            (root / f"entry-{index:04}").touch()
        for key in (b"\x1b", b"\r"):
            run = TerminalRun(executable, root)
            try:
                run.wait_for(b"Search root")
                run.send(b"**")
                run.wait_for(b"a.cpp")
                assert b"Searching" in run.output and b"Search complete." not in run.output
                expected = ""
                if key == b"\r":
                    expected = str(root / "a.cpp")
                run.finish(key, expected)
            finally:
                run.close()
    print("FileSearch: six real-terminal interaction and restoration scenarios passed.")


if __name__ == "__main__":
    main()
