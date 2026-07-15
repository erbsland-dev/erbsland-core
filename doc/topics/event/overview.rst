..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*************************
Event System API Overview
*************************

.. erbsland-draft::

This page gives you a practical overview of the event system in Erbsland Core.

Introduction
============

Event based applications are often more robust and easier to implement and maintain, as procedural approaches.
This is especially true to multi-threaded systems, like servers and interactive applications.
Yet, we also think, that an event based approach helps making traditional tools easier to write and maintain.
Therefore, Erbsland Core contains a event system that is modular, so it works for a wide range of use-cases.

Events vs Procedural
====================

.. compare the two methods, based on our library and explain strengths and weaknesses of the approaches.

Three Common Ways to Use Events in Erbsland Core
================================================

Using the Main Event Loop from ``Application``
----------------------------------------------

The :cpp:class:`Application <erbsland::core::Application>` automatically starts an event loop, if no other ``main``
method is set.
It is the fastest way to write an event based application, with minimal boiler plate code.

.. erbsland-demo::
    :source: event/FileSizeMonitor/main.cpp
    :exec: event/file_size_monitor {file:text}
    :source-sha256: 06baf21f76eb513b9f643d44f39c998a451075e405d54f3c21fea927668026de

.. code-block:: cpp

    /// In an event-based application, you initialize your event handlers and let the event loop run.
    void initialize() {
        el::stdOut()->printLine("Monitoring size of file: "_el, appData.pathInfo.resolvedPath());
        poll();
        appData.pollTimer = el::application().events()->createTimer(poll);
        appData.pollTimer->startFixedDelay(el::Seconds{1});
    }

    /// This method is called in configured intervals to display the file size.
    void poll() {
        appData.pathInfo.reload();
        if (!appData.pathInfo.exists()) {
            el::stdOut()->printLine("File not found"_el);
        } else {
            el::stdOut()->printLine("Size: "_el, appData.pathInfo.fileSize(), " bytes"_el);
        }
    }

    /// This method is automatically called after the options are parsed.
    void handleOptions(const el::OptionValuesPtr &values) {
        try {
            auto path = el::Path::fromNativeOrThrow(values->getText("file"_el));
            path = path.resolveOrThrow(el::PathResolveMode::Physical);
            appData.pathInfo = path.info(el::PathInfoPart::Type);
        } catch (const el::ParseError &) {
            throw el::ApplicationError{"The path has an invalid format"_el, std::current_exception()};
        } catch (const el::PathError &) {
            throw el::ApplicationError{
                "The file at the given path does not exist or is no regular file"_el, std::current_exception()};
        }
        const auto timeout = el::Seconds{values->getInteger("timeout"_el)};
        if (timeout.isNegative()) {
            throw el::ApplicationError{"The timeout must be a non-negative integer."_el};
        }
        el::application().events()->invoke(initialize);
        if (timeout.isPositive()) {
            el::application().events()->invokeAfter(timeout, []() -> void { el::application().quit(); });
        }
    }

    /// Here you create the command line options.
    auto createOptions() -> el::OptionSetPtr {
        auto optionSet = el::OptionSet::create();
        optionSet->addOption("file"_el).setRequired().setHelp("The path to the file to monitor."_el);
        optionSet->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setHelp("The timeout in seconds. Zero for no timeout."_el)
            .setDefaultValue(5);
        optionSet->setPostParsingFn(handleOptions);
        return optionSet;
    }

    /// This simple main method is the entry point of the application.
    auto main(const int argc, char *argv[]) -> int {
        auto app = el::Application{argc, argv};
        app.enableTerminal();
        app.info().setApplicationName("File Size Monitor"_el);
        app.options()->addSet(createOptions());
        return app.run();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Monitoring size of file: /private/var/folders/f2/rq38shx1797_b7m0vy142phw0000gn/T/erbsland
    -demo-doc-_8wse00h/file-0-text
    Size: 187 bytes
    Size: 187 bytes
    Size: 187 bytes
    Size: 187 bytes
    Size: 187 bytes

.. erbsland-demo-end::

For this use-case, you write you initialization code using an initial invoke call.

.. code-block:: cpp

    el::application().events()->invoke(initialize);

You define no custom main function, so ``app.run()`` will automatically start the main event loop for you.
As soon the event loop is started, your ``initialize`` method is called where you kick off your application.

This is the right choice for single or multi-threaded, event based applications that do not required custom
initialization or a custom :cpp:class:`Application <erbsland::core::Application>` class.

Starting the Main Loop Manually
-------------------------------

If you create your own application class and need to override the ``main`` method, you have access to the main loop and
can start it manually after all initialization work is done.

.. erbsland-demo::
    :source: event/FileTimeMonitor/FileTimeMonitor.hpp
    :exec: event/file_time_monitor {file:text}
    :source-sha256: 128cf936793cb291074f008854ce8fb17baedacc199d1c45af251fdbbe408923

.. code-block:: cpp

    /// Here we demonstrate how to enter the main loop if you derive from `Application`.
    /// The whole logic is in this derived class, which is not recommended for a real application.
    class FileTimeMonitorApp : public el::Application {
    public:
        using Application::Application;

    protected:
        void initialize() override { info().setApplicationName("File Time Monitor"_el); }
        void registerCommandLineOptions(const el::OptionsPtr &options) override {
            options->addOption("file"_el).setRequired().setHelp("The path to the file to monitor"_el);
            options->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
                .setType(el::OptionType::Integer)
                .setHelp("The timeout in seconds. Zero for no timeout."_el)
                .setDefaultValue(5);
        }
        [[nodiscard]] auto main() -> el::ExitCode override {
            _path = std::filesystem::path{el::StringConverter{optionValues()->getText("file"_el)}.toStdString()};
            if (!exists(_path)) {
                throw el::ApplicationError{"File does not exist"_el};
            }
            const auto timeout = el::Seconds{optionValues()->getInteger("timeout"_el)};
            if (timeout.isPositive()) {
                events()->invokeAfter(timeout, []() -> void { el::application().quit(); });
            }
            _pollTimer = events()->createTimer([this]() -> void { poll(); });
            _pollTimer->startFixedDelay(el::Seconds{1});
            el::stdOut()->printLine("Monitoring file: ", _path.string());
            // at this point, enter the main event loop.
            return runEventLoop();
        }
        void poll() {
            // const auto lastWriteTimeStd = std::filesystem::last_write_time(_path);
            // FIXME! There is currently no postable way to get the file time using std::filesystem that works
            // out of the box. Replace this as soon the `Path` feature is available.
            const auto lastWriteTime = el::DateTime::now(); // FIXME!
            el::stdOut()->printLine("Last Modification: ", lastWriteTime.toIsoString());
        }

    private:
        el::EventTimerPtr _pollTimer;
        std::filesystem::path _path;
    };

.. erbsland-ansi::
    :escape-char: ␛

    Monitoring file: /var/folders/f2/rq38shx1797_b7m0vy142phw0000gn/T/erbsland-demo-doc-vm7o_r
    1x/file-0-text
    Last Modification: 2026-07-15 14:24:29
    Last Modification: 2026-07-15 14:24:30
    Last Modification: 2026-07-15 14:24:31
    Last Modification: 2026-07-15 14:24:32

.. erbsland-demo-end::

Using Event Threads in a Procedural Application
-----------------------------------------------

A main event loop is not strictly required.
Therefore, you can write a procedural and/or multithreaded program in a traditional way, but make use of event threads
for individual parts.
It is one of the simplest and reliable ways to create worker threads.

.. example program demonstrating this

Invoking, Scheduling and Timers
===============================



Use ``post`` when you already have a raw :cpp:class:`Event <erbsland::event::Event>` object.
Use ``invoke`` to queue a callback for execution in an event loop, including callbacks sent from another thread.
Use ``invokeAfter`` for fire-and-forget delayed callbacks where no cancellation handle is required.

Use a timer for scheduled work that needs an explicit lifetime.
The returned timer is the cancellation object: keep it for as long as the timer shall remain active, and call ``stop``
or release the last timer pointer to cancel future callbacks.
