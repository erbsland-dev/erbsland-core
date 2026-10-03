*******************************************
API Documentation on Member Implementations
*******************************************

:Rule ID: ``api_documentation_on_member_implementation``
:Severity: medium

Document each class member at its declaration inside the class.
Repeating its API documentation on a separate implementation creates two descriptions that can drift apart.
If an implementation block contains information missing from the declaration, move that information to the declaration
before removing the implementation block.
Use ordinary ``//`` comments for notes about the implementation algorithm.

Mechanical Detection
====================

The scanner reports ``///`` API blocks attached to out-of-class member definitions in ``*.hpp`` and ``*.tpp`` files.
It recognizes qualified member names using the library's PascalCase type naming convention.
Template parameter lists, constraints, and attributes may separate the documentation block from the definition.
The declaration may be in another header; detection does not depend on finding it in the same file.
Members defined directly inside their class and namespace-scope free functions remain valid documentation locations.
Regular implementation comments and definitions without API documentation do not trigger this rule.

Related Guidelines
==================

See :doc:`../code_style` for API documentation requirements and :doc:`missing_api_documentation` for missing declaration
blocks.
