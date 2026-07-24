************************
Utilities API Guidelines
************************

Core Semantics
==============

Container Model
---------------

.. code-block:: text

    index = zero-based element position
    size = non-negative element count
    slice = half-open element range
    storage = copy-on-write wrapper around the matching standard container
    mutation = detach and return the receiver
    transformation = return a new value without changing the source
    standard-container access = explicit raw-value boundary

Result Model
------------

.. code-block:: text

    result = typed success or failure status, never an implicitly interpreted boolean
    success state = encoded in the lower half of the status range
    failure state = encoded in the upper half of the status range
    result data = payload transported beside a specialized status
    loop result = reason a visitor or parser stopped

Coroutine Model
---------------

.. code-block:: text

    synchronous generator = lazy pull sequence
    task = eagerly started move-only single-consumer result awaited as an rvalue
    asynchronous generator = lazy move-only asynchronous single-pass sequence
    asynchronous next = at most one outstanding operation
    failure = rethrown when consuming a task result or awaiting a generator value
    incomplete destruction = request cancellation without forcibly interrupting active work
    worker service = process-wide, without caller-thread affinity, and separate from native input/output workers

Primary Types
=============

.. code-block:: text

    List❮Value❯ // copy-on-write sequential container
    Set❮Key❯, HashSet❮Key❯ // copy-on-write ordered and unordered sets
    Map❮Entry❯, HashMap❮Entry❯ // copy-on-write ordered and unordered maps
    EnumFlags❮Enum❯ // type-safe enum-class flag set
    Result, ResultWithData❮Data❯ // typed status and payload-bearing result
    CoGenerator❮Value❯ // lazy synchronous pull generator
    CoTask❮Value❯ // eagerly started move-only coroutine task
    CoAsyncGenerator❮Value❯ // lazy asynchronous single-pass generator

Secondary Types
===============

.. code-block:: text

    LoopResult // completion reason returned by visitors and parsers
    LoopStatus // continue, regular stop, or error request from a loop callback

Pattern Definitions
===================

.. code-block:: text

    E = ❮Element❯/❮Entry❯ // element or map-entry value
    K = ❮Key❯ // map or set key
    R = ❮RawContainer❯ // wrapped standard container
    V = ❮Value❯ // stored or mapped value

Hash Patterns
=============

.. code-block:: text

    combineHash(first, second) -> std::size_t // combine two computed hashes
    advanceHash(hash, value) // add one value to an existing hash
    createHash(first, rest...) -> std::size_t // combine hashes for one or more values

Common Container Patterns
=========================

.. code-block:: text

    T([values]) // create an empty container or copy compatible values
    o.toRawValue() -> R // cross the explicit standard-container boundary
    o.count()/countIf(function) -> unit::ElementCount // count elements or predicate matches
    o.first()/last() -> E // access an iteration-boundary value or its default
    o.clear()/swap(other) -> T& // remove or exchange contents
    o.remove/removeIf(selector) -> T& // remove selected data in place
    o.removed/removedIf(selector) -> T // return a copy without selected data
    o.forEach(function) -> LoopResult // visit elements with container-specific callback arguments
    o.contains(value-or-key) -> bool // test membership
    o.allOf/anyOf/noneOf(function) -> bool // test a predicate over elements
    o.begin()/end() -> T // provide minimal standard iteration

List Patterns
=============

.. code-block:: text

    o.get(index[, fallback]) -> E // access a value or fallback
    o.set(index, value) -> T& // replace a valid index
    o.resize/reserve/shrinkToFit(count) -> T& // manage sequential storage
    o.slice/prefix/suffix(range-or-count) -> T // copy a selected range
    o.take(index-or-range) -> E // remove and return selected data
    o.takeIf(function) -> T // remove and return predicate matches
    o.insert/append/prepend(position, value-or-list) -> T& // add sequential values
    o.map/reverse/sort([function]) -> T& // transform ordering or values in place
    o.mapped/reversed/sorted([function]) -> T // return transformed values
    o.findFirst/findLast(value-or-function[, start]) -> unit::ElementIndex // locate a value or predicate match
    o.toStdVector()/toStdSet() -> R // explicitly copy into a standard container

Set Patterns
============

.. code-block:: text

    T::fromList(values) -> T // create from an Erbsland list
    o.insert(key)/remove(key) -> T& // mutate membership
    o.tryInsert/tryRemove(key) -> bool // mutate and report whether membership changed
    o.unite/intersect/subtract/symmetricDifference(other) -> T& // apply a set relation in place
    o.unitedWith/intersectedWith/subtractedBy/symmetricDifferenceWith(other) -> T // return a set relation
    o.isSubsetOf/isSupersetOf/isDisjointWith/intersects(other) -> bool // test set relations
    o.toList()/toStd❮Container❯() -> T // explicitly copy into another container

Map Patterns
============

.. code-block:: text

    o.get(key[, fallback]) -> V // access a mapped value or fallback
    o.set(key, value) -> T& // insert or replace a mapping
    o.tryInsert/tryReplace(key, value) -> bool // conditionally mutate a mapping
    o.take(key) -> V // remove and return a mapped value or its default
    o.removeIf❮Part❯/takeIf❮Part❯(function) -> T // remove key, value, or entry matches
    o.forEach❮Part❯(function) -> LoopResult // visit entries, keys, or values
    o.mapValue(function) -> T& // transform mapped values in place
    o.mappedValues(function) -> T // return transformed mapped values
    o.to❮Part❯List()/to❮Part❯Set() -> T // copy keys, values, or entries into a Core container
    o.toStd❮Container❯() -> R // explicitly copy into a standard container

Flag Patterns
=============

.. code-block:: text

    T([flag-or-flags]) // create an empty or initialized flag set
    T::fromRawValue(value) -> T // import raw flag bits
    o.toRawValue() -> V // export raw flag bits
    o.isEmpty()/hasAny()/isSet(flag) -> bool // test flag state
    o.contains/intersects(flags) -> bool // test complete or partial overlap
    o.set/clear([flags]) // add or remove bits
    o.replaceMasked(flags, mask) // replace selected bits

Result Patterns
===============

.. code-block:: text

    o.isSuccessful()/isFailure() -> bool // test status without implicit conversion
    T(status, data) // create a payload-bearing result
    o.status()/data() -> T // inspect status or payload
    o.takeData() -> V // move out transported data

Coroutine Patterns
==================

.. code-block:: text

    T::run(function) -> CoTask❮V❯ // eagerly run bounded work on the coroutine worker service
    o.isComplete() -> bool // poll task completion
    o.result()/takeResult() -> V // inspect or consume a task result
    o.cancel() // request cancellation and release the task handle
    o.next() -> T // produce or await the next optional value
