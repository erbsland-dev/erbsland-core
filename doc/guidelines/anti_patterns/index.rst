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

    ./erbsland-core-tool anti-patterns

The scanner reports the most severe findings first and limits its output to 20 report rows by default.
Within one severity, rows representing more findings are shown first.
Multiple missing API documentation or default-group comment findings in the same file are grouped into one row that
shows the first location and the number of additional findings.
Pass one or more project-relative files or directories to narrow the scan, use ``--limit`` to change the row limit, and
use ``--show-suppressed`` to audit accepted locations.
``--jobs`` selects 1 to 64 file workers; the default is the bounded logical CPU count.
Each report ends with paths to the relevant pages in this catalog.

The scanner uses conservative mechanical heuristics.
A finding is a request to inspect the location, not proof that the code is wrong.
Findings inside conditional compilation blocks opened with an ``ERBSLAND_OS`` macro are ignored because these blocks
contain platform-specific integration code that must be inspected in its native API context.

Source-level findings are cached in ``.cache/anti_patterns.json``.
Unchanged files are not read or parsed again, while changes to a source file or to the scanner implementation
automatically invalidate the affected cached results.

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
For a directory- or file-wide exception, add a rule entry to ``tool/conf/tools/anti_patterns.elcl``.
The excluded path is relative to the project root.
Directory coverage must use an explicit recursive glob such as ``demos/**``.
Central exceptions require a reason and remain visible with ``--show-suppressed``.

.. code-block:: text

    --*[ Anti Patterns . Exception ]*--
    Path   : "test/unittest/src/**"
    Rules  : "static_cast_void", "regular_string_literal"
    Reason : "These test fixtures require the standard-library forms to exercise their boundaries."

Each exception requires a project-relative ``Path`` glob, a nonempty list of ``Rules`` identifiers, and a nonempty
``Reason``.
Group rules by path and explain the constraint for each rule in the shared reason.
A glob must match the complete project-relative file path; ``*`` does not cross separators and ``**`` may cross
directories.
Duplicate path/rule pairs and unknown rule identifiers are errors.
The catalog entry for ``unnecessary_conversion`` describes policy but has no automated detector.

Anti-Pattern Catalog
====================

.. toctree::
    :maxdepth: 1

    oversized_file
    anonymous_namespace
    type_in_wrong_unit
    namespace_in_wrong_unit
    implementation_in_wrong_unit
    oversized_nested_type
    forward_declaration
    static_global_object
    static_only_class
    missing_api_documentation
    api_documentation_on_member_implementation
    misplaced_unittest_metadata
    missing_default_group_comment
    multiple_types_in_header
    regular_string_literal
    erbsland_core_containers_strict
    named_constant_for_repeated_literals
    static_cast_void
    unnecessary_conversion
    nested_namespace
    multiple_namespaces
    unnecessary_unit_conversion
    unnecessary_type_construction
    namespace_noise
