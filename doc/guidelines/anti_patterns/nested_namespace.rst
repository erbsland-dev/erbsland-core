****************************************
Nested Namespace Blocks and End Comments
****************************************

:Rule ID: ``nested_namespace``
:Severity: low

Do not open a named namespace inside another named namespace block.
Use one qualified namespace declaration instead.

.. code-block:: cpp

    // bad
    namespace erbsland::example {
    namespace impl {

    }
    }

    // correct
    namespace erbsland::example::impl {

    }

The qualified form states the complete namespace directly, avoids unnecessary indentation, and removes intermediate
closing braces that are easy to mismatch during maintenance.

Namespace End Comments
======================

Do not add a comment after the closing brace of a namespace.

.. code-block:: cpp

    // bad
    namespace erbsland::example {

    } // namespace erbsland::example

    // correct
    namespace erbsland::example {

    }

End comments frequently become incorrect when a namespace is renamed or code is moved.
Modern development tools show the namespace containing a declaration, so the repeated name provides no reliable
navigation benefit.

Mechanical Detection
====================

The scanner reports nested named namespace blocks and comments on namespace closing braces.
