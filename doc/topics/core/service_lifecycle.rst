..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Service Lifecycle
    single: Service; Creating
    single: Windows Service
    single: Linux; systemd Service
    single: macOS; launchd Daemon
    single: Daemon; Foreground

****************************************
Creating Services and Foreground Daemons
****************************************

A background service has the same business logic as an interactive server, but a different relationship with the
operating system.
It is started without a terminal, monitored by a service manager, and expected to stop in a predictable way when the
machine shuts down or an administrator requests it.

This page shows how one Erbsland Core executable can meet those expectations on Windows, Linux, and macOS while still
remaining convenient to run from a terminal during development.
You will build the lifecycle into the application, install the executable with each platform's native service manager,
and learn which status and stop requests are translated into the regular application lifecycle.

What Makes a Service Different?
===============================

A command-line program normally begins because a person launches it, inherits that person's terminal and environment,
and exits when its immediate task is complete.
A service is instead owned by a supervisor.
The supervisor decides when it starts, which identity and environment it receives, where its output goes, whether a
failure should restart it, and how long shutdown may take.

That ownership has a useful consequence: the application does not need to turn itself into a traditional background
daemon.
It should stay in the foreground, keep its main process alive, and let the supervisor track that process.
Forking into the background, writing a PID file, or detaching from the session would hide the process that a modern
service manager is trying to manage.

The three supported platforms express this relationship differently:

* Windows services register with the Service Control Manager (SCM), publish lifecycle states, and receive service
  controls such as stop and system shutdown.
* Linux distributions commonly use systemd.
  A unit file describes a foreground process, and systemd sends signals to stop it and may collect its output in the
  journal.
* macOS uses launchd.
  A property-list file describes a system-wide launch daemon or a per-user launch agent, and launchd owns the process
  lifetime.

The :cpp:class:`Application <erbsland::core::Application>` service lifecycle bridges the native stop mechanisms to the
same orderly shutdown path used by an interactive application.
It does not install the program, choose a service account, or define a restart policy; those deployment decisions stay
in the platform configuration where administrators expect to find them.

Preparing One Executable for Every Mode
=======================================

Call :cpp:func:`enableServiceLifecycle() <erbsland::core::Application::enableServiceLifecycle>` after constructing the
application and before :cpp:func:`run() <erbsland::core::Application::run>`.
The call is intentionally made at the composition root, where it is easy to see that native lifecycle integration is
active.
Repeated calls have no additional effect, but enabling it after ``run()`` has begun is an error.

.. erbsland-demo::
    :source: core/ServiceLifecycle/main.cpp
    :source-sha256: 3415ac41f4bceef6333792be592bb19f7fc277eb85acb531a17433414156dad1

.. code-block:: cpp

    /// Enable native service integration before the application lifecycle starts.
    ///
    /// The executable runs as a Windows service when the Service Control Manager launches it.
    /// Everywhere else, it runs as an ordinary foreground process.
    /// Both modes use the same initialization, main event loop, and orderly cleanup.
    auto main(const int argc, char *argv[]) -> int {
        auto app = CatalogMonitorApplication{argc, argv};
        app.enableServiceLifecycle();
        return app.run();
    }

.. erbsland-demo-end::

On Windows, ``run()`` first offers the executable to the SCM.
When the SCM launched it, the application enters service mode and publishes native state transitions.
When a developer starts the same file in PowerShell or a terminal, Windows reports that no service controller is
attached and the ordinary application lifecycle continues.

Linux and macOS always run the executable as a foreground process.
The lifecycle subscribes to the termination signals used by supervisors and translates them into
:cpp:func:`quit() <erbsland::core::Application::quit>`.
This means you can start the executable directly, inspect its output, and stop it with :kbd:`Ctrl+C` without maintaining
a second development entry point.

The demo is a small catalog monitor that remains on the main event loop and refreshes its index periodically.
Run ``core/core_service_lifecycle --help`` to inspect it, or start ``core/core_service_lifecycle`` and stop it with
:kbd:`Ctrl+C`.
A production service can use the same shape while replacing the sample initialization and timer callback with its own
configuration, storage, and server components.

