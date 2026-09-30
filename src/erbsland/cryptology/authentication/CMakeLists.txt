# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AuthenticationTokenPair.cpp
        AuthenticationTokenPair.hpp
        AuthenticationTokenPair_fwd.hpp
        ClientAuthenticationToken.cpp
        ClientAuthenticationToken.hpp
        ClientAuthenticationToken_fwd.hpp
        PendingAuthenticationChallenge.cpp
        PendingAuthenticationChallenge.hpp
        PendingAuthenticationChallenge_fwd.hpp
        ServerAuthenticationToken.cpp
        ServerAuthenticationToken.hpp
        ServerAuthenticationToken_fwd.hpp
)
