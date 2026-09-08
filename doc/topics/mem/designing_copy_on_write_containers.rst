.. index::
    single: Memory; Copy-on-write
    single: Copy-on-write; Custom containers
    single: SharedData
    single: SharedDataPointer
    single: SharedVirtualData
    single: SharedArrayData
    single: CowStorage
    single: CowManualStorage

*****************************************
Designing Custom Copy-on-Write Containers
*****************************************

A value type is pleasant to use because copies behave independently: changing one value does not unexpectedly change
another.
For a value that owns a large data structure, however, eagerly duplicating all storage on every copy can be needlessly
expensive.
Copy-on-write keeps the value semantics while postponing that duplication until one of the copies is actually changed.

This page explains the idea first, then develops four practical designs.
You will see how to build regular and polymorphic value types, how to keep an array header and its elements in one
allocation, and when the higher-level storage helpers are a better fit.

What Copy-on-Write Means
========================

Imagine a design board containing thousands of concepts.
Copying the board to create a variation does not immediately require another independent list because both boards still
contain identical data.
They can share one immutable-looking data object for as long as neither board changes.
When the variation receives a new concept, it first copies the shared object and then changes its private copy.
The original remains untouched.

This gives a copy-on-write value two distinct kinds of operation:

* A copy of the public value is cheap because it only adds another owner to the existing data.
* A write may be expensive because it must first *detach* shared data by copying it.

Reads never detach.
Writing to an already unique value also needs no copy.
The useful case is therefore a large value that is copied more often than its copies are changed.
Immutable snapshots, syntax trees, collections, and strings often have this shape.

The trade-off is less attractive for small values or values that are almost always changed immediately after copying.
Reference counting adds work to copies and destruction, and the first mutation has a less obvious cost.
Sharing also does not make mutation automatically thread-safe: independent value objects may manage their ownership in
different threads, but simultaneous access to the same public object or mutable data still needs synchronization.

Copy-on-write and ``std::shared_ptr`` solve related but different problems.
A copied ``std::shared_ptr`` deliberately exposes one shared object, so a mutation through either pointer is visible
through both.
A copy-on-write container uses shared ownership as an implementation technique while presenting independent value
semantics.
Before returning mutable access, it must ensure that it is the only owner.

Building a Regular Copy-on-Write Value
======================================

For a custom intrusive data object, derive the private data class from
:cpp:class:`SharedData <erbsland::mem::SharedData>` and store it in a
:cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>`.
The public class should keep that pointer private and expose ordinary value-oriented operations.
This prevents callers from holding a mutable data pointer across later copies and makes every mutation pass through the
detach mechanism.

The data class must be copy-constructible and should contain everything that belongs to one logical value.
Its constructor starts with a reference count of zero; constructing ``SharedDataPointer`` from the new allocation adds
the first reference.
The inherited :cpp:class:`SharedData <erbsland::mem::SharedData>` copy constructor intentionally does not copy the
reference count, so the detached object begins its own lifetime.

Const access through ``operator->``, ``operator*``, ``get()``, or ``constGet()`` leaves sharing intact.
Mutable access through the same operators checks the reference count.
If several pointers own the object, :cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` creates a copy
with the data class's copy constructor, adopts that copy, and releases its reference to the old data.
The write then continues against uniquely owned storage.

This also gives you a natural customization point.
A data copy constructor can copy durable state while rebuilding or discarding derived caches.
It must still create an equivalent logical value, and it must not copy ownership state from the base.
The following board uses the default detach timing and an explicit data copy constructor.

.. erbsland-demo::
    :source: mem/SharedData/RegularCopyOnWrite.cpp
    :exec: mem/shared_data --demo RegularCopyOnWrite
    :source-sha256: faa8e3a3d91c8d20cdbf02079a83c13d1c346b70cdf412d0d33fd0bbed4d14ef

