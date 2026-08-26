# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpClientConnectionManager.cpp
        HttpClientConnectionManager.hpp
        HttpClientRequest.cpp
        HttpClientRequest.hpp
        HttpClientRequest_fwd.hpp
        HttpClientRequest_redirect.cpp
        HttpClientRequest_transaction.cpp
        HttpClientRequestEventEditor.cpp
        HttpClientRequestEventEditor.hpp
        HttpClientRequestEventEditor_fwd.hpp
        HttpClientResponse.cpp
        HttpClientResponse.hpp
        HttpClientResponse_fwd.hpp
        HttpClientResponse_output.cpp
        HttpClientResponseEventEditor.cpp
        HttpClientResponseEventEditor.hpp
        HttpClientResponseEventEditor_fwd.hpp
        HttpClientResponseHandler.hpp
        HttpClientSession.cpp
        HttpClientSession.hpp
        HttpClientSession_fwd.hpp
        HttpClientSessionEventEditor.cpp
        HttpClientSessionEventEditor.hpp
        HttpClientSessionEventEditor_fwd.hpp
)
