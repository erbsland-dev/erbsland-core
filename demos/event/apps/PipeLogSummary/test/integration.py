# Copyright (c) 2026 Tobias Erbsland - https:#erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Hold a real pipe open across report deadlines and split a severity prefix across reads."""

from pathlib import Path
from selectors import DefaultSelector, EVENT_READ
from subprocess import PIPE, Popen, run
import sys


def main() -> None:
    binary = Path(sys.argv[1]).resolve()
    with Popen([binary], stdin=PIPE, stdout=PIPE, stderr=PIPE) as process, DefaultSelector() as selector:
        try:
            selector.register(process.stdout, EVENT_READ)
            process.stdin.write(b"INFO ready\nER")
            process.stdin.flush()
            assert selector.select(timeout=5), "No report while the pipe was idle"
            assert process.stdout.readline() == b"Running  INFO=1 WARN=0 ERROR=0 OTHER=0\n"
            stdout, stderr = process.communicate(b"ROR failure\nWARN incomplete", timeout=5)
            assert process.returncode == 0, stderr
            assert stdout.splitlines()[-1:] == [b"Final  INFO=1 WARN=1 ERROR=1 OTHER=0"]
        finally:
            process.kill()
    for data, final in [
        (b"", b"Final  INFO=0 WARN=0 ERROR=0 OTHER=0"),
        (b"INFO " + b"x" * 20000 + b"\n\n?", b"Final  INFO=1 WARN=0 ERROR=0 OTHER=2"),
    ]:
        result = run([binary], input=data, capture_output=True, check=True, timeout=5)
        assert result.stdout.splitlines() == [final], result


if __name__ == "__main__":
    main()
