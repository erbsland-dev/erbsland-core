.. index::
    single: Erbsland Core; Background
    single: Erbsland Core; History

******************************
The Story Behind Erbsland Core
******************************

Why did we create another C++ foundation library, and why did we publish it as open source? Erbsland Core did not begin
as a plan for a new framework.
It grew out of a simple rule, more than sixteen years of practical development, and a collection of libraries that
gradually became a foundation of their own.
This page tells that story.

A Simple Rule: Avoid Duplicated Code
====================================

Throughout our commercial projects, we followed one basic principle: avoid duplicating algorithms and code whenever
possible.
If a new implementation had even a modest chance of being useful in another project, we moved it into a new library or
added it to an existing one.

At first, this was merely a practical habit.
Over time, it became one of our greatest advantages.
New applications could build on a large body of established code instead of solving the same problems again.
This not only accelerated development; it also reduced the effort required for testing and validation because the shared
libraries had already been exercised in other applications.

C++ and Python became the main languages behind this work.
Rust, Swift, Java, JavaScript, and other languages also had their place, but C++ remained especially important where
performance mattered or where applications had to communicate with older systems.
For such systems, C and C++ are often the most direct and dependable bridge.

Because Erbsland Core is a C++ foundation library, the rest of this story follows the development of our C++ code.
Similar efforts took place in other languages, but on a smaller scale.

Growing Alongside Qt
====================

For many years, our C++ libraries developed alongside Qt.
Qt provided the platform abstraction and core types, while our own libraries filled the gaps required by commercial
applications.
These libraries remained closed source because they were built on top of a commercial, highly customized Qt environment.
Without that environment, they offered little practical value to the public.

One early example was REST communication.
Many of our applications needed it, so we built fast JSON and HTTP client and server libraries on top of Qt.
Qt added its own JSON and HTTP implementations years later, yet we often continued using ours because their design was
already aligned with our conservative, security-focused approach.

This pattern repeated itself.
The public interfaces of our libraries stayed largely stable, while their implementations became less dependent on Qt.
Whenever the abstraction offered by Qt did not provide enough control or detail, we replaced that part with our own
platform-independent implementation.

File operations are a good example.
In a large enterprise environment, knowing that an operation failed is rarely enough.
An application also needs the exact reason reported by the operating system so it can diagnose the problem and respond
appropriately.
Where Qt did not expose the required diagnostic information, our own implementation did.

Gradually, our applications used fewer Qt APIs directly.
Most functionality came through our own libraries, with only Qt's core types remaining underneath.
For later libraries, we often omitted Qt entirely and built directly on the C++ standard library, keeping dependencies
to a minimum.

When Reuse Becomes Fragmentation
================================

The original rule had worked: we had accumulated hundreds of thousands of lines of useful, well-tested code.
Yet that code was spread across many small libraries and several generations of frameworks.
Qt had also become a substantial dependency for applications that used very little of it.

What had once made development faster was beginning to create complexity of its own.
We needed a common base that could bring the strongest parts together, provide consistent conventions, and support our
higher-level libraries without pulling in another framework.

That need led to Erbsland Core: a new, dependency-free foundation library intended to become the shared base for our
future work.
It also gave us the opportunity to separate the foundation from its commercial origins and make it available to
everyone.
We deliberately chose a straightforward open-source model rather than dual licensing because this foundation should
remain free and open source.

Separate First, Then Port and Combine
=====================================

The transition began in 2022. We separated code from existing libraries into independent branches, reduced Qt
dependencies where that was practical, and rewrote other components from scratch.
This was a slow and deliberate preparation rather than a single large migration.

By early 2025, the independent pieces were ready to be brought into the new foundation.
We began with strings, input and output, mathematics, memory, units, date and time, and general utilities.
These fundamental areas appear in almost every application and therefore offered the greatest opportunity to eliminate
duplication.

Once this base worked as a coherent whole, Erbsland Core was ready for its first public step.
We released the first alpha version in June 2026 and have published new versions regularly since then.

Erbsland Core is therefore both new and the result of many years of experience.
Its code is being reorganized, reviewed, and in many places rewritten, but the problems it addresses are familiar ones.
The library brings those lessons into a single foundation that we can improve in the open and share with every project
that finds it useful.
