# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Tls13Hkdf.cpp
        Tls13Hkdf.hpp
        Tls13KeySchedule.cpp
        Tls13KeySchedule.hpp
        Tls13KeyScheduleTestAccess.hpp
        Tls13KeyScheduleTestAccess_fwd.hpp
        Tls13Transcript.cpp
        Tls13Transcript.hpp
        TlsRecordState.cpp
        TlsRecordState.hpp
        TlsRecordState_fwd.hpp
        TlsRecordTestAccess.hpp
)
