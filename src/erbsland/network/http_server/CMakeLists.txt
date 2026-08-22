# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpCookieSecurePolicy.hpp
        HttpCookieSessionManager.cpp
        HttpCookieSessionManager.hpp
        HttpCookieSessionManager_fwd.hpp
        HttpCookieSessionManagerOptions.hpp
        HttpMediaTypeMapping.cpp
        HttpMediaTypeMapping.hpp
        HttpMediaTypeMapping_fwd.hpp
        HttpServer.hpp
        HttpServer_fwd.hpp
        HttpServerEventEditor.hpp
        HttpServerOptions.hpp
        HttpServerRequest.hpp
        HttpServerRequest_fwd.hpp
        HttpServerRequestEventEditor.hpp
        HttpServerRequestFn.hpp
        HttpServerRouteOptions.hpp
        HttpServerSession.hpp
        HttpServerSession_fwd.hpp
        HttpServerSessionContext.hpp
        HttpServerSessionEventEditor.hpp
        HttpServerSessionManager.hpp
        HttpServerSessionManager_fwd.hpp
        HttpServerSessionSelection.hpp
        HttpServerTlsOptions.hpp
        HttpSessionData.hpp
        HttpStaticContent.cpp
        HttpStaticContent.hpp
        HttpStaticContent_fwd.hpp
        HttpStaticContentHandler.cpp
        HttpStaticContentHandler.hpp
        HttpStaticContentHandler_fwd.hpp
        HttpStaticFileHandler.cpp
        HttpStaticFileHandler.hpp
        HttpStaticFileHandler_fwd.hpp
        HttpStaticResourceHandler.cpp
        HttpStaticResourceHandler.hpp
        HttpStaticResourceHandler_fwd.hpp
)
