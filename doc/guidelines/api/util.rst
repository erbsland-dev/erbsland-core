************************
Utilities API Guidelines
************************

These guidelines extend the Common API Guidelines for public APIs in the ``util`` namespace.
This namespace provides small safe building blocks for containers, flags, hashes, and domain-independent coroutine APIs.
If you introduce new vocabulary or types, update this page.

Core Semantics
==============

.. code-block:: text

    // unit types for index based element access
    unit::ElementIndex  // for index based element access
    unit::ElementCount  // for counting elements
    unit::ElementRange  // for slicing, and addressing element ranges

    // pattern placeholders
    K, Key = key type
    V, Value = value type
    H, HashFn = hash function
    C, CompareFn = compare function
    E, EqualFn = equality function
    Enum = enum class

Primary Types
=============

.. code-block:: text

    List<Value> // COW wrapper around std::vector with element units.
    Set<Key, Compare> // COW wrapper around std::set with set operations.
    HashSet<Key, HashFn, EqualFn> // COW wrapper around std::unordered_set with set operations.
    Map<Key, Value, CompareFn> // COW wrapper around std::map with key/value operations.
    HashMap<Key, Value, HashFn, EqualFn> // COW wrapper around std::unordered_map with key/value operations.
    EnumFlags<Enum> // safe value wrapper for enum-class flag sets.
    CoGenerator<Value> // simple lazy synchronous pull generator.
    CoTask<Value> // eagerly started, move-only coroutine task.
    CoAsyncGenerator<Value> // lazy asynchronous single-pass generator.

Coroutine Semantics
===================

*   Keep :cpp:class:`CoGenerator <erbsland::util::CoGenerator>` as the simple synchronous pull generator. Do not add
    asynchronous state or scheduling to it.
*   Use :cpp:class:`CoTask <erbsland::util::CoTask>` for one eagerly started asynchronous result. Tasks are move-only,
    single-consumer values; await them as rvalues.
*   Use :cpp:class:`CoAsyncGenerator <erbsland::util::CoAsyncGenerator>` only when producing each next value may itself
    suspend. Generators are lazy, move-only, single-pass values and allow one outstanding ``next()`` operation.
*   Exceptions escape through ``result()``, ``takeResult()``, or ``co_await`` for tasks and through ``co_await
    generator.next()`` for asynchronous generators.
*   Destroying an incomplete task requests cancellation. Work already running is not forcibly interrupted, but
    Erbsland coroutine awaiters observe cancellation before continuing the coroutine.
*   Coroutine worker threads have no caller-thread affinity. Code that needs a specific event or UI thread explicitly
    dispatches back to it.
*   Bounded blocking work submitted through ``CoTask::run()`` uses the process-wide coroutine worker service. This
    service is separate from domain-specific native-I/O workers to avoid starving the work that completes an awaited
    operation.

Coroutine Patterns
==================

.. code-block:: text

    CoTask<T>::run(function) -> CoTask<T> // eagerly run bounded work on the coroutine worker service.
    task.isComplete() -> bool // poll completion without waiting.
    task.result() -> const T& // inspect a completed result without consuming it.
    task.takeResult() -> T // move a completed result out of the task.
    co_await std::move(task) -> T // suspend and consume the task result.
    task.cancel() // request cancellation and release this task handle.
    generator.next() // awaitable producing optional<T>; empty means completion.

Helper Methods
==============

.. code-block:: text

    // HashHelper.hpp:
    combineHash(hash1, hash2) -> std::size_t // combine two already computed hash values.
    advanceHash(hash, value) // hash one value and combine it into an existing hash.
    createHash(first, rest...) -> std::size_t // create a combined hash from one or more values.

Shared Container Patterns
=========================

All util containers provide the following common shape unless a specific container family below narrows it.

.. code-block:: text

    T{} // create an empty container.
    T{values...} // create from an initializer list.
    T(raw) // create from the wrapped standard container.
    o.toRawValue() -> const Raw& // access the wrapped standard container.
    o.count() -> unit::ElementCount // count contained elements.
    o.countIf(function) -> unit::ElementCount // count elements matching a predicate.
    o.first()/last() -> Element-or-Entry // get iteration-boundary element or default for empty containers.
    o.reserve(count) -> Self& // reserve storage where meaningful; ordered sets/maps may no-op.
    o.capacity() -> unit::ElementCount // get current capacity, bucket capacity, or count for ordered sets/maps.
    o.shrinkToFit() -> Self& // reduce unused storage where meaningful.
    o.clear() -> Self& // remove all elements.
    o.swap(other) -> Self& // swap contents.
    o.remove(selector) -> Self& // remove by index/range, key, or value depending on container.
    o.removeIf(function) -> Self& // remove matching data in-place.
    o.removed(selector) -> Self // return a copy with selected data removed.
    o.removedIf(function) -> Self // return a copy with matching data removed.
    o.forEach(function) -> util::LoopResult // call function for each element.
    o.contains(value-or-key) -> bool // test membership.
    o.allOf/anyOf/noneOf(function) -> bool // predicate tests over all elements.
    o.begin()/end() -> const_iterator // minimal std-library iteration support.
    swap(a, b) // standard friend swap.

List Patterns
=============

