// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/StdFormat.hpp>

#include <unordered_map>
#include <unordered_set>

TESTED_TARGETS(Parser)
TAGS(Parsing)

using PatternNodeId = impl::PatternNodeId;
using namespace impl::node_data;

class ParserNodeIdTest final : public UNITTEST_SUBCLASS(ParserBase) {
private:
    void requireValidIdTree(const PatternNodePtr &root) {
        REQUIRE(root != nullptr);
        REQUIRE_EQUAL(root->id(), static_cast<PatternNodeId>(0));
        REQUIRE(root->parent() == nullptr);

        std::unordered_map<PatternNodeId, const PatternNode *> nodes;
        std::unordered_set<PatternNodeId> ids;
        PatternNodeId maxId{0};

        root->traverse([&](const PatternNode &node, int /*depth*/) {
            const auto id = node.id();
            REQUIRE_FALSE(ids.contains(id));
            ids.insert(id);
            maxId = std::max(maxId, id);

            if (id != root->id()) {
                const auto parent = node.parent();
                REQUIRE(parent != nullptr);

                // Ensure parent is visited before children (pre-order traversal)
                // and that the parent pointer references the correct structural parent.
                const auto parentId = parent->id();
                REQUIRE(nodes.contains(parentId));
                REQUIRE_EQUAL(parent.get(), nodes.at(parentId));

                // Ensure parent really references this node.
                bool foundInParent = false;
                for (const auto &child : parent->children()) {
                    if (child.get() == &node) {
                        foundInParent = true;
                        break;
                    }
                }
                REQUIRE(foundInParent);
            } else {
                REQUIRE(node.parent() == nullptr);
            }

            nodes.insert({id, &node});
        });

        // IDs are intended to be unique and allocated consecutively (root=0, then 1..N).
        REQUIRE_EQUAL(ids.size(), nodes.size());
        REQUIRE_EQUAL(ids.size(), static_cast<std::size_t>(maxId + static_cast<PatternNodeId>(1)));
    }

public:
    void testNodeBehaviourDirect() {
        const auto root = std::make_shared<PatternNode>(0, Group{});
        REQUIRE_EQUAL(root->id(), static_cast<PatternNodeId>(0));
        REQUIRE(root->parent() == nullptr);

        const auto seq1 = std::make_shared<PatternNode>(1, Sequence{});
        root->addChild(seq1);
        REQUIRE(seq1->parent() != nullptr);
        REQUIRE_EQUAL(seq1->parent()->id(), root->id());
        REQUIRE_EQUAL(seq1->parent().get(), root.get());

        const auto a = std::make_shared<PatternNode>(2, CharacterSequence{el::text::Char{U'a'}});
        seq1->addChild(a);
        REQUIRE(a->parent() != nullptr);
        REQUIRE_EQUAL(a->parent()->id(), seq1->id());
        REQUIRE_EQUAL(a->parent().get(), seq1.get());

        // Replace last child should update parentId as well.
        const auto b = std::make_shared<PatternNode>(3, CharacterSequence{el::text::Char{U'b'}});
        seq1->replaceLastChild(b);
        REQUIRE(b->parent() != nullptr);
        REQUIRE_EQUAL(b->parent()->id(), seq1->id());
        REQUIRE_EQUAL(b->parent().get(), seq1.get());

        // Old node keeps its previous parent pointer (we don't clear it on detach).
        REQUIRE(a->parent() != nullptr);
        REQUIRE_EQUAL(a->parent()->id(), seq1->id());

        // Add two children.
        const auto seq2 = std::make_shared<PatternNode>(4, Sequence{});
        root->addChild(seq2);
        REQUIRE(seq2->parent() != nullptr);
        REQUIRE_EQUAL(seq2->parent()->id(), root->id());

        const auto x = std::make_shared<PatternNode>(5, CharacterSequence{el::text::Char{U'x'}});
        const auto y = std::make_shared<PatternNode>(6, CharacterSequence{el::text::Char{U'y'}});
        seq2->addChild(x);
        seq2->addChild(y);
        REQUIRE(x->parent() != nullptr);
        REQUIRE_EQUAL(x->parent()->id(), seq2->id());
        REQUIRE(y->parent() != nullptr);
        REQUIRE_EQUAL(y->parent()->id(), seq2->id());

        // Replace last child must update the parent.
        const auto z = std::make_shared<PatternNode>(7, CharacterSequence{el::text::Char{U'z'}});
        seq2->replaceLastChild(z);
        REQUIRE(z->parent() != nullptr);
        REQUIRE_EQUAL(z->parent()->id(), seq2->id());
    }

    void testParserIdTreeSimple() {
        parseTree("abc"_el);
        WITH_CONTEXT(requireValidIdTree(node));
    }

    void testParserIdTreeComplexPatterns() {
        const std::vector<String> patterns = {
            "a|b|c"_el,
            "(ab)c"_el,
            "(?:ab)+"_el,
            "(a(bc)d)e"_el,
            "^a.*b$"_el,
            "[a-zA-Z0-9_]+"_el,
            "(?im)^a.+?b$"_el,
            "((a)|(b(c)?))+"_el,
        };
        for (const auto &pattern : patterns) {
            runWithContext(
                SOURCE_LOCATION(),
                [&] {
                    parseTree(pattern);
                    requireValidIdTree(node);
                },
                [&]() -> std::string {
                    return std::format("pattern: \"{}\"", pattern.toSafeString(el::unit::CpLength{200U}));
                });
        }
    }
};
