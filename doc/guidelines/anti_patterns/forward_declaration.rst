******************************************
Forward Declarations at the Usage Location
******************************************

:Rule ID: ``forward_declaration``
:Severity: high

Forward declarations are useful for reducing dependencies and resolving circular references.

Why We Do Not Want Forward Declarations at the Usage Location
=============================================================

Scattering forward declarations throughout the code base makes maintenance harder by introducing hidden dependencies on
the original type.
When a type is renamed or removed, these declarations are easily forgotten, resulting in difficult-to-diagnose
compilation errors.

Correct Solution
================

In Erbsland Core, every forward declaration has its own header.

For a type ``Example`` declared in ``Example.hpp``, create ``Example_fwd.hpp`` containing only the forward declaration.
``Example.hpp`` *must* include ``Example_fwd.hpp`` so conflicting declarations become immediately visible.

Any code that only requires the forward declaration should include ``Example_fwd.hpp``.

Mechanical Detection
====================

The scanner reports namespace-scope class and struct forward declarations outside ``_fwd.hpp`` files.
A few false positives are expected.
