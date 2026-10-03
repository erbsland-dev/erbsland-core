************************
Utilities API Guidelines
************************

Primary Types
=============

.. code-block:: text

    List❮Value❯ // copy-on-write sequential container
    Set❮Key❯, HashSet❮Key❯ // copy-on-write ordered and unordered sets
    Map❮Entry❯, HashMap❮Entry❯ // copy-on-write ordered and unordered maps
    FifoCache, LruCache // independently owned bounded caches
    EnumFlags❮Enum❯ // type-safe enum-class flag set
    Result, ResultWithData❮Data❯ // typed status and payload-bearing result
    CoGenerator❮Value❯ // lazy synchronous pull generator
    CoTask❮Value❯ // eagerly started move-only coroutine task
    CoAsyncGenerator❮Value❯ // lazy asynchronous single-pass generator
    WorkerPool // independently owned parallel execution with optional owner-loop outcomes
    Utilities // event-loop frontend for explicitly bound utility event sources

Secondary Types
===============

.. code-block:: text

    WorkerPoolOptions, WorkerPoolEventEditor // pool configuration and source-owned notifications
    WorkerPoolFullError, WorkerPoolClosedError, WorkerCancelledError // admission and cancellation outcomes
    LoopResult // completion reason returned by visitors and parsers
    LoopStatus // continue, regular stop, or error request from a loop callback

Cooperative Stop Types
======================

.. code-block:: text

    StopSource // shared signalling authority for one irreversible stop request
    StopToken // shared read-only observation; a default token never reports stopping

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
    o.count()/countIf(function) -> unit::ItemCount // count elements or predicate matches
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

    o.lastIndex() -> unit::ItemIndex // final element index; noIndex for an empty list
    o.get(index[, fallback]) -> E // access a value or fallback
    o.getRef(index) -> const E& // borrow a value or a shared immutable default
    o.getRefOrThrow(index) -> const E& // borrow a value or throw for an invalid index
    o.set(index, value) -> T& // replace a valid index
    o.resize/reserve/shrinkToFit(count) -> T& // manage sequential storage
    o.slice/prefix/suffix(range-or-count) -> T // copy a selected range
    o.take(index-or-range) -> E // remove and return selected data
    o.takeIf(function) -> T // remove and return predicate matches
    o.insert/append/prepend(position, value-or-list) -> T& // add sequential values
    o.map/reverse/sort([function]) -> T& // transform ordering or values in place
    o.mapped/reversed/sorted([function]) -> T // return transformed values
    o.findFirst/findLast(value-or-function[, start]) -> unit::ItemIndex // locate a value or predicate match
    o.toStdVector()/toStdSet() -> R // explicitly copy into a standard container

Set Patterns
============

.. code-block:: text

    T::fromList(values) -> T // create from an Erbsland Core list
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

Cache Patterns
==============

.. code-block:: text

    T(capacity) // finite maximum entry count; zero disables retention
    o.capacity()/setCapacity(count) // inspect or change the limit, evicting excess entries immediately
    o.get(key) -> std::optional<V> // copy a value; LRU refreshes recency, FIFO preserves insertion order
    o.peek(key) -> std::optional<V> // copy a value without changing eviction order
    o.set(key, value) -> T& // insert or replace; LRU refreshes recency, FIFO preserves existing order
    o.count()/isEmpty()/contains(key) // inspect retained entries without changing order
    o.remove(key)/clear() -> T& // remove one key or all entries

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

Cooperative Stop Patterns
=========================

.. code-block:: text

    o.stopToken() -> StopToken // retain read-only observation independently of source lifetime
    o.requestStop() -> bool // request stopping; true only for the first request
    o.isStopRequested() -> bool // observe stopping on a source or token

Worker Pool Patterns
====================

.. code-block:: text

    T::create([options]) -> WorkerPoolPtr // open an independent, joinable pool
    o.submit(work) -> std::future<R> // submit a value or void callable
    o.map(values, work) -> std::future<List<R>> // snapshot and transform a finite range in input order
    o.forEach(values, work) -> std::future<void> // snapshot and process a finite range
    o.submit/map/forEach(..., completed[, failed]) // deliver the outcome on the owner loop
    o.events() -> WorkerPoolEventEditor& // bind the current owner loop once and edit source-owned handlers
    o.onError/onCancelled/onIdle/onFinal(callback) -> T& // replace one owner-loop handler
    o.wait() // drain work captures without closing admission or dispatching callbacks
    o.quit()/stop() // close admission and drain/request cooperative cancellation without waiting
    o.join() // wait for worker termination without requesting shutdown
    o.isAccepting()/workerCount()/stopToken() -> T // inspect procedural admission, concurrency, or stopping
    o.workerCount()/setWorkerCount(count) // options: zero means automatic hardware concurrency
    o.maximumPendingTasks()/setMaximumPendingTasks(count) // options: zero means unlimited operations

Utilities Frontend Patterns
===========================

.. code-block:: text

    o.get<Utilities>() -> Utilities& // frontend belonging to the selected loop
    o.createWorkerPool([options]) -> WorkerPoolPtr // create and bind on that frontend's owner loop
