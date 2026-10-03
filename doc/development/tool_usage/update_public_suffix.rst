..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************
Update the Public Suffix List
*****************************

A Public Suffix List update replaces the checked-in ICANN and PRIVATE rule snapshot.
The generator prepares these rules using the separately pinned Unicode source.
Updating the list does not select a new Unicode release.

1. Acquire a Snapshot
=====================

Acquire an upstream snapshot into staging and preserve its exact bytes and license header.
Review the complete ICANN and PRIVATE sections and record its version, upstream commit when available, source, retrieval
date, and license in ``tool/data/public_suffix/README.md``.

2. Replace the Snapshot and Manifest
====================================

Replace ``public_suffix_list.dat`` and update its line-ending-independent SHA3-256 hash in ``manifest.elcl`` under
``[files]``.
Set ``[snapshot].version`` to the snapshot's ``// VERSION:`` header value.
If the filename or directory changes, update ``[public_suffix].input_file`` in ``tool/conf/source/public_suffix.elcl``.

Record File Hashes
------------------

Compute SHA3-256 after replacing CRLF and lone CR with LF.
Preserve all other bytes, including BOMs and the final line terminator.
Native tools use ``ProjectFiles::textChecksum(path, el::HashAlgorithm::Sha3_256)``.
For example, from a macOS/Linux shell at the repository root (on Windows, run the Python statements in
``.venv\Scripts\python.exe`` ):

.. code-block:: console

    .venv/bin/python3 - <<'PY'
    import hashlib
    from pathlib import Path
    source = Path("tool/data/public_suffix/public_suffix_list.dat")
    print(hashlib.sha3_256(source.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")).hexdigest().upper())
    PY

Store the uppercase hexadecimal result in the matching quoted filename entry under ``[files]``.
Manifests are parsed without placeholder expansion.

3. Regenerate and Verify
========================

The commands below use the macOS/Linux shortcut; on Windows use ``tool\bin\erbsland-core-tool.exe``.
Use the executable built from your current checkout when changing a generator alongside its inputs.
See :doc:`../tool_dev/build_and_test` for building the tool and running its tests.
With the installed tool up to date, run:

.. code-block:: console

    ./erbsland-core-tool generate-public-suffix-data
    cmake-build-debug/tool/test/unittest/erbsland-core-tool-unittest +tag:FullRun
    ./erbsland-core-tool generate-public-suffix-data --check

Run affected Core PSL and cookie tests and the repository's Python process tests.
Review canonical rule changes, storage budgets, and the generated diff.
Complete available platform builds, documentation checks, and ``pre-commit`` before accepting the update.
For reproducibility comparisons, keep the captured year, environment, and resolved rendering configuration constant.