Installing the Compiled Application
===================================

Build and test the executable before installing it, then copy the release artifact to a stable, absolute path.
Service definitions outlive individual build directories, and supervisors generally start with a small, controlled
environment.
Do not depend on the current directory, a developer shell's ``PATH``, or environment variables that are not declared in
the service definition.

Run under a dedicated, unprivileged account whenever the service does not need system privileges.
Give that account access only to the configuration, data, sockets, and log destinations it actually uses.
The examples below use ``CatalogMonitor`` consistently, but the service name, executable path, identity, restart policy,
and resource limits are deployment choices rather than library requirements.

Windows Service Control Manager
-------------------------------

Open PowerShell as an administrator and copy the executable to its permanent location.
``New-Service`` stores the complete command line in the SCM database, so the inner quotes around a path containing
spaces are significant.

.. code-block:: powershell

    $installDirectory = 'C:\Program Files\Erbsland\Catalog Monitor'
    New-Item -ItemType Directory -Force -Path $installDirectory
    Copy-Item '.\core_service_lifecycle.exe' "$installDirectory\catalog-monitor.exe"

    $serviceParameters = @{
        Name = 'CatalogMonitor'
        BinaryPathName = '"C:\Program Files\Erbsland\Catalog Monitor\catalog-monitor.exe"'
        DisplayName = 'Catalog Monitor'
        Description = 'Periodically refreshes the deployment catalog index.'
        StartupType = 'Automatic'
    }
    New-Service @serviceParameters
    Start-Service -Name 'CatalogMonitor'
    Get-Service -Name 'CatalogMonitor'

Without ``Credential``, this example uses the SCM's default ``LocalSystem`` identity.
For a deployed system, configure a less privileged, dedicated service account and grant it read and execute access to
the installation directory before the first start.
If the executable accepts options, include them after the quoted executable name in ``BinaryPathName``.

Use the same service name for routine control and removal:

.. code-block:: powershell

    Stop-Service -Name 'CatalogMonitor'
    Remove-Service -Name 'CatalogMonitor'

Stopping the service first gives the application a chance to clean up.
When replacing the executable during an upgrade, stop it, atomically install the tested replacement, and start it again.

Linux with systemd
------------------

Install the executable at a stable path and create a dedicated account using the account-management policy of your
distribution.
The following ``/etc/systemd/system/catalog-monitor.service`` unit uses ``Type=simple`` because the application stays in
the foreground.

.. code-block:: systemd

    [Unit]
    Description=Catalog Monitor
    After=network.target

    [Service]
    Type=simple
    ExecStart=/usr/local/libexec/catalog-monitor
    User=catalog-monitor
    Group=catalog-monitor
    Restart=on-failure
    RestartSec=5s

    [Install]
    WantedBy=multi-user.target

Copy the binary, set conservative ownership and permissions, then load and start the unit:

.. code-block:: console

    $ sudo install -o root -g root -m 0755 core_service_lifecycle /usr/local/libexec/catalog-monitor
    $ sudo systemctl daemon-reload
    $ sudo systemctl enable --now catalog-monitor.service
    $ systemctl status catalog-monitor.service
    $ journalctl -u catalog-monitor.service

By default, systemd sends ``SIGTERM`` when the unit stops.
The service lifecycle converts that signal into an orderly application shutdown, so no custom ``ExecStop`` command is
needed.
Choose ``TimeoutStopSec`` to accommodate the application's worst-case cleanup time; after that limit, systemd may force
termination.

To stop the unit and prevent it from starting at the next boot, disable it before removing its definition:

.. code-block:: console

    $ sudo systemctl disable --now catalog-monitor.service
    $ sudo rm /etc/systemd/system/catalog-monitor.service
    $ sudo systemctl daemon-reload

macOS with launchd
------------------

A system-wide daemon belongs in ``/Library/LaunchDaemons`` and runs without a logged-in user.
A process that serves only the current graphical session is usually a launch agent instead, placed in
``~/Library/LaunchAgents``.
The following system daemon assumes that a dedicated ``_catalogmonitor`` account already exists.

Install the executable at its permanent path:

