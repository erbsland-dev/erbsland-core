# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Exercise timer ownership and deadlines while another process changes a real file."""

from pathlib import Path
from queue import Empty, Queue
import os
import subprocess
import sys
import tempfile
from threading import Thread
import time


class Monitor:
    def __init__(self, executable: str, path: Path, *arguments: str) -> None:
        self.process = subprocess.Popen(
            [executable, str(path), "--interval-ms", "25", *arguments],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            bufsize=1,
        )
        self.lines: Queue[str | None] = Queue()
        self.output: list[str] = []
        self.reader = Thread(target=self.read_output, daemon=True)
        self.reader.start()

    def read_output(self) -> None:
        assert self.process.stdout is not None
        for line in self.process.stdout:
            self.lines.put(line.rstrip())
        self.lines.put(None)

    def __enter__(self) -> Monitor:
        try:
            self.expect("Monitoring file:")
        except BaseException:
            self.__exit__()
            raise
        return self

    def __exit__(self, *_: object) -> None:
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait(timeout=5)
        self.reader.join(timeout=5)
        assert self.process.stdout is not None
        self.process.stdout.close()

    def next_line(self) -> str | None:
        try:
            line = self.lines.get(timeout=10)
        except Empty:
            raise AssertionError(f"Timed out waiting for monitor output: {self.output}") from None
        if line is not None:
            self.output.append(line)
        return line

    def expect(self, prefix: str) -> None:
        line = self.next_line()
        assert line is not None and line.startswith(prefix), (prefix, self.output)

    def finish(self, *, success: bool = True) -> None:
        while self.next_line() is not None:
            pass
        code = self.process.wait(timeout=5)
        assert (code == 0) == success, (code, self.output)


def main() -> None:
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="file-activity-monitor-") as temporary:
        root = Path(temporary)
        path = root / "activity-é.log"
        path.write_text("Started\n", encoding="utf-8")

        # No changes: polling alone must not extend the quiet period.
        with Monitor(executable, path, "--idle-timeout", "1", "--timeout", "10") as monitor:
            monitor.expect("Inactivity timeout reached.")
            monitor.finish()

        # Timestamp-only and size changes both count as activity. Keep changing the file beyond
        # the original two-second deadline, observing each update before sending another.
        with Monitor(executable, path, "--idle-timeout", "2", "--timeout", "10") as monitor:
            for index in range(4):
                time.sleep(0.65)
                if index == 0:
                    stat = path.stat()
                    os.utime(path, ns=(stat.st_atime_ns, stat.st_mtime_ns + 2_000_000_000))
                else:
                    with path.open("a", encoding="utf-8") as stream:
                        stream.write(f"Update {index}\n")
                monitor.expect("Changed:")
            monitor.expect("Inactivity timeout reached.")
            monitor.finish()

        # Activity keeps the idle timer alive, but cannot extend the overall deadline.
        with Monitor(executable, path, "--idle-timeout", "2", "--timeout", "3") as monitor:
            changes = 0
            test_deadline = time.monotonic() + 10
            while True:
                assert time.monotonic() < test_deadline, monitor.output
                time.sleep(0.35)
                with path.open("a", encoding="utf-8") as stream:
                    stream.write("Still active\n")
                line = monitor.next_line()
                if line == "Overall deadline reached.":
                    break
                assert line is not None and line.startswith("Changed:"), monitor.output
                changes += 1
            assert changes >= 2, monitor.output
            monitor.finish()

        # A disappearance and replacement by a directory both fail through application cleanup.
        for replacement in (False, True):
            with Monitor(executable, path, "--idle-timeout", "5", "--timeout", "10") as monitor:
                path.unlink()
                if replacement:
                    path.mkdir()
                monitor.finish(success=False)
                assert "unavailable or no longer a regular file" in "\n".join(monitor.output), monitor.output
            if replacement:
                path.rmdir()
            path.write_text("Restarted\n", encoding="utf-8")

        def run(*arguments: str) -> subprocess.CompletedProcess[str]:
            return subprocess.run([executable, *arguments], capture_output=True, text=True, timeout=5)

        help_result = run("--help")
        assert help_result.returncode == 0 and "--idle-timeout" in help_result.stdout
        for option, value in (
            ("--interval-ms", "0"),
            ("--interval-ms", "-1"),
            ("--idle-timeout", "0"),
            ("--idle-timeout", "-1"),
            ("--timeout", "-1"),
        ):
            result = run(str(path), f"{option}={value}")
            assert result.returncode != 0 and "Monitoring file:" not in result.stdout, result
        for invalid_path in (root, root / "missing.log"):
            result = run(str(invalid_path))
            assert result.returncode != 0 and "Monitoring file:" not in result.stdout, result
        assert path.read_text(encoding="utf-8") == "Restarted\n"
    print("FileActivityMonitor: real activity, inactivity, overall deadline, file failures, and options passed.")


if __name__ == "__main__":
    main()
