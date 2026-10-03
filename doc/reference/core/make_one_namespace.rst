.. index::
    single: Namespace; Flattening

******************
Make One Namespace
******************

Core automatically imports its API namespaces into the main ``erbsland`` namespace for consumers.
Include the public headers for the types you use; no separate namespace header is needed.
For example, including ``<erbsland/String.hpp>`` makes ``erbsland::text::String`` available as ``erbsland::String`` and
``el::String`` when the short namespace alias is enabled.

The standard-stream helper namespace ``erbsland::stream::io`` is exposed as ``erbsland::io`` and ``el::io``.
System-domain types are also available in ``erbsland``.
The nested machine-information namespace ``erbsland::system::info`` is exposed as ``erbsland::sys_info`` and
``el::sys_info`` instead of the ambiguous ``erbsland::info`` name.

Consumer Configuration
======================

Define ``ERBSLAND_CORE_DO_NOT_FLATTEN_NS`` on a consuming target to keep its APIs in their domain namespaces:

.. code-block:: cmake

    target_compile_definitions(example PRIVATE ERBSLAND_CORE_DO_NOT_FLATTEN_NS=1)

This is a consumer compile definition, not a Core CMake option.
Define it consistently for the target before any Core headers are processed.
The short namespace alias remains available, so explicit names such as ``el::text::String`` still work.

Library Compilation
===================

Core's own target privately defines ``ERBSLAND_CORE_BUILDING_LIBRARY`` and always compiles with explicit namespaces,
including in embedded and release builds.
The marker is not propagated to consumers.
Demos, maintenance tools, profiling tools, and test applications use the regular consumer view by default.

``Definitions.hpp`` selects the namespace view.
Its generated internal ``core/impl/MakeOneNamespace.hpp`` helper is not a public include header and must not be included
directly.
It rejects inclusion while compiling the Core library.
Generated aggregate headers follow the same policy through ``Definitions.hpp``.