.. code-block:: console

    $ sudo install -o root -g wheel -m 0755 core_service_lifecycle /usr/local/libexec/catalog-monitor

Then save this definition as ``/Library/LaunchDaemons/dev.erbsland.demo.catalog-monitor.plist``:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
        "https://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
    <dict>
        <key>Label</key>
        <string>dev.erbsland.demo.catalog-monitor</string>
        <key>ProgramArguments</key>
        <array>
            <string>/usr/local/libexec/catalog-monitor</string>
        </array>
        <key>UserName</key>
        <string>_catalogmonitor</string>
        <key>KeepAlive</key>
        <true/>
        <key>StandardOutPath</key>
        <string>/var/log/catalog-monitor.log</string>
        <key>StandardErrorPath</key>
        <string>/var/log/catalog-monitor-error.log</string>
    </dict>
    </plist>

``ProgramArguments`` contains one array element per argument, with the absolute executable path first.
``KeepAlive`` asks launchd to maintain the long-running process and also causes an initial launch.
For an on-demand service, declare the appropriate socket or Mach service instead of keeping the process alive
unconditionally.

Validate the file, set the ownership expected for a system daemon, and bootstrap it into the system domain:

.. code-block:: console

    $ plutil -lint dev.erbsland.demo.catalog-monitor.plist
    $ sudo install -o root -g wheel -m 0644 dev.erbsland.demo.catalog-monitor.plist \
        /Library/LaunchDaemons/dev.erbsland.demo.catalog-monitor.plist
    $ sudo launchctl bootstrap system /Library/LaunchDaemons/dev.erbsland.demo.catalog-monitor.plist
    $ sudo launchctl print system/dev.erbsland.demo.catalog-monitor

Use ``kickstart`` when you deliberately want to restart a loaded job.
Use ``bootout`` before deleting its definition:

.. code-block:: console

    $ sudo launchctl kickstart -k system/dev.erbsland.demo.catalog-monitor
    $ sudo launchctl bootout system/dev.erbsland.demo.catalog-monitor
    $ sudo rm /Library/LaunchDaemons/dev.erbsland.demo.catalog-monitor.plist

Reporting a Slow Windows Startup
================================

Windows expects a service to keep the SCM informed while it starts.
Erbsland Core normally handles this without application code: an application without parts becomes ready immediately
before main work begins, while an application with automatically started parts remains pending until every required part
has reached its running state.

Initialization outside the part manager sometimes takes longer—for example, validating a large catalog or opening a
remote data store.
Call :cpp:func:`reportStartupPending() <erbsland::core::Application::reportStartupPending>` before the lengthy phase,
update the positive estimate when meaningful progress is made, and call
:cpp:func:`reportStartupComplete() <erbsland::core::Application::reportStartupComplete>` as soon as the service can do
useful work.

.. erbsland-demo::
    :source: core/ServiceLifecycle/CatalogMonitorApplication.cpp
    :source-sha256: 11831bb8d989b985b8694d3f7919fa200711be7be6bef14fda568ee123e37398

.. code-block:: cpp

    /// Tell the service manager about initialization work that may take a while.
    ///
    /// The remaining time must be positive and should be a realistic upper estimate rather than a heartbeat interval.
    /// Once `reportStartupPending()` is called, every successful startup path must eventually call
    /// `reportStartupComplete()`.
    void CatalogMonitorApplication::initialize() {
        info().setApplicationName("Catalog Monitor"_el);
        info().setApplicationVersion(el::Version{1, 0, 0});

        // Publish an initial estimate before loading external configuration.
        reportStartupPending(el::Seconds{30});
        loadConfiguration();

        // Refine the estimate before opening the catalog and starting periodic work.
        reportStartupPending(el::Seconds{10});
        openCatalog();
        _refreshTimer = events()->createTimer([this]() -> void { refreshCatalog(); });
        _refreshTimer->startFixedDelay(el::Seconds{30});

        // Mark the service ready only after it can perform its advertised work.
        reportStartupComplete();
    }

    void CatalogMonitorApplication::cleanup() noexcept {
        _refreshTimer.reset();
        el::io::printLine("Catalog monitor stopped."_el);
    }

    void CatalogMonitorApplication::loadConfiguration() {
        el::io::printLine("Configuration loaded."_el);
    }

    void CatalogMonitorApplication::openCatalog() {
        el::io::printLine("Catalog opened."_el);
    }

    void CatalogMonitorApplication::refreshCatalog() {
        el::io::printLine("Catalog index refreshed."_el);
    }