.. code-block:: cpp

    /// Build a value type with an intrusive copy-on-write data object.
    ///
    /// The data derives from `SharedData`, while the public value stores a
    /// `SharedDataPointer`. Copies initially share their data. Mutable pointer
    /// access detaches automatically and invokes the data type's copy constructor.
    class DesignBoard {
        class Data final : public el::mem::SharedData {
        public:
            Data(el::String title, std::vector<el::String> concepts) :
                _title{std::move(title)}, _concepts{std::move(concepts)} {}
            Data(const Data &other) : SharedData{other}, _title{other._title}, _concepts{other._concepts} {}

            el::String _title;
            std::vector<el::String> _concepts;
        };

    public:
        DesignBoard(el::String title, std::vector<el::String> concepts) :
            _data{new Data{std::move(title), std::move(concepts)}} {}

        void addConcept(el::String conceptName) { _data->_concepts.emplace_back(std::move(conceptName)); }
        [[nodiscard]] auto title() const noexcept -> const el::String & { return _data->_title; }
        [[nodiscard]] auto conceptCount() const noexcept -> std::size_t { return _data->_concepts.size(); }
        [[nodiscard]] auto isShared() const noexcept -> bool { return _data.isShared(); }

    private:
        el::mem::SharedDataPointer<Data> _data;
    };

    void regularCopyOnWrite() {
        auto original = DesignBoard{"Rolig form"_el, {"cirkel"_el, "bølge"_el}};

        // Copying the value only shares its data object.
        auto variation = original;
        const auto sharedBeforeWrite = original.isShared() && variation.isShared();

        // The first mutable access copies the shared data before changing it.
        variation.addConcept("lys"_el);

        el::io::printLine("Board             : "_el, original.title());
        el::io::printLine("Shared after copy : "_el, el::BooleanFormat::yesNo(), sharedBeforeWrite);
        el::io::printLine("Original concepts : "_el, original.conceptCount());
        el::io::printLine("Variation concepts: "_el, variation.conceptCount());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Board             : Rolig form
    Shared after copy : yes
    Original concepts : 2
    Variation concepts: 3

.. erbsland-demo-end::

The default-constructed ``SharedDataPointer`` is null.
A wrapper that permits this state must check it before dereferencing; a regular value type is usually simpler when each
constructor creates valid data and every instance therefore maintains a non-null invariant.
Allocation and the data copy constructor may throw during construction or detachment.
Plan the public mutating operation so it has not changed other state before that possible copy.

Keeping the Dynamic Backend Type
================================

Sometimes one public value can store several representations behind a common interface.
A compact byte representation may suit small values, while an integer representation preserves a wider range.
Copy-on-write still works, but a copy made through the common base must preserve the concrete backend type.

Derive the shared base from :cpp:class:`SharedVirtualData <erbsland::mem::SharedVirtualData>` and implement its virtual
``clone()`` operation in every final backend.
The function must allocate and return an unreferenced copy of the same dynamic type.
Returning ``new Derived{*this}`` is the usual implementation; a covariant ``Derived*`` return type keeps it concise.
The shared base also needs a virtual destructor, which ``SharedVirtualData`` already provides.

Store a :cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>` specialized for the common base.
On detachment it calls ``clone()`` through that base rather than slicing the object with a base copy constructor.
The example below constructs both supported representations and then proves that modifying a copied byte sequence keeps
the byte backend.

.. erbsland-demo::
    :source: mem/SharedData/PolymorphicCopyOnWrite.cpp
    :exec: mem/shared_data --demo PolymorphicCopyOnWrite
    :source-sha256: a808550d7374a6b610da19c66e019624b94ed0584247c01f12c4a81bbda30c22

.. code-block:: cpp

    /// Preserve a polymorphic backend while detaching a copy-on-write value.
    ///
    /// Every backend derives from `SharedVirtualData` and implements `clone()`.
    /// `SharedDataPointer` calls that virtual function on the first write to a
    /// shared value, so the detached object keeps its original dynamic type.
    class DesignSequence {
        class Data : public el::mem::SharedVirtualData {
        public:
            [[nodiscard]] virtual auto kind() const noexcept -> el::String = 0;
            [[nodiscard]] virtual auto first() const noexcept -> std::uint32_t = 0;
            virtual void setFirst(std::uint32_t value) = 0;
        };

        class ByteData final : public Data {
        public:
            explicit ByteData(std::vector<std::uint8_t> values) : _values{std::move(values)} {}
            [[nodiscard]] auto clone() const -> ByteData * override { return new ByteData{*this}; }
            [[nodiscard]] auto kind() const noexcept -> el::String override { return "bytes"_el; }
            [[nodiscard]] auto first() const noexcept -> std::uint32_t override { return _values.front(); }
            void setFirst(const std::uint32_t value) override { _values.front() = static_cast<std::uint8_t>(value); }

        private:
            std::vector<std::uint8_t> _values;
        };

        class IntegerData final : public Data {
        public:
            explicit IntegerData(std::vector<std::uint32_t> values) : _values{std::move(values)} {}
            [[nodiscard]] auto clone() const -> IntegerData * override { return new IntegerData{*this}; }
            [[nodiscard]] auto kind() const noexcept -> el::String override { return "integers"_el; }
            [[nodiscard]] auto first() const noexcept -> std::uint32_t override { return _values.front(); }
            void setFirst(const std::uint32_t value) override { _values.front() = value; }

        private:
            std::vector<std::uint32_t> _values;
        };

    public:
        [[nodiscard]] static auto fromBytes(std::vector<std::uint8_t> values) -> DesignSequence {
            return DesignSequence{new ByteData{std::move(values)}};
        }
        [[nodiscard]] static auto fromIntegers(std::vector<std::uint32_t> values) -> DesignSequence {
            return DesignSequence{new IntegerData{std::move(values)}};
        }

        void setFirst(const std::uint32_t value) { _data->setFirst(value); }
        [[nodiscard]] auto kind() const noexcept -> el::String { return _data->kind(); }
        [[nodiscard]] auto first() const noexcept -> std::uint32_t { return _data->first(); }

    private:
        explicit DesignSequence(Data *data) : _data{data} {}

    private:
        el::mem::SharedDataPointer<Data> _data;
    };

    void polymorphicCopyOnWrite() {
        const auto byteSequence = DesignSequence::fromBytes({12U, 24U, 36U});
        auto variation = byteSequence;

        // Virtual cloning keeps the byte backend when the variation detaches.
        variation.setFirst(48U);

        const auto integerSequence = DesignSequence::fromIntegers({1000U, 2000U});
        el::io::printLine("Byte backend      : "_el, variation.kind());
        el::io::printLine("Original first    : "_el, byteSequence.first());
        el::io::printLine("Variation first   : "_el, variation.first());
        el::io::printLine("Integer backend   : "_el, integerSequence.kind());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Byte backend      : bytes
    Original first    : 12
    Variation first   : 48
    Integer backend   : integers

.. erbsland-demo-end::

The same rules as for regular shared data still apply.
Keep backend pointers inside the public value, route writes through mutable pointer access, and make every clone a
logically equivalent independent object.
If a backend owns resources that cannot be copied, it is not a suitable copy-on-write representation unless ``clone()``
can define a meaningful independent equivalent.

Keeping an Array in One Allocation
==================================

A conventional shared data class containing a dynamic array normally needs one allocation for the data object and
another for the array elements.
:cpp:class:`SharedArrayData <erbsland::mem::SharedArrayData>` combines its reference counter, current size, capacity,
and aligned trailing array in one allocation.
This is useful for compact string-like and byte-oriented storage where allocation count and locality matter.

The allocation must be created with ``create(size, capacity)`` and owned by a matching
:cpp:class:`SharedDataPointer <erbsland::mem::SharedDataPointer>`.
Never use an ordinary ``new`` or ``delete`` expression for it: the type's creation and destruction functions know the
size and layout of the complete allocation.
The size may change up to the fixed capacity, but growing beyond that capacity requires creating replacement storage.

For the default :cpp:enum:`SharedArrayDataConstructMethod <erbsland::mem::SharedArrayDataConstructMethod>` value,
``None``, the element type must be trivially copyable and the raw element storage is left uninitialized.
``DefaultConstruct`` and ``ValueConstruct`` construct every capacity element and also destroy those elements during
cleanup, which supports ordinary copy-constructible element types.
The size type can be ``uint32_t`` or ``uint64_t`` and defaults to the smaller representation.

Detachment preserves both size and capacity.
Raw storage copies the used range; constructed storage copy-constructs the used elements and initializes the unused
capacity according to the chosen construction method.
The data class already supplies this clone behavior, so the wrapper only has to ensure that mutable element access goes
through a mutable ``SharedDataPointer`` operation.

.. erbsland-demo::
    :source: mem/SharedData/ArrayCopyOnWrite.cpp
    :exec: mem/shared_data --demo ArrayCopyOnWrite
    :source-sha256: 965d95132accb69cb9b168330636219228c9c154b1c991afdb6e2535278c0e1a

.. code-block:: cpp

    /// Store a copy-on-write byte array in one allocation.
    ///
    /// `SharedArrayData` combines its sharing metadata and trailing element array.
    /// It creates, clones, and destroys that allocation through the matching
    /// `SharedDataPointer` traits.
    class ToneStrip {
        using Data = el::mem::SharedArrayData<std::uint8_t>;

    public:
        explicit ToneStrip(const std::uint32_t size) : _data{Data::create(size, size)} {}

        void set(const std::uint32_t index, const std::uint8_t value) { _data->data()[index] = value; }
        [[nodiscard]] auto at(const std::uint32_t index) const noexcept -> std::uint8_t { return _data->data()[index]; }
        [[nodiscard]] auto size() const noexcept -> std::uint32_t { return _data->size(); }
        [[nodiscard]] auto isShared() const noexcept -> bool { return _data.isShared(); }

    private:
        el::mem::SharedDataPointer<Data> _data;
    };

    void arrayCopyOnWrite() {
        auto palette = ToneStrip{3U};
        palette.set(0U, 18U);
        palette.set(1U, 90U);
        palette.set(2U, 160U);

        // Copies share the header and array until mutable access detaches them.
        auto brighter = palette;
        const auto sharedBeforeWrite = palette.isShared();
        brighter.set(0U, 64U);

        el::io::printLine("Concept           : nordisk lys"_el);
        el::io::printLine("Tone count        : "_el, palette.size());
        el::io::printLine("Shared after copy : "_el, el::BooleanFormat::yesNo(), sharedBeforeWrite);
        el::io::printLine("Original first    : "_el, palette.at(0U));
        el::io::printLine("Brighter first    : "_el, brighter.at(0U));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Concept           : nordisk lys
    Tone count        : 3
    Shared after copy : yes
    Original first    : 18
    Brighter first    : 64

.. erbsland-demo-end::

:cpp:enum:`SharedArrayDataCleanupMethod <erbsland::mem::SharedArrayDataCleanupMethod>` normally selects ``None``.
For raw, trivially copyable elements, ``SecureErase`` instead zero-initializes element capacity and securely erases the
complete allocation before releasing it.
Choose that specialization when the storage can hold secrets, while remembering that temporary copies elsewhere may
still need their own secure-lifetime design.

Choosing a Higher-Level Storage Helper
======================================

The intrusive classes are appropriate when allocation layout, an embedded reference count, or virtual cloning is part of
the design.
For an ordinary copy-constructible C++ data type, :cpp:class:`CowStorage <erbsland::mem::CowStorage>` and
:cpp:class:`CowManualStorage <erbsland::mem::CowManualStorage>` provide the same value behavior with much less code.
Both always hold a valid object in a ``std::shared_ptr`` and copy it when a shared value becomes writable.

:cpp:class:`CowStorage <erbsland::mem::CowStorage>` uses overloads of ``data()``.
Calling it on a const storage returns a const reference without detaching; calling it on mutable storage returns a
mutable reference and detaches first.
This is compact when a small wrapper has straightforward const and non-const operations.
Be deliberate in read-only code: a call on a mutable storage selects the writable overload even if you only inspect the
result.

:cpp:class:`CowManualStorage <erbsland::mem::CowManualStorage>` makes that distinction visible in the name.
``data()`` is always read-only, while ``detachedData()`` is the only operation that returns a mutable reference.
This is often the clearer choice for a larger class, because searches and reviews can identify every possible detach and
mutation point.

.. erbsland-demo::
    :source: mem/SharedData/StorageHelpers.cpp
    :exec: mem/shared_data --demo StorageHelpers
    :source-sha256: d70c3403d4815a5f64e44e985765d645a9a84161f6cd6ec0b4ca128b79b5294b

.. code-block:: cpp

    /// Choose automatic or explicitly marked writable copy-on-write access.
    ///
    /// `CowStorage` overloads `data()` for reading and writing. `CowManualStorage`
    /// keeps `data()` read-only and names its writable path `detachedData()`, which
    /// is useful when a containing class must make every mutation easy to audit.
    void storageHelpers() {
        using Concepts = std::vector<el::String>;

        auto automatic = el::mem::CowStorage<Concepts>::from({"form"_el, "rytme"_el});
        auto automaticCopy = automatic;

        // A mutable data() call automatically detaches CowStorage.
        automaticCopy.data().emplace_back("balance"_el);

        auto explicitWrite = el::mem::CowManualStorage<Concepts>::from({"linje"_el, "flade"_el});
        auto explicitCopy = explicitWrite;

        // CowManualStorage gives the writable operation a distinct name.
        explicitCopy.detachedData().emplace_back("rum"_el);

        const auto &automaticRead = automatic;
        const auto &automaticCopyRead = automaticCopy;
        el::io::printLine("Automatic original: "_el, automaticRead.data().size());
        el::io::printLine("Automatic copy    : "_el, automaticCopyRead.data().size());
        el::io::printLine("Explicit original : "_el, explicitWrite.data().size());
        el::io::printLine("Explicit copy     : "_el, explicitCopy.data().size());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Automatic original: 2
    Automatic copy    : 3
    Explicit original : 2
    Explicit copy     : 3

.. erbsland-demo-end::

Both helpers can default-construct their stored value, accept an existing value with ``from()``, or construct one in
place with ``create()``.
``setData()`` and ``emplaceData()`` replace the current object with unique storage.
``detach()`` makes an upcoming copy point explicit, and ``isShared()``, ``useCount()``, and ``storageId()`` are useful
for diagnostics and tests rather than application identity.

``CowManualStorage::sharedDefault()`` is valuable when many values begin empty.
It lets those instances share one retained default object until the first mutation; its retained canonical owner is
included in the reported use count.
Ordinary default construction instead creates unique storage for every instance.

The central design decision is not which helper has the cheapest copy.
It is whether the public type can honestly preserve value semantics and whether its workload contains enough copies
without writes to repay the reference counting and detach machinery.
When those conditions hold, copy-on-write can make a large type feel as natural to pass and return as a small value.
