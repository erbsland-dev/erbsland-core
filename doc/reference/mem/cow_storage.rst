.. index::
    single: Cow Storage

***********
Cow Storage
***********

Introduction
============

Cow Storage
-----------

:cpp:class:`CowStorage <erbsland::mem::CowStorage>` is a small copy-on-write helper for ordinary C++ data types.
It stores the data in a ``std::shared_ptr`` and always keeps a valid data object, so user code can work with references
instead of raw pointers.

Copying the storage shares the data object.
Mutable access through ``data()`` automatically detaches when the object is shared.
Calling ``detach()`` before a group of write operations is optional, but can make the copy-on-write point explicit.
Operations that create, replace, or detach data may throw allocation errors or exceptions from the stored type.

Separate storage objects may be copied, destroyed, and detached from different threads.
Concurrent access to the same storage object, or concurrent mutation of the same detached data object, still requires
external synchronization.

Example
~~~~~~~

.. code-block:: cpp

    using Storage = erbsland::mem::CowStorage<std::set<erbsland::text::Char>>;

    auto storage = Storage::from(std::set<erbsland::text::Char>{ch});
    auto copy = storage;

    storage.detach();
    storage.data().insert(otherChar);

Cow Manual Storage
------------------

:cpp:class:`CowManualStorage <erbsland::mem::CowManualStorage>` is a copy-on-write helper for code that wants
explicit writable access.
It stores the data in a ``std::shared_ptr`` and always keeps a valid data object.

Reading uses ``data()``.
Writing uses ``detachedData()``, which detaches first when the data object is shared.
This makes mutable call sites easy to find while avoiding nullable or raw pointer handling in user code.
Operations that create, replace, or detach data may throw allocation errors or exceptions from the stored type.

Use ``sharedDefault()`` when many storage instances can start with the same default-constructed data object.
The method keeps one shared default object for each data type and returns another owner for every call.
The retained canonical owner contributes to ``useCount()`` and ensures that writable access always detaches from the
default object.
Ordinary default construction remains unique and constructs a separate data object for each storage instance.

Shared Array Data
-----------------

:cpp:class:`SharedArrayData <erbsland::mem::SharedArrayData>` stores a small shared-data header and a trailing array in
one allocation.
The actual elements are placed in aligned storage immediately after the header, which keeps compact string-like storage
types cache-friendly while still using the intrusive reference-counting model.

Use this type when you build library internals that need copy-on-write array storage.
The allocation must be created, cloned, and destroyed through
:cpp:class:`SharedArrayData <erbsland::mem::SharedArrayData>` itself, because the header and trailing elements are one
memory block.

:cpp:enum:`SharedArrayDataCleanupMethod <erbsland::mem::SharedArrayDataCleanupMethod>` selects ordinary cleanup or
secure erasure for raw, trivially copyable arrays.
Secure allocations are zero-initialized across their complete element capacity and securely erased through an
optimizer-resistant platform backend before deallocation.
The final erase covers the one-block allocation in full: reference-count metadata, size and capacity fields, alignment
padding, used elements, and unused capacity.
Every copy-on-write allocation is erased independently when its final owner releases it.
If construction or cloning fails, already constructed elements are destroyed and the failed allocation follows the same
cleanup path before the exception is rethrown.

Headers that only store or pass a :cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` to shared array data
can include ``SharedArrayData_fwd.hpp``.
Constructors, destructors, copies, detach operations, and direct data access must be implemented in a source file that
includes ``SharedArrayData.hpp``.
This keeps the full allocation template out of dependent headers without adding a PImpl allocation or changing the
storage layout.

Shared Data
-----------

:cpp:class:`SharedData <erbsland::mem::SharedData>` is the base class for custom data blocks that participate in
Erbsland Core's intrusive shared-data model.
It places the :cpp:class:`ReferenceCounter <erbsland::mem::ReferenceCounter>` at the start of derived data and marks the
type as compatible with the shared-data traits.

Use this class only when you extend the library with a new shared storage type.
For ordinary application data, prefer the public value types that already manage their storage for you.

Shared Data Pointer
-------------------

:cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` is an intrusive copy-on-write pointer for supported
shared data objects.
It owns a pointer to data with an embedded :cpp:class:`ReferenceCounter <erbsland::mem::ReferenceCounter>`, increments
and decrements that counter as pointers are copied or destroyed, and destroys the allocation when the last reference is
released.

Copying the pointer shares the same data.
Mutable access automatically detaches shared data by cloning it, unless ``tManualDetach`` is enabled.
Manual detach mode is useful for code that wants to control exactly when copy-on-write materialization happens.

Separate :cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` instances may be copied, destroyed, reset,
and detached from different threads.
Concurrent access to the same pointer object, or concurrent mutation of the same already-detached data object, still
requires external synchronization.

Interface
=========

.. doxygenclass:: erbsland::mem::CowManualStorage
    :members:
.. doxygenclass:: erbsland::mem::CowStorage
    :members:
.. doxygenclass:: erbsland::mem::ReferenceCounter
    :members:
.. doxygenclass:: erbsland::mem::SharedArrayData
    :members:
.. doxygenenum:: erbsland::mem::SharedArrayDataCleanupMethod
.. doxygenenum:: erbsland::mem::SharedArrayDataConstructMethod
.. doxygenclass:: erbsland::mem::SharedData
    :members:
.. doxygenclass:: erbsland::mem::SharedDataPointer
    :members:
.. doxygenclass:: erbsland::mem::SharedVirtualData
    :members:
.. doxygenclass:: erbsland::mem::StorageIdentifier
    :members:
