.. index::
    single: Make One Namespace

******************
Make One Namespace
******************

The ``MakeOneNamespace.hpp`` header imports all API namespaces into the main ``erbsland`` namespace.
This allows user code to access types from any module without qualifying them with their full namespace.

For example, after including this header, you can write ``erbsland::String`` instead of ``erbsland::text::String``.
The standard-stream helper namespace ``erbsland::stream::io`` is folded as ``erbsland::io`` and therefore as ``el::io``
when the short namespace alias is enabled.
System-domain types are also folded into ``erbsland``.
The nested machine-information namespace ``erbsland::system::info`` is deliberately exposed as ``erbsland::sys_info``
and ``el::sys_info`` instead of the ambiguous ``erbsland::info`` name.

Collapsing namespaces is enabled by default.
It can be controlled using the ``ERBSLAND_CORE_DO_NOT_FLATTEN_NS`` macro.
When defined, the flattened namespace is not created.

For unit tests and development builds, the namespaces are not collapsed.
