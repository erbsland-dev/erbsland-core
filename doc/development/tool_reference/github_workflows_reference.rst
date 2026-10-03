..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**************************
GitHub Workflows Reference
**************************

See :doc:`../tool_usage/github_workflows` for the user workflow and :doc:`framework_reference` for shared startup,
configuration loading, logging, and project confinement.

Options and Configuration
=========================

The default invocation is read-only.
``--fix`` permits repairs to every verified version comment.
Repeatable ``--update ACTION@VERSION`` changes every occurrence of the complete action path to ``latest`` or an exact
stable ``vMAJOR.MINOR.PATCH`` tag, including unpinned references, and repairs those occurrences' comments.
Identical repeated requests are accepted; conflicting versions for one action, unused actions, and unavailable requested
tags fail with option context.
Combining ``--fix`` and ``--update`` repairs comments for unselected actions too.

The required ``[GitHub Workflows]`` section has these settings:

.. list-table::
    :header-rows: 1

    * - Setting
      - Contract
    * - ``Workflow Directory``
      - Text; defaults to ``.github/workflows``. Must be an existing project-confined directory without links.
    * - ``Safe Binary Directories``
      - Required nonempty text list of absolute trusted directory roots. Missing host directories are allowed.
    * - ``Git Executable``
      - Text; defaults to ``git``. Required basename on PATH or absolute executable path.
    * - ``Zizmor Executable``
      - Text; defaults to ``zizmor``. Optional auditor resolved with the same trust policy.

Executable symlinks are resolved before testing trusted-directory containment.
Executables inside the project are rejected.
Commands receive separate arguments, closed stdin, a restricted environment, and bounded captured output.
Truncated output is an error.
Git credential prompting is disabled.
No Python interpreter or package is used.

Parsing and Verification
========================

Only regular ``.yml`` and ``.yaml`` files immediately inside the workflow directory are read.
Suffix comparison is ASCII case-insensitive.
Ordinary block-style, single-line ``uses`` scalars may be unquoted, single-quoted, or double-quoted.
Comments and YAML block-scalar contents are ignored.
Unsupported uses syntax is reported with its physical file and line; this is a targeted workflow scanner, not a general
YAML parser.

External names have ``owner/repository[/subpath]`` form.
Local ``./`` and ``docker://`` references are excluded.
External pins require exactly 40 hexadecimal digits.
Stable tags have three unpadded unsigned decimal components, each representable in 64 bits; abbreviated tags and
prereleases are excluded.

Git runs ``ls-remote --tags`` once per repository per invocation.
Annotated tags use the peeled commit; lightweight tags use their direct target.
Multiple stable tags for one SHA resolve to the numerically highest version.
The required comment is ``# ACTION@VERSION`` on the immediately preceding line with matching indentation.
The command reports newer stable versions without failing solely because an update exists.

An unavailable Git executable, failed remote query, malformed tag response, unknown SHA, unpinned action, or incorrect
comment makes the audit fail.
No edits are written until all requested changes and remaining references validate.
Original snapshots are rechecked before writing; each changed file uses the framework's atomic replacement.
Quoting, indentation, inline comments, UTF-8 byte order marks, newline bytes, and final-newline state are preserved.

Optional zizmor uses ``--offline --collect=workflows``.
Exit codes 0 and 11 through 14 are accepted; findings remain advisory.
A missing trusted zizmor produces a warning, while other termination states fail the command.

Verification
============

Native parser and catalog tests cover text preservation, unsupported forms, stable ordering, and peeled tags.
Command integration tests use compiled fake Git and zizmor executables and require no network access.
