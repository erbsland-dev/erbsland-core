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
Class members are documented at their declarations inside the class.
Separate out-of-class member definitions in headers inherit that documentation and do not require another API block.
Members defined directly inside a class still require documentation at that definition.
Executable entry points in ``*.cpp`` files are outside this rule's scope, and demo files are excluded by configuration.
The code style also exempts program entry points and their forwarding demo functions from required API documentation.
Unit-test metadata macros may appear between an API documentation block and its declaration.
The optional ``Anti Patterns . Missing Api Documentation`` section supplies ``File Exemption Texts`` and
``Declaration Exemption Texts``.
These case-sensitive substring lists are empty when omitted.
File exemptions inspect class and struct headers and exempt the complete file; declaration exemptions apply only to the
matching API declaration.
The shipped configuration uses ``UnitTest`` for file exemptions and ``PrivateTag`` for declaration exemptions.
Separate helper headers remain subject to this rule unless they contain an exempting class header.
Comments and string literals do not activate these exemptions.
Explicitly defaulted or deleted special members are covered by the :doc:`missing_default_group_comment` rule.
