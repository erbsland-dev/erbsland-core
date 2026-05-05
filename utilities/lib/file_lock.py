# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import hashlib
import os
import re
import sys
import tempfile
from pathlib import Path
from typing import BinaryIO

from lib.error import UtilityError

LOCK_NAME_RE = re.compile(r"^[-_a-zA-Z0-9.]+$")


class FileLock:
    """A blocking exclusive inter-process file lock."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.handle: BinaryIO | None = None

    def __enter__(self) -> "FileLock":
        self.path.parent.mkdir(parents=True, exist_ok=True)
        handle = self.path.open("a+b")
        self.handle = handle
        self.acquire(handle)
        handle.seek(0)
        handle.truncate()
        handle.write(f"{os.getpid()}\n".encode("ascii"))
        handle.flush()
        return self

    def __exit__(self, exc_type: object, exc_value: object, traceback: object) -> None:
        if self.handle is None:
            return
        try:
            self.release(self.handle)
        finally:
            self.handle.close()
            self.handle = None

    @staticmethod
    def acquire(handle: BinaryIO) -> None:
        """Acquire the file lock."""
        if sys.platform == "win32":
            import msvcrt

            handle.seek(0, os.SEEK_END)
            if handle.tell() == 0:
                handle.write(b"\0")
                handle.flush()
            handle.seek(0)
            msvcrt.locking(handle.fileno(), msvcrt.LK_LOCK, 1)
            return

        import fcntl

        fcntl.flock(handle.fileno(), fcntl.LOCK_EX)

    @staticmethod
    def release(handle: BinaryIO) -> None:
        """Release the file lock."""
        if sys.platform == "win32":
            import msvcrt

            handle.seek(0)
            msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            return

        import fcntl

        fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


class ProjectFileLock(FileLock):
    """A named inter-process file lock scoped to one project directory."""

    def __init__(self, project_dir: Path, name: str) -> None:
        if LOCK_NAME_RE.fullmatch(name) is None:
            raise UtilityError(f"Invalid lock name: {name!r}.")
        digest = hashlib.sha256(project_dir.resolve(strict=False).as_posix().encode("utf-8")).hexdigest()[:16]
        lock_path = Path(tempfile.gettempdir()) / f"erbsland-core-{name}-{digest}.lock"
        super().__init__(lock_path)
