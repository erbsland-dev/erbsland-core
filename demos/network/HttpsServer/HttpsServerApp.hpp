// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamedResponse.hpp"

#include <erbsland/all.hpp>
#include <erbsland/network/http_server/all.hpp>

#include <memory>
#include <vector>

namespace demo {

/// A standalone HTTPS server demonstrating routes, directory overlays, streamed output, and graceful shutdown.
/// @notest{Compiled and exercised as part of the HTTPS server demo.}
class HttpsServerApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void parseCommandLine() override;

private:
    void configureTls();
    void configureStaticContent();
    void configureRoutes();
    void startServer();
    void onListening();
    void closeGracefully();
    void removeResponse(StreamedResponse *finished);

private:
    el::IpAddress _address;                                    ///< Local IP address selected on the command line.
    el::Port _port;                                            ///< Local port selected on the command line.
    el::Path _certificatePath;                                 ///< PEM certificate-chain path.
    el::Path _privateKeyPath;                                  ///< Matching PKCS#8 private-key path.
    std::vector<el::Path> _staticRoots;                        ///< Highest-first static overlay roots.
    el::Seconds _runFor;                                       ///< Optional duration before graceful shutdown.
    el::HttpServerPtr _server;                                 ///< Active event-loop-owned HTTPS server.
    std::vector<std::shared_ptr<StreamedResponse>> _responses; ///< Active generated response pumps.
    bool _closing{};                                           ///< Whether graceful shutdown has started.
};

}
