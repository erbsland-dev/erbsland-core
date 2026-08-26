// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

#include <utility>

namespace demo {

using namespace el::text::literals;

/// Keep the behavior of the `list` command next to the module that describes its command line.
class ListAction final {
public:
    [[nodiscard]] auto module() -> el::OptionModulePtr {
        auto result = el::OptionModule::create("list"_el);
        result->setHelpDescription("Lists the entries in the deployment catalog."_el);
        result->setMainFn([this](el::OptionValuesPtr values) -> el::ExitCode { return main(std::move(values)); });
        return result;
    }

private:
    [[nodiscard]] auto main([[maybe_unused]] const el::OptionValuesPtr &values) -> el::ExitCode {
        el::io::printLine("catalog entries: api, worker"_el);
        return el::ExitCode::success();
    }
};

/// Keep the behavior and options of the `add` command in one action object.
class AddAction final {
public:
    [[nodiscard]] auto module() -> el::OptionModulePtr {
        auto result = el::OptionModule::create("add"_el);
        result->setHelpDescription("Adds an entry to the deployment catalog."_el);
        result->addOption("name"_el).setRequired().setHelpDescription("Name of the new catalog entry."_el);
        result->setMainFn([this](el::OptionValuesPtr values) -> el::ExitCode { return main(std::move(values)); });
        return result;
    }

private:
    [[nodiscard]] auto main(const el::OptionValuesPtr &values) -> el::ExitCode {
        el::io::printLine("adding catalog entry: "_el, values->getText("name"_el));
        return el::ExitCode::success();
    }
};

/// Keep the behavior and options of the `remove` command in one action object.
class RemoveAction final {
public:
    [[nodiscard]] auto module() -> el::OptionModulePtr {
        auto result = el::OptionModule::create("remove"_el);
        result->setHelpDescription("Removes an entry from the deployment catalog."_el);
        result->addOption("name"_el).setRequired().setHelpDescription("Name of the catalog entry to remove."_el);
        result->setMainFn([this](el::OptionValuesPtr values) -> el::ExitCode { return main(std::move(values)); });
        return result;
    }

private:
    [[nodiscard]] auto main(const el::OptionValuesPtr &values) -> el::ExitCode {
        el::io::printLine("removing catalog entry: "_el, values->getText("name"_el));
        return el::ExitCode::success();
    }
};

/// Use option modules to dispatch a command-style application without a manual command switch.
///
/// The application owns every action object so callbacks that capture `this` remain valid for the complete run.
/// It deliberately keeps the base `Application::main()` implementation, which calls the selected module main function.
class CatalogApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        info().setApplicationName("Deployment Catalog"_el);
        info().setApplicationVersion(el::Version{1, 0, 0});
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Manages a small catalog of deployable services."_el);
        options->addModule(_listAction.module());
        options->addModule(_addAction.module());
        options->addModule(_removeAction.module());
    }

private:
    ListAction _listAction;
    AddAction _addAction;
    RemoveAction _removeAction;
};

/// Create the command application and let the base main implementation dispatch the selected module.
auto main(const int argc, char *argv[]) -> int {
    auto app = CatalogApplication{argc, argv};
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
