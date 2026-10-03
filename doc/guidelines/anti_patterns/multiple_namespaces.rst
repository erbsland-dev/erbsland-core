****************************************
Multiple Distinct Namespaces in One Unit
****************************************

:Rule ID: ``multiple_namespaces``
:Severity: high

A source unit should have one namespace owner.
Declaring distinct namespaces side by side mixes responsibilities, even when both names belong to the same domain.
Move declarations into their owning units, or place a small API-specific tag inside the class that consumes it.

Example
=======

.. rubric:: bad: two distinct sibling namespaces in one header

.. code-block:: cpp

    // File erbsland/cryptology/Example.hpp

    namespace erbsland::cryptology::unsafe { // ... bad!
        class Another {
            // ...
        };
    }

    namespace erbsland::cryptology {
        class Example {
            // ...
        };
    }

.. rubric:: correct: the class from the other namespace has its own unit.

.. code-block:: cpp

    // File erbsland/cryptology/unsafe/Another.hpp

    namespace erbsland::cryptology::unsafe {
        class Another {
            // ...
        };
    }

.. code-block:: cpp

    // File erbsland/cryptology/Example.hpp

    namespace erbsland::cryptology {
        class Example {
            // ...
        };
    }

Scope and Exceptions
====================

Reopening the same namespace is allowed.
A ``std`` block containing only definitions of ``std::hash`` or ``std::formatter`` specializations does not count as a
second namespace.
This exception does not cover unrelated declarations, helpers, primary templates, or empty ``std`` blocks.
Qualified specializations outside a namespace block also do not add another namespace declaration.

Anonymous namespaces and lexically nested namespace blocks are covered by :doc:`anonymous_namespace` and
:doc:`nested_namespace`.
Directory placement is checked independently by :doc:`namespace_in_wrong_unit`.
Intentional units with several namespace owners require a reasoned acceptance comment or configuration exception.

Mechanical Detection
====================

The scanner examines named top-level namespace blocks in headers, implementations, and template parts.
It reports the first declaration of each additional distinct namespace after the first eligible namespace.
Namespace names are compared after removing whitespace and comments around qualification separators.
Strings, comments, preprocessing directives, and namespaces inside ``ERBSLAND_OS`` conditional branches do not establish
namespace ownership.
The detector is lexical and does not evaluate other preprocessing conditions or expand macros.
