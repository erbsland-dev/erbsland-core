// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CustomErrorApplication.hpp"

namespace demo {

/// Supply business work and a reporting destination when adapting this template.
class Application final : public CustomErrorApplication {
public:
    using CustomErrorApplication::CustomErrorApplication;

protected: // implement CustomErrorApplication
    void startWork() override {
        // Start your operations here; connect their terminal outcomes to quit().
    }
    void reportFailure([[maybe_unused]] const el::TextDocument &document) override {
        // Deliver the complete document to your chosen reporting destination.
    }
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::Application{argc, argv};
    return app.run();
}
