.. index::
    !single: Cryptographic Algorithms; Documentation

*************************************
Cryptographic Algorithm Documentation
*************************************

Cryptographic code must be written for transparent security review as well as functional correctness.

Specification Mapping
=====================

Keep an implementation in the same logical order as its defining specification wherever practical.
Prefer direct, readable transformations over compact or clever formulations.

API documentation for an algorithm implementation must name the governing specification and relevant section.
Before each substantive algorithm step, add an inline comment that identifies the corresponding specification section
and relates the specification's notation or formula to the variables and operations in the code.

Security-Relevant Decisions
===========================

Document security-relevant bounds, representation choices, precomputations, and deviations from optional parts of the
specification where they are enforced.
State unsupported optional features explicitly where their absence affects how callers use or review the implementation.

Secret Lifetime
===============

Keep secret-state lifetime and erasure behavior visible where secret intermediates are created, transferred, or
released.
Document ownership transfer, replacement order, exception cleanup, and terminal-state erasure at the corresponding code
locations.
Build replacement generations completely before installing them, then erase the replaced secret state immediately.
