*************************
Missing API Documentation
*************************

:Rule ID: ``missing_api_documentation``
:Severity: medium

Our library has strict rules about what and how to document interfaces (see :doc:`../code_style` ).
These rules explicitly include implementation APIs that often lack the required documentation.

Even private methods profit from a brief one-line description, as a function is often not obvious from its name.

Mechanical Detection
====================

The scanner reports undocumented API declarations in ``*.hpp`` files.
Explicitly defaulted or deleted special members are covered by the :doc:`missing_default_group_comment` rule.
