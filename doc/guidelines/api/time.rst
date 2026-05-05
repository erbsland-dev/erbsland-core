**************************
Time Domain API Guidelines
**************************

These guidelines extend the Common, Math, Text, and Unit and Value API Guidelines for public APIs that model dates,
times, durations, time points, and time zones.

The purpose of this document is to define a base naming vocabulary for time APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Calendar Model
--------------

.. code-block:: text

    calendar // proleptic Gregorian
    internal epoch // 0000-01-01T00:00:00Z
    supported dates // 0000-01-01 through 9999-12-31
    invalid date/time // special state that sorts before valid values
    POSIX epoch // 1970-01-01T00:00:00Z, only for std::time_t conversion

Time and Duration Vocabulary
----------------------------

.. code-block:: text

    Duration // signed span with second resolution
    TimeDelta // signed span with nanosecond resolution
    TimePoint // monotonic steady-clock point for elapsed-time measurement
    Time // wall-clock time within one day
    DateTime // UTC instant plus display offset or named time-zone metadata
    TimeZone // UTC, fixed offset, or supported named IANA time zone

Primary Types
=============

.. code-block:: text

    Date // civil date with invalid state
    Time // wall-clock time of day with nanosecond precision
    DateTime // instant stored as UTC date/time with display offset information
    Duration // signed second-resolution duration
    TimeDelta // signed nanosecond-resolution duration
    TimePoint // monotonic steady-clock point
    ElapsedTimer // restartable elapsed-time helper
    TimeZone // UTC, fixed offset, or named IANA time zone
    TimeZoneId // transient numeric time-zone identifier

Amount Types
============

.. code-block:: text

    Nanoseconds, Microseconds, Milliseconds // fractional-second amounts
    Seconds, Minutes, Hours, Days, Weeks // second-ratio amounts
    Months // calendar month amount
    Years // calendar year amount

Part Types
==========

.. code-block:: text

    Year // Gregorian calendar year, range 0..9999
    Month // month of year, range 1..12
    Day // day of month, range 1..31
    DayOfYear // one-based day of year, range 1..366
    DayOfWeek // Monday through Sunday, stored as 0..6
    Hour // hour of day, range 0..23
    Minute // minute of hour, range 0..59
    Second // second of minute, range 0..59

Supporting Types
================

.. code-block:: text

    DateParts, TimeParts, DateTimeParts // named aggregate part results
    YearDayOfYearParts, YearMonthParts, MonthDayParts // calendar extraction/navigation results
    Duration::Parts, Duration::DaysAndNanoseconds // duration decomposition results
    TimeWrapResult // time-of-day addition result plus day crossings
    tz::TimeOffset // UTC offset, zone id, abbreviation id and DST flag for an instant

Enumerations and Flags
======================

.. code-block:: text

    DateTimePrecision // ISO parse/format precision from year through nanosecond
    DayOfWeekFormat // short or long day-of-week name
    DurationPart // largest part used when splitting Duration
    IsoTimeFormat, IsoTimeFormatFlags // ISO date/time formatting flags
    TimeOccurrenceInFold // first or second local occurrence during a DST fold
    SecondsUnitTag, MonthsUnitTag, YearsUnitTag // unit tags for time amounts

Part Patterns
=============

.. code-block:: text

    T(value) // create a clamped part value
    o.isFirst()/isLast() -> bool // test part range boundaries
    o.added/subtracted(value-or-amount) -> T // clamped part arithmetic
    o.add/subtract(value-or-amount) -> void // clamped in-place part arithmetic
    o.incremented/decremented() -> T // single-step clamped part arithmetic
    o.increment/decrement() -> void // single-step in-place part arithmetic
    o.toAmount() -> Amount // convert one-based or ranged part to zero-based amount
    T::fromAmount(amount) -> T // create part from zero-based amount, clamping
    T::fromAmountOrThrow(amount) -> T // create part from amount or throw err::OutOfRangeError
    T::first()/last() -> T // supported part boundaries
    T::minimum()/maximum() -> T // aliases for first and last
    T::contains(value) -> bool // raw range membership

Calendar Patterns
=================

