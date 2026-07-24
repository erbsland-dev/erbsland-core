# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Validate domain-specific API guideline pages."""

from __future__ import annotations

from pathlib import Path

from lib.api_guidelines import validate_api_guideline_text
from lib.error import UtilityError
from lib.utility import UtilityApp


class ApiGuidelinesApp(UtilityApp):
    """Validate domain-specific API guideline pages."""

    description = "Validate domain-specific API guideline pages."

    def run(self, argv=None) -> None:
        super().run(argv)
        guideline_directory = self.project_directory / "doc" / "guidelines" / "api"
        findings: list[str] = []
        paths = sorted(guideline_directory.glob("*.rst"), key=lambda item: item.as_posix().casefold())
        for path in paths:
            if path.name == "common.rst":
                continue
            text = path.read_text(encoding="utf-8")
            relative_path = path.relative_to(self.project_directory).as_posix()
            for issue in validate_api_guideline_text(text):
                findings.append(f"{relative_path}:{issue.line_number}: {issue.message}")
        if findings:
            raise UtilityError("Invalid API guideline pages:\n" + "\n".join(findings))


def main() -> None:
    raise SystemExit(ApiGuidelinesApp().main())


if __name__ == "__main__":
    main()
