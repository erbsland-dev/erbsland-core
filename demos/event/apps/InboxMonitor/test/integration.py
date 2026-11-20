# Copyright (c) 2026 Tobias Erbsland - https:#erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Check a finite scan and arrivals observed by the same live watcher."""

from pathlib import Path
from queue import Queue
from subprocess import PIPE, Popen, run
from tempfile import TemporaryDirectory
from threading import Thread
import sys


def main() -> None:
    binary = Path(sys.argv[1]).resolve()
    with TemporaryDirectory() as temp:
        inbox = Path(temp)
        (inbox / "first").touch()
        (inbox / "nested").mkdir()
        (inbox / "nested" / "ignored").touch()
        result = run([binary, "--once", inbox], capture_output=True, text=True, check=True, timeout=5)
        assert result.stdout.splitlines() == ["Arrived  first", "Scan complete. Arrivals: 1"], result
        with Popen([binary, inbox], stdout=PIPE, text=True) as watcher:
            lines = Queue()

            def collect_output():
                for line in watcher.stdout:
                    lines.put(line.rstrip())

            reader = Thread(target=collect_output, daemon=True)
            reader.start()
            try:
                assert lines.get(timeout=5) == "Arrived  first"
                (inbox / "second").touch()
                assert lines.get(timeout=5) == "Arrived  second"
            finally:
                watcher.terminate()
                watcher.wait(timeout=5)
                reader.join(timeout=5)


if __name__ == "__main__":
    main()
