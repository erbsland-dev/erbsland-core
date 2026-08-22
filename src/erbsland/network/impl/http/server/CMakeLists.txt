# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpRequestTarget.cpp
        HttpRequestTarget.hpp
        HttpRouteHandler.cpp
        HttpRouteHandler.hpp
        HttpRoutes.cpp
        HttpRoutes.hpp
        HttpServer.cpp
        HttpServer.hpp
        HttpServer_fwd.hpp
        HttpServerConnection.cpp
        HttpServerConnection.hpp
        HttpServerConnection_fwd.hpp
        HttpServerEventEditor.cpp
        HttpServerEventEditor.hpp
        HttpServerEventEditor_fwd.hpp
        HttpServerRequest.cpp
        HttpServerRequest.hpp
        HttpServerRequest_fwd.hpp
        HttpServerRequestEventEditor.cpp
        HttpServerRequestEventEditor.hpp
        HttpServerRequestEventEditor_fwd.hpp
        HttpServerSession.cpp
        HttpServerSession.hpp
        HttpServerSession_fwd.hpp
        HttpServerSessionEventEditor.cpp
        HttpServerSessionEventEditor.hpp
        HttpServerSessionEventEditor_fwd.hpp
)
