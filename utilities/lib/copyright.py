# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Literal

from lib.config import read_elcl_file
from lib.error import UtilityError


HeaderKind = Literal["hpp", "cpp", "py", "include"]


def current_year() -> int:
    """Return the current calendar year for generated copyright headers."""
    return date.today().year


@dataclass(frozen=True)
class HeaderConfig:
    """Shared generated-file header templates."""

    hpp_copyright: str
    cpp_copyright: str
    py_copyright: str
    include_copyright: str
    cpp_warning: str
    py_warning: str

    @classmethod
    def read(cls, config_file: Path) -> "HeaderConfig":
        """Read the shared header configuration included by a tool config."""
        header_config = read_elcl_file(config_file)["header"]
        result = cls(
            hpp_copyright=header_config.get_text("hpp_copyright", default=""),
            cpp_copyright=header_config.get_text("cpp_copyright", default=""),
            py_copyright=header_config.get_text("py_copyright", default=""),
            include_copyright=header_config.get_text("include_copyright", default=""),
            cpp_warning=header_config.get_text("cpp_warning", default=""),
            py_warning=header_config.get_text("py_warning", default=""),
        )
        result.validate()
        return result

    def validate(self) -> None:
        """Validate required templates and placeholders."""
        self.validate_template(self.hpp_copyright, "hpp_copyright", "{year}")
        self.validate_template(self.cpp_copyright, "cpp_copyright", "{year}")
        self.validate_template(self.py_copyright, "py_copyright", "{year}")
        self.validate_text(self.include_copyright, "include_copyright")
        self.validate_template(self.cpp_warning, "cpp_warning", "{tool}")
        self.validate_template(self.py_warning, "py_warning", "{tool}")

    @staticmethod
    def validate_text(template: str, name: str) -> None:
        """Validate one configured header text."""
        if not template.strip():
            raise UtilityError(f"Missing Header.{name} configuration.")

    @staticmethod
    def validate_template(template: str, name: str, placeholder: str) -> None:
        """Validate one configured header template."""
        HeaderConfig.validate_text(template, name)
        if placeholder not in template:
            raise UtilityError(f"Header.{name} must contain the {placeholder} placeholder.")

    def source_header(
        self,
        kind: HeaderKind,
        *,
        tool: str,
        pragma_once: bool = False,
        year: int | None = None,
    ) -> str:
        """Create a generated source header for the given file kind."""
        if year is None:
            year = current_year()
        copyright_text = self.copyright_template(kind).format(year=year).rstrip()
        warning_text = self.warning_template(kind).format(tool=tool).rstrip()
        if kind == "include":
            return f"{copyright_text}\n{warning_text}"
        if pragma_once:
            copyright_text += "\n#pragma once"
        return f"{copyright_text}\n\n{warning_text}"

    def copyright_template(self, kind: HeaderKind) -> str:
        """Get the copyright template for a file kind."""
        if kind == "hpp":
            return self.hpp_copyright
        if kind == "cpp":
            return self.cpp_copyright
        if kind == "py":
            return self.py_copyright
        if kind == "include":
            return self.include_copyright
        raise UtilityError(f"Unknown generated header kind: {kind}")

    def warning_template(self, kind: HeaderKind) -> str:
        """Get the warning template for a file kind."""
        if kind in {"hpp", "cpp", "include"}:
            return self.cpp_warning
        if kind == "py":
            return self.py_warning
        raise UtilityError(f"Unknown generated header kind: {kind}")