.. code-block:: text

    o + value/list -> Self // return appended copy.
    o += value/list -> Self& // append in-place.
    o[index] -> Element // get element or default value.
    o.toStdVector() -> std::vector<Element> // convert to std::vector.
    o.toStdSet() -> std::set<Element> // convert to std::set.
    o.get(index[, defaultValue]) -> Element // get value or default.
    o.set(index, value) -> Self& // replace if index is valid.
    o.resize(count[, value]) -> Self& // resize the list.
    o.sliceFirst()/sliceLast() -> std::pair<Element, Self> // split off boundary element.
    o.slice(range) -> Self // copy a valid range.
    o.prefix/suffix(count) -> Self // copy leading/trailing elements.
    o.remove(index/range) -> Self& // remove at index or range in-place.
    o.removeFirst/removeLast() -> Self& // remove boundary element in-place.
    o.take(index/range) -> Element-or-Self // remove and return data.
    o.takeIf(function) -> Self // remove and return matching elements.
    o.takeFirst/takeLast() -> Element // remove and return boundary element.
    o.forEachReverse(function) -> util::LoopResult // iterate in reverse order.
    o.map(function) -> Self& // transform elements in-place.
    o.mapped(function) -> Self // return transformed copy.
    o.reverse()/reversed() -> Self&/Self // reverse order.
    o.collapse()/collapsed() -> Self&/Self // remove duplicate neighboring values.
    o.sort([function]) -> Self& // sort in-place.
    o.sorted([function]) -> Self // return sorted copy.
    o.findFirst(value[, start]) -> unit::ElementIndex // find first matching index.
    o.findFirstIf(function[, start]) -> unit::ElementIndex // find first predicate match.
    o.findLast(value[, start]) -> unit::ElementIndex // find last matching index.
    o.findLastIf(function[, start]) -> unit::ElementIndex // find last predicate match.
    o.insert(index, value/list) -> Self& // insert at index.
    o.append(value/list) -> Self& // append to the end.
    o.prepend(value/list) -> Self& // prepend to the beginning.
    o.compare(other) -> std::strong_ordering // compare list contents.

Set and HashSet Patterns
========================

.. code-block:: text

    T::fromList(values) -> Self // create a set from a List.
    o.toList() -> List<Key> // convert to an Erbsland list.
    o.toStdVector() -> std::vector<Key> // convert to std::vector.
    o.toStdSet() -> std::set<Key> // convert to std::set.
    o.toStdUnorderedSet() -> std::unordered_set<Key> // convert to std::unordered_set.
    o.tryRemove(key) -> bool // remove key and report if it existed.
    o.forEachReverse(function) -> util::LoopResult // Set only: iterate in reverse key order.
    o.unite/intersect/subtract/symmetricDifference(other) -> Self& // mutate set relation.
    o.unitedWith/intersectedWith/subtractedBy/symmetricDifferenceWith(other) -> Self // return set relation.
    o.insert(key) -> Self& // insert key.
    o.tryInsert(key) -> bool // insert key and report if it was new.
    o.compare(other) -> bool // compare contents.
    o.isSubsetOf/isSupersetOf(other) -> bool // test set containment.
    o.isDisjointWith/intersects(other) -> bool // test set overlap.

Map and HashMap Patterns
========================

.. code-block:: text

    o.toStdMap() -> std::map<Key, Value> // convert to std::map.
    o.toStdUnorderedMap() -> std::unordered_map<Key, Value> // convert to std::unordered_map.
    o.toStdKeyVector() -> std::vector<Key> // convert keys to std::vector.
    o.toStdVector() -> std::vector<Entry> // convert entries to std::vector.
    o.toKeySet/toKeyHashSet() -> Set-or-HashSet<Key> // convert keys to a set.
    o.toValueSet/toValueHashSet() -> Set-or-HashSet<Value> // convert values to a set.
    o.countIfKey/countIfValue(function) -> unit::ElementCount // count by key or value.
    o.get(key) -> std::optional<Value> // get value or empty optional.
    o.get(key, defaultValue) -> Value // get value or default value.
    o.toKeyList/toValueList/toList() -> List<Key-or-Value-or-Entry> // convert to Erbsland lists.
    o.removeIfKey/removeIfValue(function) -> Self& // remove by key or value.
    o.removedIfKey/removedIfValue(function) -> Self // return a copy removing by key or value.
    o.take(key) -> Value // remove key and return value or default value.
    o.takeIf/takeIfKey/takeIfValue(function) -> Self // remove and return matching entries.
    o.forEachKey/forEachValue(function) -> util::LoopResult // iterate over keys or values.
    o.forEachReverse/forEachKeyReverse/forEachValueReverse(function) -> util::LoopResult // Map only: reverse iteration.
    o.mapValue(function) -> Self& // transform mapped values in-place.
    o.mappedValues(function) -> Self // return copy with transformed mapped values.
    o.set(key, value) -> Self& // insert or replace value.
    o.tryReplace(key, value) -> bool // replace only if key exists.
    o.tryInsert(key, value) -> bool // insert only if key is new.
    o.compare(other) -> bool // compare entries.
    o.compareKeys(other) -> bool // compare key sets.

EnumFlags Patterns
==================

.. code-block:: text

    EnumFlags{} // create an empty flag set.
    EnumFlags{flag} // create from one enum flag.
    EnumFlags{flag1, flag2} // create from an initializer list.
    T::fromRawValue(value) -> T // import raw bits explicitly.
    o.toRawValue() -> Value // export raw bits explicitly.
    o &/|/^ flags-or-flag -> T // intersection, union, or exclusive union.
    o &=/|=/^= flags-or-flag -> T& // mutate bit operations.
    ~o -> T // bounded complement, only when Enum::All exists.
    o.isEmpty()/hasAny() -> bool // test if any bits are set.
    o.isSet(flag) -> bool // test one enum flag.
    o.contains(flags) -> bool // test if all bits are present.
    o.intersects(flags) -> bool // test if at least one bit overlaps.
    o.set(flag-or-flags) // add bits.
    o.clear([flag-or-flags]) // clear selected bits or all bits.
    o.replaceMasked(flags, mask) // replace only masked bits.
