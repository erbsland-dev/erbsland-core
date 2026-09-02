.. index::
    single: Namespaces

**********
Namespaces
**********

The ``Namespaces`` header forward-declares all API namespaces within the ``erbsland`` namespace.
It is included by ``Definitions.hpp`` to provide forward declarations of every module namespace.

The library organizes its API into the following namespaces:

*   ``core`` – Platform detection, namespace forward declarations, and short namespace setup.
*   ``cryptology`` – Cryptographic algorithms, selection metadata, and stateful operations.
*   ``err`` – Exception types and error handling utilities.
*   ``log`` –  Logging infrastructure.
*   ``math`` – Integer math, saturating arithmetic, and type traits.
*   ``mem`` – Memory management, reference counting, and storage types.
*   ``stream`` – Stream interfaces for byte and text I/O, including ``stream::io`` standard-stream helpers.
*   ``system`` – Portable process, operating-system, environment, and subprocess facilities; machine-wide queries are
    grouped in ``system::info``.
*   ``text`` – String types, character handling, and Unicode support.
*   ``time`` – Time, date and duration types.
*   ``unit`` – Unit-tagged integer types for indexes, lengths, offsets, and ranges.
*   ``util`` – General utilities like hash helpers and enum flag support.
