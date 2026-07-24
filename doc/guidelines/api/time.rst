**************************
Time Domain API Guidelines
**************************

Core Semantics
==============

Calendar Model
--------------

.. code-block:: text

    calendar = proleptic Gregorian
    internal epoch = 0000-01-01T00:00:00Z
    supported civil dates = 0000-01-01 through 9999-12-31
    invalid civil value = special state ordered before valid values
    POSIX epoch = 1970-01-01T00:00:00Z used only at the native POSIX-time boundary

Time Model
----------

.. code-block:: text

    wall-clock time = nanosecond-precision time within one day
    zoned wall-clock time = time and zone without an instant until combined with a date
    instant = UTC date and time plus display offset or named-zone metadata
    time zone = UTC, normalized fixed offset, or supported named IANA zone
    monotonic point = steady-clock value used only for elapsed-time measurement

Delta Model
-----------

.. code-block:: text

    duration = signed span with second resolution
    precise delta = signed span with nanosecond resolution
    calendar delta = independent signed calendar and fixed-unit components
    calendar arithmetic = apply years and months in civil space, then fixed units
    saturating arithmetic = clamp to supported boundaries
    exact arithmetic = report overflow instead of clamping

Primary Types
=============

.. code-block:: text

    Date // civil Gregorian date with an invalid state
    Time // nanosecond-precision wall-clock time within one day
    TimeWithZone // wall-clock time plus a zone but no date
    DateTime // UTC instant with display offset or named-zone metadata
    Duration // signed second-resolution span
    TimeDelta // signed nanosecond-resolution span
    CalendarDelta // independent calendar and fixed-unit delta
    TimeZone // UTC, fixed offset, or supported named IANA zone
    TimePoint // monotonic steady-clock point

Amount and Part Types
=====================

.. code-block:: text

    Nanoseconds, Microseconds, Milliseconds // fractional-second amounts
    Seconds, Minutes, Hours, Days, Weeks // fixed-ratio second amounts
    Months, Years // calendar amounts
    Year, Month, Day, DayOfYear, DayOfWeek // bounded calendar parts
    Hour, Minute, Second // bounded wall-clock parts

Supporting Types
================

.. code-block:: text

    DateParts, TimeParts, DateTimeParts // named aggregate part results
    YearDayOfYearParts, YearMonthParts, MonthDayParts // calendar extraction results
    Duration::Parts, Duration::DaysAndNanoseconds // duration decomposition results
    TimeWrapResult // time-of-day arithmetic result with day crossings
    ElapsedTimer // restartable monotonic elapsed-time helper
    TimeZoneId // transient numeric zone identifier
    tz::TimeOffset // resolved offset, abbreviation, and daylight-saving metadata
    TimeDeltaFormat // human-readable and ELCL delta formatting policy

Policy Types
============

.. code-block:: text

    DateTimePrecision // ISO precision from year through nanosecond
    IsoTimeFormat, IsoTimeFormatFlags // ISO date and time formatting policy
    DayOfWeekFormat, DurationPart // name style and largest split duration part
    TimeOccurrenceInFold // first or second local occurrence during a daylight-saving fold
    TimeDeltaUnit // smallest fixed unit shown by delta formatting
    SecondsUnitTag, MonthsUnitTag, YearsUnitTag // dimensions of time amount types

Part Value Patterns
===================

.. code-block:: text

    T(value) // create a clamped part value
    T::first/last/minimum/maximum() -> T // create part boundaries
    T::contains(value) -> bool // test raw-value membership
    o.isFirst()/isLast() -> bool // test a part boundary
    o.add/subtract(value) // apply clamped part arithmetic
    o.added/subtracted(value) -> T // return a clamped arithmetic result
    o.next/previous([context]) -> T // navigate to an adjacent valid calendar part
    o.toAmount() -> T // convert a bounded part to its zero-based amount
    T::fromAmount/fromAmountOrThrow(amount) -> T // convert with clamping or exact failure

Calendar Inspection Patterns
============================

.. code-block:: text

    o.year()/month()/day()/dayOfYear()/dayOfWeek() -> T // access calendar parts
    o.hour()/minute()/second() -> T // access wall-clock parts
    o.parts() -> T // return a named aggregate of available parts
    T::exists(parts) -> bool // test whether civil parts form a real date
    T::firstDay/lastDay(context) -> Date // get a month or year boundary date
    T::extract❮Parts❯(value) -> T // decompose an epoch or day-of-year value

