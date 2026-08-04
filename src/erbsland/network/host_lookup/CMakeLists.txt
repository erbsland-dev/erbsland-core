# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HostLookup.hpp
        HostLookup_fwd.hpp
        HostLookupEventEditor.hpp
        HostLookupEventEditor_fwd.hpp
        HostLookupOptions.hpp
        HostLookupOptions_fwd.hpp
        HostResolvedFn.hpp
)
