..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Usage

*****
Usage
*****

.. toctree::
    :maxdepth: 2
    :hidden:

    integrate-as-submodule
    build-configuration
    install-static-library

Erbsland Core is a static foundation library that can be built with your source tree or installed as a CMake package.
Source integration through a pinned Git submodule is our recommended way for any project.

.. grid:: 1
    :margin: 4 4 0 0
    :gutter: 1

    .. grid-item-card:: :fas:`code-branch;sd-text-success` Integrate as a Git Submodule
        :link: integrate-as-submodule
        :link-type: doc

        Place Core below ``erbsland/core``, aggregate Erbsland Core libraries in one CMake directory, and configure
        executables with Core's application setup helper.

    .. grid-item-card:: :fas:`sliders;sd-text-success` Configure the Build
        :link: build-configuration
        :link-type: doc

        Choose the supported test, demo, precompiled-header, namespace, developer, and unity-build settings.

    .. grid-item-card:: :fas:`box-archive;sd-text-success` Install the Static Library
        :link: install-static-library
        :link-type: doc

        Build and install Core into a prefix, then consume the package through ``find_package()``.
