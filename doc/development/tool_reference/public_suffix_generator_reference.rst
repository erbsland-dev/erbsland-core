..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*********************************
Public Suffix Generator Reference
*********************************

``tool::psl::PublicSuffixTool`` prepares and encodes the rules used by HTTP cookie domain checks.
See :doc:`../tool_usage/generate_public_suffix_data` for the regeneration workflow.

Configuration and Input Format
==============================

``[generate_public_suffix_data]`` is required; its text setting ``output_file`` defaults to
``src/erbsland/network/impl/http/cookie/PublicSuffixData.cpp``.
The checked-in file is ``tool/conf/tools/generate_public_suffix_data.elcl``; an empty section supplies the default.
``--output-file`` (``-o``, ``--output`` ) overrides it for one invocation.
``--check`` (``-c``) compares without writing and returns failure for missing or different output.
Output paths use the project file service and require an existing parent directory; comparison and replacement rules are
specified in :doc:`framework_reference`.

The required ``[public_suffix]`` section in ``tool/conf/source/public_suffix.elcl`` has one text setting,
``input_file``, defaulting to ``tool/data/public_suffix/public_suffix_list.dat``.
The required shared ``[unicode]`` section follows
:ref:`development-tool-reference-unicode-configuration`.
There are no command-line input overrides.

A sibling ``manifest.elcl`` contains ``[snapshot].version`` and a ``[files]`` entry whose quoted name matches the input
filename and whose value is its line-ending-independent SHA3-256 hash in hexadecimal.
The source verifies that hash, reads UTF-8 in tolerant mode with automatic BOM handling, and compares the version with
the snapshot's ``// VERSION:`` header.
Manifests are read without placeholder expansion.
Rule parsing requires the ICANN and PRIVATE section boundaries.

Unicode Independence
====================

The shared Unicode source verifies all thirteen production inputs by line-ending-independent hashes and applicable
versioned source headers, and reads UTF-8 in tolerant mode with automatic BOM handling before deriving data.
Its UnicodeData reader validates First/Last ranges and retains source locations.
``tool::idna::IdnaData`` supplies assignment, categories, bidi, joining, scripts, virama, and RFC 5892 statuses from
those sources to both generators.

Label Preparation
-----------------

PSL preparation applies official nontransitional UTS #46 mapping, then source-backed NFC and strict IDNA2008 validation.

NFC uses canonical decompositions, combining classes, composition exclusions, and algorithmic Hangul processing.
Working storage grows dynamically; decomposition cycles and malformed records are rejected.

Labels undergo contextual and bidi checks individually.
A-labels must decode to valid NFC Unicode and round trip canonically.
Empty labels and DNS length violations fail.

Core supplies pure Punycode arithmetic, strings, containers, hashing, rendering, and file services.
It supplies no Unicode property or normalization decisions in this pipeline.

Reproducibility
---------------

For identical source snapshots and resolved configuration, output is independent of Core's compiled Unicode version.
Reproducibility comparisons must hold the captured year, environment, and resolved rendering settings constant.
See :doc:`rendering_reference` for placeholder filters that depend on linked Core and for the inputs to keep constant
when comparing output.

Representation and Verification
===============================

``tool::psl::PublicSuffixList`` validates section boundaries, canonicalizes rules, deduplicates within each rule family,
and rejects cross-family overlaps with source diagnostics.
The parser stores rules with reversed label order.

Rule Encoding
-------------

``EncodedData`` encodes lexicographically sorted rules in independent 32-rule blocks.
Common-prefix lengths and content tokens use separate Huffman trees with deterministic frequency, minimum-token, and
creation-order tie breaking.

Branches are flattened in preorder; leaves carry bit 7, and the stream is MSB-first.
Singleton alphabets have a deterministic branch with duplicate leaves.

Payload Validation
------------------

Every generated payload is independently decoded and compared to every canonical rule and block end before rendering.
Representation limits and the 45,000-byte storage budget are enforced.
Full tests verify complete encoding and decoding, deterministic output, storage budgets, and current-output freshness.

Conformance Tests
-----------------

``UnicodeSourceFullTest`` checks official NFC conformance.
``PublicSuffixFullTest`` checks every code point against the official IDNA2008 category table and exercises applicable
nontransitional IDNA test vectors.

Synthetic newer-version fixtures change assignment, decomposition, category, and bidi properties to verify that
preparation follows source data.
Focused and process tests cover malformed inputs, compression boundaries, options, check mode, confinement, and output
preservation.
See :ref:`Run the tool tests <development-tool-setup-run-the-tool-tests>` for the exhaustive ``FullRun`` checks.
