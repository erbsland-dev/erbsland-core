..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************
Use the Maintenance Tool
************************

``erbsland-core-tool`` runs maintenance commands for the Erbsland Core repository.
It builds documentation and regenerates checked-in library data.
The :doc:`repository setup <../setup>` installs the executable.

Start with :doc:`usage` for shared options, then follow the guide for the command you need.
The snapshot-update guides explain how to acquire and review new upstream data before regenerating tables.
To extend the tool, see :doc:`../tool_dev/index`.

.. toctree::
    :maxdepth: 1

    usage
    pre_commit
    doc_build
    doc_api_guidelines
    doc_api_test_status
    doc_update_reference
    doc_preprocess_sources
    create_demo
    doc_process_rst
    elcl_format
    code_cleanup
    anti_patterns
    generate_display_texts
    generate_idna_data
    generate_unicode_data
    generate_category_test_data
    generate_time_zone_data
    generate_common_box_frame_style
    generate_frame_border_joint_chars
    generate_public_suffix_data
    rendering_configuration
    update_unicode
    update_public_suffix
    update_public_headers
    generate_saturating_math_data
    generate_time_tests
    import_cryptology_vectors
    security_hashes
    github_workflows
    include_dependencies
