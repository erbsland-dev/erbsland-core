# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpClientBodyProgress.hpp
        HttpClientRedirectAction.hpp
        HttpClientRedirectConstraint.hpp
        HttpClientRedirectContext.hpp
        HttpClientRedirectHostPolicy.hpp
        HttpClientRedirectOptions.hpp
        HttpClientRequest.hpp
        HttpClientRequest_fwd.hpp
        HttpClientRequestEventEditor.hpp
        HttpClientRequestFn.hpp
        HttpClientResponse.hpp
        HttpClientResponse_fwd.hpp
        HttpClientResponseEventEditor.hpp
        HttpClientResponseOptions.hpp
        HttpClientSession.hpp
        HttpClientSession_fwd.hpp
        HttpClientSessionEventEditor.hpp
        HttpClientSessionOptions.hpp
        HttpClientTlsOptions.hpp
        HttpCookie.hpp
        HttpCookieInsertionOptions.hpp
        HttpCookieJar.hpp
        HttpCookieJar_fwd.hpp
        HttpCookieJarOptions.hpp
)
