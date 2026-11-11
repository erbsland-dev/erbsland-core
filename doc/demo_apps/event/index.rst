..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************
Event Applications
******************

These applications show several ways to organize work around the event system.
Start with the monitors for a small main-loop application, then choose a walkthrough by the execution pattern you need.
:doc:`/topics/event/events_in_erbsland_core` introduces loops, targets, sources, and editors before these concrete designs.

.. list-table::
    :header-rows: 1

    * - Application
      - What it demonstrates
    * - :doc:`file_size_monitor`
      - Direct application configuration, retained timer, and completion
    * - :doc:`file_time_monitor`
      - Derived application with procedural setup around main-loop dispatch
    * - :doc:`file_activity_monitor`
      - Fixed-rate file polling, a restartable inactivity timeout, and an overall deadline
    * - :doc:`file_hasher`
      - Procedural main, event workers, a result receiver, and joining
    * - :doc:`config_check`
      - Independent file checks, stored-error reporting, and an optional custom error limit
    * - :doc:`git_change_summary`
      - Procedural preparation, lazy subprocess binding, and output processing on an event thread
    * - :doc:`file_server`
      - An application part owning an HTTP service and graceful shutdown
    * - :doc:`file_search`
      - Interactive glob search, progressive worker results, obsolete replies, and cancellation
    * - :doc:`pi_calculator`
      - Required results, optional display callbacks, batching, and cancellation
    * - :doc:`lookup_lost_source`
      - Missing callbacks after a local source is destroyed
    * - :doc:`lookup_owned_source`
      - An application member retaining a source until completion
    * - :doc:`lookup_required_recipient`
      - Strong callback ownership of a required report
    * - :doc:`lookup_optional_recipient`
      - Weak progress reporting with independently retained results
    * - :doc:`lookup_ownership_cycle`
      - Observing and explicitly breaking a controller/source cycle
    * - :doc:`checksum_packer`
      - Sequential workflow stages coordinated through bound callbacks
    * - :doc:`checksum_packer_invoke`
      - The same workflow with explicit target invocations and captures

.. toctree::
    :maxdepth: 1

    file_size_monitor
    file_time_monitor
    file_activity_monitor
    file_hasher
    config_check
    git_change_summary
    file_server
    file_search
    pi_calculator
    lookup_lost_source
    lookup_owned_source
    lookup_required_recipient
    lookup_optional_recipient
    lookup_ownership_cycle
    checksum_packer
    checksum_packer_invoke
