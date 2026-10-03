****************************
Misplaced Unit-Test Metadata
****************************

:Rule ID: ``misplaced_unittest_metadata``
:Severity: high

The unit-test metadata generator reads ``TESTED_TARGETS(...)``, ``TAGS(...)``, and ``SKIP_BY_DEFAULT()`` directly before
a test suite or method declaration.
Placing these macros inside a function body silently loses the tags and skip flag.
This can omit tests from a ``tag:FullRun`` run and unexpectedly include slow tests in the default run.

Keep all metadata together immediately before the declaration.
Put documentation and other comments before the metadata group:

.. code-block:: cpp

    /// Validate every pinned response record.
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllRecords() {
        // Test implementation.
    }

Mechanical Detection
====================

The scanner requires each metadata group to be followed by a ``class``, ``struct``, or ``void`` token.
It also reports metadata inside function bodies and comments or other tokens that interrupt a metadata group or separate
it from its declaration.
Comments, string literals, and preprocessing definitions containing macro names are ignored.
