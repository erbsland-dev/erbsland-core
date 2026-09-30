..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*********************
Data Formats Overview
*********************

Choose the wire format used by the other application or protocol.
Each parser accepts one complete document or value, provides a nonthrowing optional form and a diagnostic throwing form,
and accepts explicit parse limits.

JSON
====

:doc:`parsing_and_rendering_json` shows how to inspect, edit, and format a text value tree.

BSON
====

:doc:`reading_and_writing_bson` walks through parsing, building, navigating, and serializing BSON documents.
It also explains input limits, binary subtypes, millisecond timestamps, and preserved wire types.

CBOR and DAG-CBOR
=================

:doc:`reading_and_writing_cbor` covers binary value trees and the deterministic DAG-CBOR profile.
It explains supported tags, navigation, input limits, and the different rules for ordinary CBOR and DAG-CBOR.

XML
===

:doc:`reading_and_writing_xml` shows how to parse, build, navigate, and serialize XML documents.
It also explains mixed content, preserved markup, unresolved references, and input limits.
