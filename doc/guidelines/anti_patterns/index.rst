**************************
Bad Code and Anti-Patterns
**************************

This catalog describes code that is considered bad practice and anti-patterns that must be avoided.
These patterns are often introduced under time pressure, through shortcuts, or due to lack of experience.
While they may appear convenient, they degrade code quality over time.
Whenever code is modified or housekeeping is performed, such patterns must be removed or—if truly unavoidable—clearly
documented, including *why* they cannot be avoided.

Severity
========

Each anti-pattern has one of these severities:

* **high**: Must not exist in our code base, unless it is a documented, well-reasoned exception.
* **medium**: Should not exist in our code base, but fixes can be postponed if the quality impact is low.
* **low**: Should not exist in our code base and can be fixed in dedicated housekeeping sessions.

Automated Scanner
=================

Run the agent-focused scanner from the project root:

.. code-block:: console

    .venv/bin/python3 utilities/run.py anti_patterns

The scanner reports the most severe findings first and limits its output to 20 report rows by default.
Within one severity, rows representing more findings are shown first.
Multiple missing API documentation or default-group comment findings in the same file are grouped into one row that
shows the first location and the number of additional findings.
Pass one or more project-relative files or directories to narrow the scan, use ``--limit`` to change the row limit, and
use ``--show-suppressed`` to audit accepted locations.
Each report ends with paths to the relevant pages in this catalog.

The scanner uses conservative mechanical heuristics.
A finding is a request to inspect the location, not proof that the code is wrong.
Findings inside conditional compilation blocks opened with an ``ERBSLAND_OS`` macro are ignored because these blocks
contain platform-specific integration code that must be inspected in its native API context.

Accepted Locations
==================

Prefer fixing a finding.
If a construct is unavoidable, accept only the narrowest possible scope and always explain why.
A single location can be accepted with an adjacent line comment before the construct or after its statement:

.. code-block:: cpp

    // anti-pattern: allow static_cast_void -- The discarded result is safe because validation already succeeded.
    static_cast<void>(operation());

    static_cast<void>(operation()); // anti-pattern: allow static_cast_void -- Validation already succeeded.

The marker must use the exact rule identifier and include a non-empty reason.
For a directory- or file-wide exception, add a rule entry to ``utilities/conf/anti_patterns.elcl``.
The excluded path is relative to the project root.
A directory path recursively covers its contents.
Central exceptions require a reason and remain visible with ``--show-suppressed``.

.. code-block:: text

    --*[ Rule . static_cast_void ]*--
    Excluded Path : "test/unittest/src"
    Reason        : "The guideline explicitly permits this construct in unit tests."

If an excluded path contains ``*`` or ``?``, it is matched as a case-sensitive path pattern against each
project-relative path.
Wildcards do not cross directory separators; use ``**`` to match any number of directories.

.. code-block:: text

    --*[ Rule . static_only_class ]*--
    Excluded Path : "src/erbsland/unit/*Unit.hpp"
    Reason        : "These unit traits define compile-time representation mappings."

Anti-Pattern Catalog
====================

.. toctree::
    :maxdepth: 1

    anonymous_namespace
    type_in_wrong_unit
    namespace_in_wrong_unit
    implementation_in_wrong_unit
    oversized_nested_type
    forward_declaration
    static_global_object
    static_only_class
    missing_api_documentation
    missing_default_group_comment
    multiple_types_in_header
    regular_string_literal
    static_cast_void
    unnecessary_conversion
    nested_namespace
