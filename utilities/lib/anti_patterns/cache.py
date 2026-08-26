# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import json
import os
import sys
import tempfile
from hashlib import sha256
from pathlib import Path

from lib.error import UtilityError
from lib.path_safety import (
    read_safe_text,
    require_directory,
    require_safe_existing_file,
    require_safe_parent_directory,
)

from .model import Candidate, Finding, Suppression
from .rules import RULES_BY_IDENTIFIER


class AntiPatternCache:
    """Persistent cache for source-level anti-pattern findings."""

    _format_version = 1
    _maximum_size = 32 * 1024 * 1024

    def __init__(self, project_directory: Path) -> None:
        self.path = project_directory / ".cache" / "anti_patterns.json"
        self._fingerprint = self._scanner_fingerprint()
        self._records: dict[str, object] = {}
        self._dirty = False
        self._load()

    @staticmethod
    def file_signature(path: Path) -> tuple[int, int, int]:
        """Return a fast signature that changes when a local file is replaced or modified."""
        status = path.stat()
        return status.st_size, status.st_mtime_ns, status.st_ctime_ns

    def get(self, relative_path: Path, signature: tuple[int, int, int]) -> tuple[Finding, ...] | None:
        """Return cached source findings if the file signature still matches."""
        key = relative_path.as_posix()
        record = self._records.get(key)
        if not isinstance(record, dict) or record.get("signature") != list(signature):
            return None
        try:
            findings = tuple(self._decode_finding(relative_path, item) for item in record["findings"])
        except (KeyError, TypeError, ValueError):
            self._records.pop(key, None)
            self._dirty = True
            return None
        return findings

    def put(self, relative_path: Path, signature: tuple[int, int, int], findings: tuple[Finding, ...]) -> None:
        """Store source findings for one scanned file."""
        self._records[relative_path.as_posix()] = {
            "signature": list(signature),
            "findings": [self._encode_finding(finding) for finding in findings],
        }
        self._dirty = True

    def retain(self, relative_paths: set[Path]) -> None:
        """Discard records for sources that are no longer part of a complete scan."""
        retained_keys = {path.as_posix() for path in relative_paths}
        stale_keys = self._records.keys() - retained_keys
        if not stale_keys:
            return
        for key in stale_keys:
            del self._records[key]
        self._dirty = True

    def save(self) -> None:
        """Atomically save a changed cache; cache failures never block the scanner."""
        if not self._dirty:
            return
        temporary_path: Path | None = None
        try:
            require_safe_parent_directory(self.path, "Anti-pattern cache")
            if self.path.parent.exists():
                require_directory(self.path.parent, "Anti-pattern cache directory")
            else:
                self.path.parent.mkdir(parents=True)
            require_safe_existing_file(self.path, "Anti-pattern cache", self._maximum_size)
            payload = {
                "format_version": self._format_version,
                "scanner_fingerprint": self._fingerprint,
                "files": self._records,
            }
            with tempfile.NamedTemporaryFile(
                mode="w", encoding="utf-8", dir=self.path.parent, prefix="anti_patterns_", delete=False
            ) as temporary_file:
                json.dump(payload, temporary_file, ensure_ascii=False, separators=(",", ":"), sort_keys=True)
                temporary_file.write("\n")
                temporary_path = Path(temporary_file.name)
            os.replace(temporary_path, self.path)
            temporary_path = None
            self._dirty = False
        except (OSError, TypeError, UtilityError, ValueError):
            return
        finally:
            if temporary_path is not None:
                temporary_path.unlink(missing_ok=True)

    def _load(self) -> None:
        """Load a compatible cache, ignoring missing, stale, or malformed data."""
        try:
            require_safe_existing_file(self.path, "Anti-pattern cache", self._maximum_size)
            if not self.path.exists():
                return
            payload = json.loads(read_safe_text(self.path, "Anti-pattern cache", self._maximum_size))
            if (
                not isinstance(payload, dict)
                or payload.get("format_version") != self._format_version
                or payload.get("scanner_fingerprint") != self._fingerprint
                or not isinstance(payload.get("files"), dict)
            ):
                self._dirty = True
                return
            self._records = payload["files"]
        except (json.JSONDecodeError, OSError, TypeError, UtilityError):
            self._dirty = True

    @classmethod
    def _scanner_fingerprint(cls) -> str:
        """Hash scanner implementation files so code changes invalidate all cached results."""
        digest = sha256()
        digest.update(f"python:{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}\n".encode())
        for path in sorted(Path(__file__).parent.glob("*.py"), key=lambda item: item.name):
            digest.update(path.name.encode())
            digest.update(b"\0")
            digest.update(path.read_bytes())
            digest.update(b"\0")
        return digest.hexdigest()

    @staticmethod
    def _encode_finding(finding: Finding) -> dict[str, object]:
        """Encode one source finding without configuration-derived suppression state."""
        inline_reason = None
        if finding.suppression is not None and finding.suppression.source == "inline":
            inline_reason = finding.suppression.reason
        return {
            "rule": finding.candidate.rule.identifier,
            "start": finding.candidate.start,
            "end": finding.candidate.end,
            "line": finding.line_number,
            "end_line": finding.end_line_number,
            "snippet": finding.snippet,
            "inline_reason": inline_reason,
        }

    @staticmethod
    def _decode_finding(relative_path: Path, record: object) -> Finding:
        """Decode and validate one cached source finding."""
        if not isinstance(record, dict):
            raise TypeError("Finding record must be a mapping.")
        identifier = record["rule"]
        if not isinstance(identifier, str) or identifier not in RULES_BY_IDENTIFIER:
            raise ValueError("Finding record has an unknown rule.")
        integer_fields = ("start", "end", "line", "end_line")
        if any(not isinstance(record.get(field), int) or record[field] < 0 for field in integer_fields):
            raise ValueError("Finding record has an invalid source coordinate.")
        snippet = record["snippet"]
        inline_reason = record["inline_reason"]
        if not isinstance(snippet, str) or inline_reason is not None and not isinstance(inline_reason, str):
            raise TypeError("Finding record has invalid text.")
        rule = RULES_BY_IDENTIFIER[identifier]
        suppression = Suppression("inline", inline_reason) if inline_reason is not None else None
        return Finding(
            candidate=Candidate(rule.info, record["start"], record["end"]),
            path=relative_path,
            line_number=record["line"],
            end_line_number=record["end_line"],
            snippet=snippet,
            suppression=suppression,
        )
