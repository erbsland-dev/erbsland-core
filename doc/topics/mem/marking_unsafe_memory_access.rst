.. index::
    single: Memory; Unsafe access
    single: Pointers; Unsafe memory
    single: UnsafeCharPtr
    single: UnsafeMemoryPtr

************************************
Marking Unsafe Memory Access in Code
************************************

Raw pointers are sometimes unavoidable at a platform API, a C interface, or a carefully isolated low-level routine.
The pointer alone does not describe how many elements are valid, whether the storage is still alive, whether writes are
allowed, or whether two regions overlap.
Erbsland Core's ``Unsafe...Ptr`` aliases make such a boundary conspicuous without changing its binary representation.

Why an Unsafe Name Helps
========================

A ``char*`` can point to one character, a terminated string, a fixed buffer, or invalid memory.
A ``void*`` says even less.
The compiler cannot prove the assumptions that turn an operation through either pointer into a valid one.
A small mismatch can cause an out-of-bounds access, a use-after-free, corrupted data, or disclosure of memory that does
not belong to the operation.

Naming the type ``Unsafe`` does not prevent any of these failures.
It creates a visible review marker.
A reader who encounters :cpp:type:`UnsafeCharPtr <erbsland::mem::UnsafeCharPtr>` or
:cpp:type:`UnsafeMemoryPtr <erbsland::mem::UnsafeMemoryPtr>` immediately knows to look for the missing guarantees:
lifetime, bounds, alignment, initialization, mutability, overlap, and, for text, termination and encoding.
Search tools can also find the boundaries without producing every harmless pointer used inside an implementation.

Keep the Unsafe Region Small
============================

The most useful pattern is to establish invariants with safer types before crossing the low-level boundary.
An owning container establishes lifetime, while ``std::span`` or an Erbsland byte span carries a range length.
Validate sizes and relationships while that information is still available, then pass the raw address only to the narrow
operation that requires it.
Do not store the unsafe pointer or let it escape unless the surrounding object's contract explicitly governs its
lifetime.

:cpp:type:`UnsafeConstMemoryPtr <erbsland::mem::UnsafeConstMemoryPtr>` and
:cpp:type:`UnsafeMemoryPtr <erbsland::mem::UnsafeMemoryPtr>` represent read-only and writable untyped memory.
:cpp:type:`UnsafeConstCharPtr <erbsland::mem::UnsafeConstCharPtr>` and
:cpp:type:`UnsafeCharPtr <erbsland::mem::UnsafeCharPtr>` do the same for ``char`` data.
The ``char8_t`` variants, :cpp:type:`UnsafeConstChar8Ptr <erbsland::mem::UnsafeConstChar8Ptr>` and
:cpp:type:`UnsafeChar8Ptr <erbsland::mem::UnsafeChar8Ptr>`, preserve the distinct UTF-8 code-unit type when an interface
uses it.

The following example keeps storage and sizes in arrays and spans.
Only two tiny functions accept unsafe pointers: one wraps an untyped memory copy and the other writes a text terminator.
Their callers establish enough destination capacity before extracting the addresses.

.. erbsland-demo::
    :source: mem/UnsafePointers/MarkedBoundary.cpp
    :exec: mem/unsafe_pointers --demo MarkedBoundary
    :source-sha256: 3eede240fb9780bc0a5b7a170eee6b6f43d030910f872eb1377ff040edb903ab

.. code-block:: cpp

    /// Mark and contain operations that cannot express memory bounds in their type.
    ///
    /// The `Unsafe...Ptr` aliases behave exactly like raw pointers; their names add
    /// no runtime checks. They make a low-level boundary visible in signatures so
    /// reviewers can verify lifetime, bounds, overlap, and termination assumptions.
    void copyBytes(
        const el::mem::UnsafeMemoryPtr destination, const el::mem::UnsafeConstMemoryPtr source, const std::size_t size) {
        std::memcpy(destination, source, size);
    }

    void terminateText(const el::mem::UnsafeCharPtr destination, const std::size_t index) {
        destination[index] = '\0';
    }

    void markedBoundary() {
        const auto source = std::array{'f', 'o', 'r', 'm'};
        auto destination = std::array<char, 5>{};

        // Safe containers establish the sizes before their pointers cross the boundary.
        const auto sourceView = std::span{source};
        auto destinationView = std::span{destination};
        copyBytes(destinationView.data(), sourceView.data(), sourceView.size_bytes());
        terminateText(destinationView.data(), sourceView.size());

        el::io::printLine("Copied concept    : "_el, destination.data());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Copied concept    : form

.. erbsland-demo-end::

Because these aliases are ordinary pointer aliases, existing C and operating-system APIs accept them without casts or
adapter objects.
That compatibility is intentional, but it also means the aliases add no runtime validation and cannot distinguish two
unsafe pointers from the corresponding raw pointer during overload resolution.
Use them as documentation in declarations, not as a reason to relax checks at the call site.

When you control both sides of an interface, prefer a value, reference, span, or dedicated view that expresses the
required guarantees directly.
Reserve an unsafe pointer for the smallest layer where those guarantees truly cannot be represented, and translate back
to safer types as soon as the boundary has been crossed.
