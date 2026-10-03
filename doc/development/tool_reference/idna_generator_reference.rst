..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************
IDNA Generator Reference
************************

``tool::idna::IdnaTool`` derives and serializes IDNA2008 attributes from the shared Unicode source.
See :doc:`../tool_usage/generate_idna_data` for the regeneration workflow.

Command Configuration
=====================

``[generate_idna_data]`` is required, with text setting ``output_file`` defaulting to
``src/erbsland/text/punycode/impl/IdnaData.cpp``.
Its checked-in file is ``tool/conf/tools/generate_idna_data.elcl``; an empty section uses the default.
``--output-file`` (``-o``, ``--output`` ) overrides this setting for one invocation.
``--check`` (``-c``) compares without writing and returns failure if output is missing or differs.
Output paths use the project file service; the parent directory must exist.
Comparison and replacement rules are specified in :doc:`framework_reference`.

.. _development-tool-reference-unicode-configuration:

Shared Unicode Configuration
----------------------------

Both IDNA and Public Suffix List generation require ``[unicode]`` from ``tool/conf/source/unicode.elcl``.
All settings are text:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Setting
      - Default and validation
    * - ``ucd_directory``
      - ``tool/data/ucd``; nonempty single-line path inside the selected project
    * - ``unicode_version``
      - Required, with no default; three decimal components separated by dots, matching the manifest and applicable headers

There are no command-line input overrides.
The directory contains ``manifest.elcl`` with ``[dataset].unicode_version`` and a ``[files]`` section mapping quoted
filenames to hexadecimal SHA3-256 hashes of normalized text.
Manifests are read without placeholder expansion.
Ordinary initialization verifies the thirteen production inputs listed below; conformance tests additionally verify
``NormalizationTest.txt``, ``Idna2008.txt``, and ``IdnaTestV2.txt``.

.. _development-tool-generate_idna_data-inputs-and-derivation:

Inputs and Derivation
=====================

The shared Unicode source verifies thirteen production inputs.
IDNA status and attribute derivation uses these eight: ``UnicodeData.txt``, ``Scripts.txt``, ``DerivedJoiningType.txt``,
``DerivedCoreProperties.txt``, ``PropList.txt``, ``Blocks.txt``, ``HangulSyllableType.txt``, and
``DerivedNormalizationProps.txt``.
The upstream ``extracted/DerivedJoiningType.txt`` is stored directly in the snapshot directory.

``CompositionExclusions.txt`` and ``IdnaMappingTable.txt`` prepare shared NFC normalization and UTS #46 mapping.
``CaseFolding.txt``, ``EastAsianWidth.txt``, and ``ReadMe.txt`` support the other shared Unicode consumers and the
release pin.
``UnicodeSource`` verifies all thirteen inputs and parses UnicodeData once for the consumers.

Input Integrity
---------------

``ReadMe.txt`` records the version; ``LICENSE.txt`` contains the Unicode license.
``manifest.elcl`` records SHA3-256 hashes of every generation input.
The manifest version, file hashes, and versioned property headers are validated before any data is derived.
UnicodeData has no version header and is verified by its hash.
Generation reads this subset directly; it does not import Python IDNA data or query Core's compiled Unicode tables.

Status Derivation
-----------------

The generator implements the rule ordering in `RFC 5892 <https://www.rfc-editor.org/rfc/rfc5892.html>`__: exceptions,
backward compatibility, unassigned characters, LDH, join controls, unstable characters, ignorable properties and blocks,
old Hangul Jamo, then eligible letters and digits.
The backward compatibility set is empty; RFC 6452 introduced no overrides.
Unassigned and disallowed characters share the runtime's zero encoding.
Contextual statuses remain distinct.

Unstable characters use ``Changes_When_NFKC_Casefolded`` from `Unicode 17 UAX #44
<https://www.unicode.org/reports/tr44/tr44-36.html#Property_Definitions>`__.
NFKC case folding also removes default ignorables.
Those characters are already rejected by the ignorable rule unless an earlier exception or join-control rule applies.
Official IDNA status verification covers every code point.

Source Attributes
-----------------

UnicodeData supplies assignment, general category, bidi class and canonical combining class (virama is class 9).
Scripts and joining types come directly from the corresponding UCD files.
Defaults are represented by zero bits.
The shared ``tool::unicode::UnicodeData`` reader supplies these records to both IDNA and PSL generation.
UnicodeData First/Last records must be paired, ordered and have matching properties; malformed code-point ranges,
categories and joining types are rejected.

.. _development-tool-generate_idna_data-compatibility-and-verification:

Compatibility and Verification
==============================

The runtime stores each set of attributes in 13 bits:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Bits
      - Attribute
    * - 0–1
      - IDNA status
    * - 2–5
      - Bidi class
    * - 6–8
      - Joining type
    * - 9
      - Virama flag
    * - 10–12
      - Selected script

Adjacent equal attributes are compacted within 64K pages.
The runtime lookup uses 16-bit page offsets.

Verification
------------

``PublicSuffixFullTest`` checks exhaustive IDNA statuses against the official ``Idna2008.txt`` matching the configured
Unicode version.
``IdnaDataFullTest`` reconstructs serialized range and page attributes for every code point and compares them with the
source-derived in-memory data.

A separate freshness test verifies deterministic rendering against the current generated source.
Focused tests cover malformed records, integrity errors and RFC precedence.
See :ref:`Run the tool tests <development-tool-setup-run-the-tool-tests>` for the ``FullRun`` checks.
