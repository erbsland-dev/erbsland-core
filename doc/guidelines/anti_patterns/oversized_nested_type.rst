**********************
Oversized Nested Types
**********************

:Rule ID: ``oversized_nested_type``
:Severity: high

A class or struct defined inside another class or struct must not exceed ten code lines.
The same limit applies when the nested type is forward-declared in its owner and defined later using a qualified name.
An oversized nested type usually means that ownership, responsibility, or the boundary between interface and
implementation has not been designed clearly enough.

Definition Size
===============

Count non-empty code lines from the ``class`` or ``struct`` keyword through the closing brace.
The declaration, inheritance list, braces, access labels, member declarations, and method implementations all count.
Blank lines, comment-only lines, and preprocessor-only lines do not count.
Ten code lines are allowed; the eleventh line is an anti-pattern.

Correct Solutions
=================

First identify why the nested type exists:

* If the outer class is only a thin wrapper around the nested type, merge the useful behavior into the outer class.
* If the nested class represents an independently meaningful value or service, move it into its own compilation unit.
  Place public types in the domain namespace and implementation-only types in the matching ``impl`` namespace.
* For representation or PImpl state owned by ``<domain>::Owner``, a separate ``<domain>::impl::Owner`` type is
  appropriate when it accurately represents the complete private implementation.
* If the owner already belongs to an ``impl`` namespace, choose a name that describes the extracted responsibility.
  Use ``OwnerData`` when the type specifically stores the owner's representation; do not add a generic ``Private`` or
  ``Impl`` suffix without a more precise meaning.

Some language protocols require a nested name.
For example, a coroutine return type must expose ``promise_type``.
Keep the required nested name as a short alias while moving the actual definition into an implementation unit:

.. code-block:: cpp

    template<typename tValue>
    class Generator {
    public:
        using promise_type = impl::GeneratorPromise<tValue>;
    };

Moving a large definition outside the owner without changing its nested identity does not resolve the design problem.

Mechanical Detection
====================

The scanner reports class and struct definitions that exceed the nested-type size limit.
