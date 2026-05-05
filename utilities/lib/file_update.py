# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
import re
import tempfile
from collections.abc import Callable
from pathlib import Path

from lib.error import UtilityError
from lib.path_safety import read_safe_bytes, require_safe_existing_file, require_safe_parent_directory


def write_text_atomically(path: Path, content: str, encoding: str = "utf-8") -> None:
    """
    Write text using an atomic same-directory replacement.

    :param path: The target path.
    :param content: The text content to write.
    :param encoding: The text encoding.
    """
    fd, temp_name = tempfile.mkstemp(prefix=f".{path.name}.", suffix=".tmp", dir=path.parent)
    temp_path = Path(temp_name)
    try:
        with os.fdopen(fd, "wb") as temp_file:
            temp_file.write(content.encode(encoding))
            temp_file.flush()
            os.fsync(temp_file.fileno())
        os.replace(temp_path, path)
    except Exception:
        temp_path.unlink(missing_ok=True)
        raise


class FileUpdate:
    """Write files only when their meaningful content changed."""

    MAX_COMPARE_FILE_SIZE = 32 * 1024 * 1024
    RE_COPYRIGHT_YEAR = re.compile("(Copyright (?:\\(c\\)|\\u00a9) ?)(\\d{4}(?:-\\d{4})?)")

    def __init__(self, print_verbose: Callable[[str], None] | None = None, encoding: str = "utf-8") -> None:
        self.print_verbose = print_verbose
        self.encoding = encoding

    @classmethod
    def normalize_for_comparison(cls, text: str) -> str:
        """Normalize text before comparing generated file content."""
        return cls.RE_COPYRIGHT_YEAR.sub(r"\1????", text)

    @classmethod
    def has_changed(cls, path: Path, content: str, encoding: str = "utf-8") -> bool:
        """Test if a file differs from the proposed content, ignoring copyright year-only changes."""
        require_safe_existing_file(path, "Generated file", cls.MAX_COMPARE_FILE_SIZE)
        new_bytes = content.encode(encoding)
        if len(new_bytes) > cls.MAX_COMPARE_FILE_SIZE:
            max_size_mb = cls.MAX_COMPARE_FILE_SIZE // (1024 * 1024)
            raise UtilityError(f"Generated content is larger than {max_size_mb} MiB: {path}")
        if not path.exists():
            return True
        old_bytes = read_safe_bytes(path, "Generated file", cls.MAX_COMPARE_FILE_SIZE)
        if old_bytes == new_bytes:
            return False
        try:
            old_text = old_bytes.decode(encoding)
        except UnicodeDecodeError:
            raise UtilityError(f"Generated file is not valid {encoding}: {path}") from None
        return cls.normalize_for_comparison(old_text) != cls.normalize_for_comparison(content)

    def write_if_changed(self, path: Path, content: str) -> bool:
        """
        Write the target file only when its meaningful content changed.

        :param path: The target path.
        :param content: The generated file content.
        :return: `True` if the file was written.
        """
        if not self.has_changed(path, content, self.encoding):
            return False
        if self.print_verbose is not None:
            self.print_verbose(f"  overwriting changed file: {path}")
        require_safe_parent_directory(path, "Generated file")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding=self.encoding)
        return True