.. code-block:: text

    o.year()/month()/day() -> Part // access date parts
    o.dayOfYear()/dayOfWeek() -> Part // derived calendar parts
    o.parts() -> Parts // return named part aggregate
    o.exists(year, month) -> bool // test if a day exists in a month
    o.dayCount(year) -> Days // number of days in a month or year
    o.daysSinceEpoch() -> Days // first day of a year as epoch offset
    o.daysBeforeMonth(month) -> Days // cumulative days before month start
    o.monthOfDay(dayOfYear) -> Month // resolve day-of-year to month
    o.next/previous([context]) -> T/Parts // move to adjacent calendar part, clamped
    o.hasNext/hasPrevious([context]) -> bool // test adjacent calendar availability
    T::extract❮Parts❯(...) -> Parts // split epoch or day-of-year values into calendar parts
    T::january()/february()/.../december() -> Month // named month factories
    T::monday()/tuesday()/.../sunday() -> DayOfWeek // named weekday factories

Date Patterns
=============

.. code-block:: text

    Date() // create invalid date
    Date(year, month, day) // create valid date or invalid state
    o.isValid() -> bool // test whether date represents a real calendar date
    o.isFirst()/isLast() -> bool // test supported date boundaries
    o.wouldAddSaturate(amount) -> bool // test if addition would clamp at date boundaries
    o.added(amount) -> Date // return saturated date arithmetic result
    o.addedOrThrow(amount) -> Date // return date arithmetic result or throw err::OverflowError
    o.add(amount) -> void // saturated in-place date arithmetic
    o.addOrThrow(amount) -> void // in-place date arithmetic or throw err::OverflowError
    o.next/previous([dayOfWeek]) -> Date // adjacent date or adjacent weekday
    o.toDaysSinceEpoch() -> Days // internal epoch day offset, -1 for invalid
    o.daysTo(date) -> Days // signed day distance
    o.toIsoString(flags, precision) -> text::String // ISO date text or empty for invalid
    T::fromYearMonthDay(...) -> Date // create from raw parts or invalid state
    T::fromYearMonthDayOrThrow(...) -> Date // create from raw parts or throw err::OutOfRangeError
    T::fromParts(...) -> Date // create from typed parts or invalid state
    T::fromPartsOrThrow(...) -> Date // create from typed parts or throw err::OutOfRangeError
    T::fromDaysSinceEpoch(days) -> Date // create from internal epoch day offset
    T::exists(year, month, day) -> bool // test real calendar date
    T::firstDay/lastDay(year[, month]) -> Date // date boundary for year or month
    T::epoch()/first()/last() -> Date // named date boundaries

Time Patterns
=============

.. code-block:: text

    Time() // create midnight
    Time(hour, minute[, second[, fraction]]) // create clamped time-of-day
    o.isZero() -> bool // test for midnight
    o.hour()/minute()/second() -> Part // access time parts
    o.millisecondFraction()/nanosecondFraction() -> Amount // access second fractions
    o.durationSinceMidnight() -> Duration // seconds since midnight
    o.timeDeltaSinceMidnight() -> TimeDelta // nanoseconds since midnight
    o.toSecondsSinceMidnight() -> Seconds // total seconds since midnight
    o.toNanosecondsSinceMidnight() -> Nanoseconds // total nanoseconds since midnight
    o.toIsoString(flags, precision) -> text::String // ISO time text
    o.addWithWrap(delta-or-duration) -> Days // add in-place and return day crossings
    o.addedWithWrap(delta-or-duration) -> TimeWrapResult // wrapped copy and day crossings
    T::fromDurationSinceMidnight(delta-or-duration) -> Time // wrap duration into one day
    T::first()/last() -> Time // first and last time of day

Duration and Delta Patterns
===========================

.. code-block:: text

    Duration(seconds-or-chrono-or-parts) // create second-resolution duration
    TimeDelta(nanoseconds-or-chrono) // create nanosecond-resolution duration
    o.isZero()/isPositive()/isNegative() -> bool // test sign state
    o.seconds()/minutes()/hours()/days() -> Amount // access split duration components
    o.parts(largestPart) -> Duration::Parts // split duration by largest part
    o.toSeconds()/toNanoseconds() -> Amount // total stored amount
    o.toStdSeconds()/toStdNanoseconds() -> std::chrono::duration // chrono conversion
    o.toDaysAndNanoseconds() -> Duration::DaysAndNanoseconds // split into whole days and remainder
    o.toSecondsWithFractions()/toDaysWithFractions() -> double // fractional conversion
    o.wouldConvertToTimeDeltaSaturate() -> bool // test if Duration to TimeDelta would saturate
    o.toTimeDelta() -> TimeDelta // saturating duration to delta conversion
    o.toTimeDeltaOrThrow() -> TimeDelta // duration to delta conversion or throw err::OverflowError
    o.toDuration() -> Duration // delta to second-resolution duration, truncating toward zero
    T::zero() -> T // zero duration or delta

