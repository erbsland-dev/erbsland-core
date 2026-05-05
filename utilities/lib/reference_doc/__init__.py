# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Shared helpers for reference documentation utilities."""

from .core import (
    ApiEntry,
    ApiEntryKind,
    HeaderApi,
    HeaderScanner,
    ReferenceDocConfig,
    ReferenceDocGenerator,
    ReferenceGroup,
    camel_to_snake,
    clean_function_signature,
    has_function_declaration_terminator,
    is_function_signature_complete,
    split_identifier_words,
    strip_line_comment_and_literals,
)

__all__ = [
    "ApiEntry",
    "ApiEntryKind",
    "HeaderApi",
    "HeaderScanner",
    "ReferenceDocConfig",
    "ReferenceDocGenerator",
    "ReferenceGroup",
    "camel_to_snake",
    "clean_function_signature",
    "has_function_declaration_terminator",
    "is_function_signature_complete",
    "split_identifier_words",
    "strip_line_comment_and_literals",
]
