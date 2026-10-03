..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************************
Unicode Data Generator Reference
********************************

``tool::unicode::UnicodeDataTool`` generates the three runtime translation units from one verified UCD snapshot.
See :doc:`../tool_usage/generate_unicode_data` for the workflow.

Configuration and Integrity
===========================

The required ``[generate_unicode_data]`` section has text setting ``output_directory``, defaulting to
``src/erbsland/text/impl``.
``--output-directory`` (``-o``) overrides it for one run.
``--check`` (``-c``) compares every output without writing and fails if any is missing or differs.
The output directory must exist inside the project.

The shared ``[unicode]`` configuration follows :ref:`development-tool-reference-unicode-configuration`.
``UnicodeSource`` checks the explicit version pin, line-ending-independent SHA3-256 manifest hashes, applicable source
headers, and the version statement in ``ReadMe.txt``.
The generator uses ``UnicodeData.txt``, ``CaseFolding.txt``, ``EastAsianWidth.txt``, ``CompositionExclusions.txt``, and
``DerivedNormalizationProps.txt``.
The shared source also verifies the other production inputs before deriving tables.
It does not query Python Unicode data or Core's compiled Unicode tables.

Derivation and Runtime Representation
=====================================

Unicode Light derives categories, simple uppercase and lowercase deltas, simple ``C`` and ``S`` case folds, and display
widths.
East Asian ``W`` and ``F`` widths are two; combining marks, controls, and format characters take width zero.
Adjacent equal values become ranges.
The runtime retains an ASCII table, a range table, and at most 256 distinct case-delta triples.
The range table is limited to 4,579 entries including its terminal sentinel.

Normalization derives direct canonical and compatibility decompositions, canonical combining classes, and canonical
composition pairs.
Sparse 4,096-code-point pages index LEB128 byte streams.
The generator checks mapping length, decomposition cycles, duplicate composition pairs, recursive depth, and the
runtime's fixed buffers before rendering.
Encoded decomposition, combining-class, and composition streams are limited to 29,000, 2,100, and 4,700 bytes; combined
storage is limited to 36,000 bytes.

The source classes prepare and validate values; render templates under ``tool/resources/unicode`` own the generated C++
layout.
``UnicodeDataTool`` renders all three files before updating any destination.

Verification
============

``UnicodeDataToolTest`` checks source changes, malformed case folding, normalization encoding, and delta-table limits.
``UcdDatasetTest`` checks manifest, version, and file integrity.
Full source and conformance tests use the ``FullRun`` tag.
The affected Core text tests verify runtime use of the generated tables.
