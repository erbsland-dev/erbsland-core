# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Exercise the same finite playback session shown in the walkthrough."""

from pathlib import Path
from subprocess import run
import sys
import tempfile


def main() -> None:
    binary = Path(sys.argv[1]).resolve()
    tracks = Path(__file__).resolve().parents[1] / "example-files"
    result = run(
        [binary, "--plain", "english.srt", "commentary.srt"],
        cwd=tracks,
        capture_output=True,
        text=True,
        timeout=10,
        check=True,
    )
    assert result.stdout.splitlines() == [
        "0 ms  start  english.srt #1",
        "    The tide is turning.",
        "400 ms  start  commentary.srt #1",
        "    Waves against the pier.",
        "800 ms  start  english.srt #2",
        "    Look beyond the rocks.",
        "    There is the lighthouse.",
        "1200 ms  end  english.srt #1",
        "1200 ms  end  commentary.srt #1",
        "1200 ms  start  commentary.srt #2",
        "    The foghorn sounds.",
        "2000 ms  end  english.srt #2",
        "2400 ms  end  commentary.srt #2",
    ], result.stdout
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "broken.srt"
        for content, location in [
            ("1\n00:00:01,000 --> 00:00:00,000\nBackwards\n", "line 2 (cue 1)"),
            ("1\n00:00:00,000 --> 00:00:00,001\nOK\n\n2\nwrong\n", "line 6 (cue 2)"),
            ("", "cue 1"),
        ]:
            path.write_text(content, encoding="utf-8")
            result = run([binary, "--plain", tracks / "english.srt", path], capture_output=True, text=True, timeout=5)
            assert result.returncode != 0
            diagnostic = result.stderr + result.stdout
            assert path.name in diagnostic and location in diagnostic, diagnostic
            assert "The tide is turning." not in diagnostic


if __name__ == "__main__":
    main()
