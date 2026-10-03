..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************
Pre-commit Reference
********************

``pre-commit`` runs a fixed ordered pipeline in the current process.
See :doc:`../tool_usage/pre_commit` for the review-and-rerun workflow and
:doc:`../tool_dev/framework` for task authoring.

Tasks and Defaults
==================

The command has no individual options or configuration section.
It uses shared project, configuration, verbosity, and log-file options.
Each participating command receives its own parsed default options and validated configuration immediately before
execution.

1. Capture initial Git state.
2. ``update-public-headers``
3. ``code-cleanup``
4. ``anti-patterns``
5. ``doc-api-guidelines``
6. ``doc-api-test-status``
7. ``doc-update-reference``
8. ``security-hashes``
9. Verify final Git state.

Normalization and generation are enabled.
Check-only flags, full-format overrides, and test-status normalization are not selected automatically.
Tasks run sequentially; existing worker pools inside each task retain their parallelism and caches.

Failure Contract
================

The runner catches Core ``RuntimeError`` exceptions from task preparation and execution.
The original diagnostic retains source locations and causes.
Errors logged by a normally returning task also fail that task; warnings alone do not.
Every subsequent task is still attempted, including final Git verification.
If baseline capture failed, final verification reports that comparison is unavailable.
Programming failures and exhausted resources retain the application's fatal behavior.
Project discovery, shared configuration parsing, and shared rendering validation precede the pipeline.

The final exit code is zero only when every task succeeds; otherwise it is one.
A missing, truncated, or undeliverable task log is a failure rather than a successful incomplete report.

Git State
=========

Snapshots compare NUL-separated porcelain-v1 status, binary-capable unstaged and staged diffs, and a SHA-256 aggregate
of untracked paths, types, and content fingerprints.
Regular files are hashed as exact bytes, including line endings and byte-order marks.
Symbolic links are hashed from their literal native targets without following or normalizing them.
Names outside Core's text repertoire, including a filename starting with U+FEFF, use a small native filesystem adapter.
Missing and other file types have distinct markers.
Ignored files are excluded using Git's usual rules.

Git is resolved from ``PATH`` and invoked without a shell using the shared restricted subprocess environment.
Machine output retains initial byte-order marks and NUL separators.
The external-command capture limit is eight MiB per stream; exceeding it fails the task instead of comparing partial
data.
No Git index writes, staging, commits, or rollback are performed by the runner.

Output and Logging
==================

An interactive terminal keeps completed tasks in history and refreshes the active row at most ten times per second.
Named phases use actual completed-item counts when totals are known, and an indeterminate indicator otherwise.
Rows are clipped using terminal display-cell widths.
Noninteractive output contains plain ``[RUN]``, ``[OK]``, and ``[FAIL]`` rows without animation or cursor controls.

Each task has an independent log manager that is drained before its completion row and diagnostics are printed.
Warnings and errors are retained by default; ``--verbose`` also displays information and trace entries.
The optional log file receives detailed output with task identifiers.
Successful tasks have no empty error section.