Civil Value Patterns
====================

.. code-block:: text

    T(parts) // create a civil value with clamped or invalid-state behavior
    T::fromParts/fromPartsOrThrow(parts) -> T // create with invalid or throwing failure reporting
    o.isValid()/isFirst()/isLast() -> bool // inspect civil state or supported boundaries
    o.add/subtract(amount) // apply saturating arithmetic in place
    o.added/subtracted(amount) -> T // return a saturating arithmetic result
    o.addOrThrow/subtractOrThrow(amount) // apply exact arithmetic in place
    o.addedOrThrow/subtractedOrThrow(amount) -> T // return an exact result or throw
    o.wouldAddSaturate/wouldSubtractSaturate(amount) -> bool // test boundary saturation
    o.durationTo/timeDeltaTo(other) -> T // calculate a signed distance
    T::epoch/first/last() -> T // create named civil boundaries

Time-of-Day Patterns
====================

.. code-block:: text

    o.durationSinceMidnight()/timeDeltaSinceMidnight() -> T // get fixed time since midnight
    o.addWithWrap(delta) -> Days // add in place and return signed day crossings
    o.addedWithWrap(delta) -> TimeWrapResult // return wrapped time and day crossings
    T::fromDurationSinceMidnight(delta) -> Time // wrap a fixed delta into one day
    o.time()/timeZone() -> T // inspect a zoned wall-clock value

Duration and Delta Patterns
===========================

.. code-block:: text

    T(amount-or-parts) // create a duration or delta
    T::❮unit❯/❮unit❯OrThrow(ticks) -> T // create with saturation or exact failure reporting
    T::zero() -> T // create a zero span
    o.isZero()/isPositive()/isNegative() -> bool // inspect sign state
    o.to❮Unit❯()/toStd❮Unit❯() -> T // convert to a Core or standard duration
    o.parts([largestPart]) -> T // decompose a duration
    o.toTimeDelta/toTimeDeltaOrThrow() -> TimeDelta // convert with saturation or exact failure
    o.toDuration() -> Duration // truncate a precise delta toward zero seconds
    o.isValidTimeDelta() -> bool // test exact conversion of a calendar delta

Date-Time Patterns
==================

.. code-block:: text

    T(date, time[, zone-or-offset, occurrence]) // create an instant from civil values
    o.utcDate()/utcTime() -> T // inspect stored UTC parts
    o.date()/time()/timeZone()/timeOffset() -> T // inspect display-zone parts
    o.toUtc()/toTimeZone(zone) -> DateTime // preserve the instant and change display metadata
    o.toSecondsSinceEpoch()/toTimeT() -> T // cross internal or POSIX epoch boundaries
    T::now() -> DateTime // get the current UTC instant
    T::fromSecondsSinceEpoch/fromTimeT(value) -> DateTime // create from an epoch boundary

Time-Zone Patterns
==================

.. code-block:: text

    T(offset-or-id) // create UTC, a fixed offset, or a named zone
    o.isUtc()/isStaticOffset()/isNamed()/isLocalTime() -> bool // inspect zone representation
    o.staticOffset()/name()/id() -> T // inspect zone identity
    T::fromName/fromNameOrThrow(name) -> TimeZone // resolve with empty or throwing failure reporting
    T::names() -> text::StringEditorList // list supported IANA zone names
    T::databaseVersion() -> unit::Version // inspect the bundled IANA database version
    T::utc()/local() -> TimeZone // access UTC or cached system-local zones

Formatting and Parsing Patterns
===============================

.. code-block:: text

    o.toIsoString([flags, precision]) -> text::StringEditor // create ISO text
    T::fromIsoString/fromIsoStringOrThrow(text[, zone, precision]) -> T // parse with invalid or throwing failure
    o.toString([format]) -> text::String // create normalized human-readable text
    T::shortUnits/longUnits/elcl() -> TimeDeltaFormat // create a standard delta format

Monotonic Time Patterns
=======================

.. code-block:: text

    T::now()/inFuture(delta) -> TimePoint // create current or future steady-clock points
    o.timeDeltaTo(point)/timeDeltaToNow() -> TimeDelta // measure a signed monotonic delta
    T() // start an elapsed timer at construction
    o.restart() // restart elapsed measurement at the current monotonic point
    o.elapsed() -> TimeDelta // inspect elapsed monotonic time