DateTime Patterns
=================

.. code-block:: text

    DateTime() // create invalid date/time
    DateTime(date, time) // create UTC date/time
    DateTime(date, time, offset-or-zone[, occurrence]) // create local date/time
    o.isValid()/isUtc() -> bool // date/time state tests
    o.utcDate()/utcTime() -> Date/Time // stored UTC values
    o.date()/time() -> Date/Time // local display values
    o.timeOffset() -> Duration // display UTC offset
    o.timeZone() -> TimeZone // display time zone or UTC
    o.timeZoneAbbreviation() -> text::String // zone abbreviation or empty string
    o.wouldAddSaturate/wouldSubtractSaturate(duration) -> bool // arithmetic saturation tests
    o.added/subtracted(duration) -> DateTime // saturated instant arithmetic
    o.addedOrThrow/subtractedOrThrow(duration) -> DateTime // instant arithmetic or throw err::OverflowError
    o.add/subtract(duration) -> void // saturated in-place instant arithmetic
    o.addOrThrow/subtractOrThrow(duration) -> void // in-place instant arithmetic or throw err::OverflowError
    o.durationTo(dateTime) -> Duration // signed second-resolution distance
    o.timeDeltaTo(dateTime) -> TimeDelta // signed nanosecond-resolution distance
    o.toUtc() -> DateTime // convert display zone to UTC
    o.toTimeZone(zone) -> DateTime // convert display zone
    o.toSecondsSinceEpoch() -> Seconds // internal epoch second offset, -1 for invalid
    o.toTimeT() -> std::time_t // convert using POSIX epoch
    o.toIsoString(flags, precision) -> text::String // ISO date/time text or empty for invalid
    T::now() -> DateTime // current UTC date/time
    T::fromSecondsSinceEpoch(seconds[, fractions]) -> DateTime // create from internal epoch
    T::fromDurationSinceEpoch(duration) -> DateTime // create from internal epoch duration
    T::fromTimeT(posixTime) -> DateTime // create from POSIX epoch seconds
    T::fromIsoString(text[, zone][, precision]) -> DateTime // parse or return invalid state
    T::fromIsoStringOrThrow(text[, zone][, precision]) -> DateTime // parse or throw err::ParseError
    T::epoch()/first()/last()/posixEpoch() -> DateTime // named date/time boundaries

Time Zone Patterns
==================

.. code-block:: text

    TimeZone() // create UTC
    TimeZone(offset-or-id) // create fixed offset or named zone by identifier
    TimeZone(hours[, minutes[, seconds]]) // create normalized fixed offset
    o.isUtc()/isStaticOffset()/isNamed() -> bool // time-zone storage state tests
    o.staticOffset() -> Duration // fixed offset or zero
    o.name() -> text::String // primary zone name or empty string
    o.id() -> TimeZoneId // transient zone identifier
    T::isValidName(name) -> bool // test supported zone or special UTC/fixed-offset text
    T::fromName(name) -> optional<TimeZone> // create named zone or empty optional
    T::fromNameOrThrow(name) -> TimeZone // create named zone or throw err::ParseError
    T::names() -> text::StringList // all supported IANA zone names
    T::databaseVersion() -> unit::Version // bundled IANA database version
    T::utc() -> TimeZone // UTC zone
    o.isDst() -> bool // test daylight-saving state on tz::TimeOffset

Elapsed-Time Patterns
=====================

.. code-block:: text

    TimePoint() // create steady-clock epoch value
    TimePoint(steadyClockTimePoint) // wrap std::chrono::steady_clock::time_point
    o.timeDeltaTo(point) -> TimeDelta // signed delta to another monotonic point
    o.timeDeltaToNow() -> TimeDelta // elapsed delta to current steady time
    TimePoint::now() -> TimePoint // current steady-clock point
    TimePoint::inFuture(delta) -> TimePoint // current steady-clock point plus delta
    ElapsedTimer() // start timer at construction
    o.restart() -> void // reset start point to now
    o.elapsed() -> TimeDelta // elapsed monotonic time since start or restart
