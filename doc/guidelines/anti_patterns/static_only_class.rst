********************************
Classes with Only Static Methods
********************************

:Rule ID: ``static_only_class``
:Severity: high

Classes that consist only of static functions and declarations are hacked namespaces.
They are bad practice and must not exist.

Scope and Exceptions
====================

A class with a virtual destructor is a polymorphic interface, even if its remaining methods are static factories.
The scanner does not report these classes.
Derived classes are also excluded because their inherited non-static behavior may not be visible in the declaration.

Correct Solution
================

* A collection of free functions can be put into a regular header with proper unique naming in its namespace.
* If the functions all share an explicit domain that would make their names repetitive, putting all functions in this
  header into a special namespace such as ``erbsland::<domain>::impl::<special>`` can make sense. At the usage location,
  this namespace may be imported for more readable code.
* Usually, this is a symptom of bad design. Functions should be arranged around data. Data can be values the functions
  operate on or simply configuration. For example, free functions such as ``doSomething(config, value)`` could be
  arranged around ``config``.

Mechanical Detection
====================

The scanner reports namespace-scope classes with static members but no instance state or non-static behavior.