.. erbsland-demo-end::

These reports affect a native service manager only when the executable is running as a Windows service.
They have no external effect on Linux, macOS, or an interactive Windows run, which keeps platform conditionals out of
the application; the framework still validates the call order and remaining duration on every platform.

An explicit pending report takes responsibility away from automatic readiness.
Every successful path after the first ``reportStartupPending()`` must therefore reach ``reportStartupComplete()``.
The completion call is idempotent, but returning to pending afterward is an error.
Reporting a zero or negative remaining duration is also an error, as is reporting state without first enabling the
service lifecycle.

The remaining duration is a wait hint, not a promise that startup may block indefinitely.
Estimate the whole next phase generously and update it as work advances; do not call the method from a tight timer only
to keep a stalled startup alive.
If shutdown begins while initialization is still running, stop that work promptly rather than attempting to report the
service as ready.

Windows Controls and Console Events
===================================

Once startup is complete, the Windows service accepts the SCM controls that have a direct application-lifecycle meaning.
The following mapping is deliberately small:

.. list-table:: Windows lifecycle mapping
    :header-rows: 1
    :widths: 34 66

    * - Native request
      - Application behavior
    * - SCM stop
      - Requests ``quit()`` with a successful exit code and begins orderly part and event-loop shutdown.
    * - SCM system shutdown
      - Follows the same orderly shutdown path as stop.
    * - SCM interrogate
      - Lets the SCM read the current native status; it does not call application code.
    * - :kbd:`Ctrl+C` in an interactive console
      - Requests the same orderly application shutdown.
    * - :kbd:`Ctrl+Break` in an interactive console
      - Requests the same orderly application shutdown.

Pause, continue, parameter-change, session-change, power, and custom controls are not accepted by this lifecycle.
If a service needs one of those protocols, model it through an application-level administrative interface instead of
assuming that it reaches an event callback.

During an orderly stop, the framework publishes a pending state, stops application parts in reverse dependency order,
finishes managed event loops, and finally reports the service stopped.
A zero application result becomes a successful service stop; a nonzero result is exposed to the SCM as a
service-specific failure.

Linux and macOS Signal Conventions
==================================

On POSIX systems, service integration focuses on graceful termination and leaves policy with the supervisor:

.. list-table:: POSIX lifecycle mapping
    :header-rows: 1
    :widths: 24 76

    * - Signal
      - Application behavior
    * - ``SIGTERM``
      - Requests ``quit()`` with a successful exit code and begins orderly shutdown.
    * - ``SIGINT``
      - Requests the same orderly shutdown, including when generated by :kbd:`Ctrl+C` in a terminal.
    * - ``SIGHUP``
      - Is not mapped to reload; the operating system's default handling remains in effect.
    * - ``SIGQUIT``
      - Is not mapped to graceful shutdown; the operating system's default handling remains in effect.

In particular, do not configure ``SIGHUP`` as a reload command unless the application separately installs and owns that
behavior.
Signals that cannot be caught, such as ``SIGKILL``, necessarily bypass application cleanup.
Set the supervisor's shutdown timeout long enough for ordinary cleanup, but keep cleanup bounded so an administrator is
not forced to escalate to a hard kill.

The lifecycle does not fork, create a PID file, change users, redirect streams, or send systemd or launchd readiness
notifications.
This is intentional: systemd and launchd already own process creation and supervision, while the unit or property-list
file provides a clear place for deployment policy.

A Predictable Process Boundary
==============================

Service support works best when it remains a thin boundary around an ordinary, well-structured application.
Enable it before ``run()``, keep the process in the foreground, let application parts and event loops perform their
normal orderly cleanup, and describe identity and restart policy in the native service definition.
The result is one executable that is pleasant to debug interactively and behaves predictably when the operating system
owns its lifetime.
