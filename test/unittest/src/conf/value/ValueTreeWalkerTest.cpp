// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/value/ValueTreeWalker.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringList.hpp>

using namespace el;
using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Parser)
class ValueTreeWalkerTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
private:
    static el::text::StringList collectPreorder(
        const DocumentPtr &doc, const impl::ValueTreeWalker::Filter &filter = {}) {
        el::text::StringList out;
        impl::ValueTreeWalker walker;
        auto root = std::dynamic_pointer_cast<el::conf::Value>(doc);
        walker.setRoot(root);
        if (filter)
            walker.setFilter(filter);
        walker.walk([&out](const el::conf::ConstValuePtr &node) {
            const auto np = node->namePath().toText();
            out.append(np.isEmpty() ? el::text::String{"<root>"_el} : np);
        });
        return out;
    }

    static el::text::StringList collectPreorder(
        const el::conf::ValuePtr &root, const impl::ValueTreeWalker::Filter &filter = {}) {
        el::text::StringList out;
        impl::ValueTreeWalker walker;
        walker.setRoot(root);
        if (filter)
            walker.setFilter(filter);
        walker.walk([&out](const el::conf::ConstValuePtr &node) {
            const auto np = node->namePath().toText();
            out.append(np.isEmpty() ? el::text::String{"<root>"_el} : np);
        });
        return out;
    }

public:
    void testPreorderTraversalAndFilter() {
        // Build a simple document with nested sections and values.
        const el::text::String text = R"(
# Simple tree
[main]
a = 1
[main.sub]
b = 2
[other]
c = 3
)"_el;
        auto source = createTestMemorySource(text);
        Parser parser;
        auto doc = parser.parseOrThrow(source);
        REQUIRE(doc != nullptr);

        // Collect visited name paths in order, pruning "main.sub".
        const auto visited = collectPreorder(
            doc, [](const el::conf::ConstValuePtr &node) { return node->namePath().toText() != "main.sub"_el; });

        // Expect preorder in declaration order, with "main.sub" and its child pruned.
        const auto expected = el::text::StringList{
            "<root>"_el,
            "main"_el,
            "main.a"_el,
            "other"_el,
            "other.c"_el,
        };
        REQUIRE_EQUAL(visited, expected);

        // Also verify that starting from the Value overload yields the same order when not pruning.
        const auto asValue = std::dynamic_pointer_cast<el::conf::Value>(doc);
        REQUIRE(asValue != nullptr);
        const auto visitedFull = collectPreorder(asValue);

        // Now we expect the full tree (no pruning).
        const auto expectedFull = el::text::StringList{
            "<root>"_el,
            "main"_el,
            "main.a"_el,
            "main.sub"_el,
            "main.sub.b"_el,
            "other"_el,
            "other.c"_el,
        };
        REQUIRE_EQUAL(visitedFull, expectedFull);
    }

    void testLargerDocumentTraversal() {
        // Larger tree with multiple siblings and nested subsections.
        const el::text::String text = R"(
# Larger tree
[app]
name = "demo"
version = 1
[app.ui]
theme = "dark"
[app.ui.colors]
primary = "#123456"
[app.modules]
# sibling subsections A and B
[app.modules.A]
enabled = enabled
[app.modules.B]
level = 3
[db]
host = "localhost"
port = 5432
)"_el;
        Parser parser;
        auto doc = parser.parseOrThrow(createTestMemorySource(text));

        const auto visited = collectPreorder(doc);
        const auto requiredNodes = el::text::StringList{
            "<root>"_el,
            "app"_el,
            "app.name"_el,
            "app.version"_el,
            "app.ui"_el,
            "app.ui.theme"_el,
            "app.ui.colors.primary"_el,
            "db.host"_el,
            "db.port"_el,
        };
        // Ensure all required nodes are present.
        for (const auto &name : requiredNodes) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    bool found = false;
                    for (const auto &v : visited) {
                        if (v == name) {
                            found = true;
                            break;
                        }
                    }
                    REQUIRE(found);
                },
                [&]() -> std::string {
                    return std::format(
                        "Required node not found in traversal: {}", el::text::StringConverter{name}.toStdString());
                });
        }
        // Verify preorder constraints: parents before their children.
        auto indexOf = [&](const el::text::String &name) -> std::size_t {
            auto index = std::size_t{0};
            for (const auto &visitedName : visited) {
                if (visitedName == name) {
                    return index;
                }
                ++index;
            }
            return visited.count().toSizeT();
        };
        REQUIRE(indexOf("<root>"_el) == 0);
        REQUIRE(indexOf("app"_el) < indexOf("app.name"_el));
        REQUIRE(indexOf("app"_el) < indexOf("app.version"_el));
        REQUIRE(indexOf("app"_el) < indexOf("app.ui"_el));
        REQUIRE(indexOf("app.ui"_el) < indexOf("app.ui.theme"_el));
        // Accept either implicit or explicit colors section node.
        REQUIRE(indexOf("app.ui"_el) < indexOf("app.ui.colors.primary"_el));
        // Modules ordering
        REQUIRE(indexOf("app"_el) < indexOf("app.modules.A.enabled"_el));
        REQUIRE(indexOf("app"_el) < indexOf("app.modules.B.level"_el));
        // DB ordering
        REQUIRE(indexOf("db.host"_el) > indexOf("<root>"_el));
        REQUIRE(indexOf("db.port"_el) > indexOf("<root>"_el));
    }

    void testExceptionPropagationFromVisit() {
        const el::text::String text = R"(
[root]
a = 1
)"_el;
        Parser parser;
        auto doc = parser.parseOrThrow(createTestMemorySource(text));

        struct Boom {};
        bool thrown = false;
        try {
            impl::ValueTreeWalker walker;
            auto root = std::dynamic_pointer_cast<el::conf::Value>(doc);
            walker.setRoot(root);
            walker.walk([](const el::conf::ConstValuePtr &node) {
                if (node->namePath().toText() == "root.a"_el) {
                    throw Boom{};
                }
            });
        } catch (const Boom &) {
            thrown = true;
        }
        REQUIRE(thrown);
    }

    void testExceptionPropagationFromFilter() {
        const el::text::String text = R"(
[root]
a = 1
)"_el;
        Parser parser;
        auto doc = parser.parseOrThrow(createTestMemorySource(text));

        struct MyError {};
        bool thrown = false;
        try {
            impl::ValueTreeWalker walker;
            auto root = std::dynamic_pointer_cast<el::conf::Value>(doc);
            walker.setRoot(root);
            walker.setFilter([](const el::conf::ConstValuePtr &node) -> bool {
                if (node->namePath().toText() == "root"_el) {
                    throw MyError{};
                }
                return true;
            });
            walker.walk(impl::ValueTreeWalker::Visit{});
        } catch (const MyError &) {
            thrown = true;
        }
        REQUIRE(thrown);
    }
};
