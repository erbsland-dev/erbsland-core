..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************************
Configuration Language Reference
********************************

This chapter describes the Erbsland Configuration Language framework in Erbsland Core.
The framework parses files and in-memory text into typed value trees, resolves included sources under an explicit access
policy, optionally validates signatures, and applies reusable validation rules.

All textual APIs use Core :cpp:type:`erbsland::text::String <erbsland::text::String>` values and file APIs use
:cpp:class:`erbsland::path::Path <erbsland::path::Path>`. Sources and parsed values use shared ownership so a
document can retain its source locations without requiring the parser or source stream to remain alive.

.. toctree::
    :maxdepth: 1

    access_control
    data
    document
    errors
    locations
    names
    parser
    signature_validation
    signing
    source
    source_resolution
    validation_rules
    values
