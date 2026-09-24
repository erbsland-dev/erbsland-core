.. index::
    !single: Archive Formats; Documentation

****************************
Archive Format Documentation
****************************

Archive code must be written for transparent security review as well as functional correctness.
An archive combines an externally controlled record graph, resource-expanding codecs, and filesystem paths, so format
validation and extraction policy are part of the security boundary.

Specification Mapping
=====================

Keep an implementation in the same logical record order as its defining specification wherever practical.
Prefer direct, bounded record transformations over compact parsing that obscures field boundaries or dependencies.

API documentation for an archive parser or writer must name the governing specification and relevant section.
ZIP implementation units use PKWARE APPNOTE 6.3.10 as their governing specification.
Before each substantive format step, add an inline comment that identifies the corresponding APPNOTE section and relates
specification field names to the implementation variables and checks.

Security-Relevant Decisions
===========================

Document security-relevant bounds, integer-overflow checks, path normalization, record consistency rules, checksum
validation points, representation choices, and deviations from optional parts of the specification where they are
enforced.
State unsupported optional records and features explicitly where their absence affects how callers use or review the
implementation.

Path and Record Safety
======================

Document how stored names become normalized relative paths and where root removal, Unicode normalization, component
validation, duplicate detection, and file/directory collision checks occur.
Record parsers must make offset ownership visible: explain how local entries, descriptors, central-directory data, and
end records are bounded and how overlaps, gaps, trailing records, and arithmetic overflow are rejected.

Integrity and Resource Lifetime
===============================

Keep checksum and declared-length validation visible at the point where decoded bytes cross into memory or the
filesystem.
Document ownership transfer and closure for archive streams, entry streams, temporary files, and reader-backed item
handles at their corresponding code locations.
For opaque-field or compressed-payload passthrough, identify which fields are retained unchanged, which managed fields
are regenerated, and which integrity checks are intentionally deferred until extraction.
