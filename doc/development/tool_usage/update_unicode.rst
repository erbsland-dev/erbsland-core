..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************
Update Unicode Sources
**********************

A Unicode update changes the shared data used by Unicode, IDNA, Public Suffix List, and terminal frame generation.
Acquire and review a complete release before replacing the checked-in snapshot.
Source initialization only verifies and loads local files; it does not download updates.

1. Acquire a Release
====================

Acquire one official Unicode release into a staging directory, using its versioned UCD and IDNA distributions.
Keep downloaded bytes unchanged, including line endings, and retain upstream licenses and version information.
See ``tool/data/ucd/PROVENANCE.md`` for the origins of the current inputs.

2. Collect Production Inputs
============================

Collect the thirteen production inputs: ``UnicodeData.txt``, ``Blocks.txt``, ``DerivedCoreProperties.txt``,
``DerivedJoiningType.txt``, ``DerivedNormalizationProps.txt``, ``HangulSyllableType.txt``, ``PropList.txt``,
``Scripts.txt``, ``CompositionExclusions.txt``, ``IdnaMappingTable.txt``, ``CaseFolding.txt``, ``EastAsianWidth.txt``,
and ``ReadMe.txt``.
``DerivedJoiningType.txt`` comes from the UCD's ``extracted`` directory; keep its bytes unchanged when storing it
directly in the snapshot directory.

3. Collect Conformance Inputs
=============================

Also acquire the release's official ``NormalizationTest.txt``, ``Idna2008.txt``, and ``IdnaTestV2.txt``.
These are verification inputs, loaded through the manifest verifier only for conformance tests.
They are not required by ordinary source initialization.

4. Replace the Snapshot and Pin
===============================

Review provenance and licensing, replace the snapshot under ``tool/data/ucd``, and record line-ending-independent
SHA3-256 hashes for all thirteen production inputs and the three conformance inputs in ``manifest.elcl``.
Update ``[dataset].unicode_version`` and the explicit ``[unicode].unicode_version`` pin in
``tool/conf/source/unicode.elcl`` together.
Update the provenance record, ``ReadMe.txt``, and license material as appropriate.

There is no Unicode version default in C++.
The explicit configuration pin must match the manifest and applicable source headers.
A new release can generate data while linked Core still uses an earlier release: all Unicode-sensitive preparation uses
the selected snapshot.

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
    source = Path("tool/data/ucd/UnicodeData.txt")
    print(hashlib.sha3_256(source.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")).hexdigest().upper())
    PY

Store the uppercase hexadecimal result in the matching quoted filename entry under ``[files]``.
Manifests are parsed without placeholder expansion.

5. Regenerate and Verify
========================

The commands below use the macOS/Linux shortcut; on Windows use ``tool\bin\erbsland-core-tool.exe``.
Use the executable built from your current checkout when changing a generator alongside its inputs.
See :doc:`../tool_dev/build_and_test` for building the tool and running its tests.
With the installed tool up to date, run:

.. code-block:: console

    ./erbsland-core-tool generate-idna-data
    ./erbsland-core-tool generate-unicode-data
    ./erbsland-core-tool generate-category-test-data
    ./erbsland-core-tool generate-common-box-frame-style
    ./erbsland-core-tool generate-frame-border-joint-chars
    ./erbsland-core-tool generate-public-suffix-data
    cmake-build-debug/tool/test/unittest/erbsland-core-tool-unittest +tag:FullRun
    ./erbsland-core-tool generate-idna-data --check
    ./erbsland-core-tool generate-unicode-data --check
    ./erbsland-core-tool generate-category-test-data --check
    ./erbsland-core-tool generate-common-box-frame-style --check
    ./erbsland-core-tool generate-frame-border-joint-chars --check
    ./erbsland-core-tool generate-public-suffix-data --check

Run affected Core tests and the repository's Python process tests.
Review changes in properties, mappings, and generated tables before accepting a Unicode release.
Complete available platform builds, documentation checks, and ``pre-commit`` before accepting the update.
For reproducibility comparisons, keep the captured year, environment, and resolved rendering configuration constant.
