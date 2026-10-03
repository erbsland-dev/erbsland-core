// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Scanner.hpp"

#include "SourceIndex.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>
#include <erbsland/WorkerPool.hpp>

#include <algorithm>
#include <compare>

namespace tool::anti_patterns {

using namespace el::text::literals;

Scanner::Scanner(
    const Policy &policy, const RuleRegistry &rules, const frame::ProjectFiles &files, frame::TaskProgress progress) :
    _progress{std::move(progress)}, _policy{policy}, _rules{rules}, _files{files} {
}

auto Scanner::inventory(const el::StringList &requested, const el::OptionValuePtr &requestedOption) const
    -> el::List<Job> {
    auto roots = el::PathList{};
    if (requested.isEmpty()) {
        roots = _policy.roots(_files);
    } else {
        for (const auto &name : requested) {
            try {
                const auto relative = el::Path{name};
                if (name.isEmpty() || !relative.isRelative() || relative.elements().contains(".."_el)) {
                    throw el::ApplicationError{
                        "Anti-Pattern Scan Failed"_el, "Requested paths must stay inside the project."_el};
                }
                const auto checked = _files.checked(relative);
                if (!checked.info().isDirectory() && (!checked.info().isRegularFile() || !_policy.accepts(checked))) {
                    throw el::ApplicationError{
                        "Anti-Pattern Scan Failed"_el,
                        "Requested scan path is missing or has an unsupported suffix."_el}
                        .withSourcePath(name);
                }
                roots.append(checked);
            } catch (const el::ApplicationError &error) {
                if (requestedOption) {
                    throw el::OptionError{
                        "Invalid Scan Path"_el, error.description(), requestedOption, std::current_exception()};
                }
                throw;
            }
        }
    }
    auto paths = el::StringMap<el::String>{};
    for (const auto &root : roots) {
        if (root.info().isRegularFile()) {
            paths.set(root.toString(), FindingCache::signature(root.info()));
            continue;
        }
        root.walker().walkOrThrow(
            [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
                if (info.isRegularFile() && _policy.accepts(path)) {
                    paths.set(path.toString(), FindingCache::signature(info));
                }
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    }
    auto jobs = el::List<Job>{};
    jobs.reserve(paths.count());
    for (const auto &[name, signature] : paths) {
        const auto path = el::Path{name};
        jobs.append({path, path.toRelativeOrThrow(_files.projectRoot()), signature});
    }
    return jobs;
}

auto Scanner::scanFile(const Job &job) const -> Result {
    const auto file = _files.readWalkedTextFile(job.absolute);
    const auto source = SourceIndex{job.relative, file.text()};
    auto result = Result{};
    for (const auto &rule : _rules.all()) {
        auto candidates = el::List<el::ByteRange>{};
        rule->scan(source, candidates);
        for (const auto &candidate : candidates) {
            if (source.insideOsConditional(candidate.beginIndex())) {
                continue;
            }
            const auto start = candidate.beginIndex();
            const auto end = std::max(candidate.endIndex(), start.incremented());
            result.findings.append(
                Finding{
                    rule->identifier(),
                    rule->severity(),
                    job.relative,
                    el::ByteRange{start, end},
                    el::LineRange{source.lineIndex(start), source.lineIndex(end.decremented()).incremented()},
                    source.snippet(el::ByteRange{start, end}),
                    source.inlineReason(rule->identifier(), el::ByteRange{start, end}),
                    {}});
        }
    }
    result.signature = FindingCache::signature(job.absolute);
    return result;
}

auto Scanner::scan(
    const el::StringList &requested, const unsigned int workers, const el::OptionValuePtr &requestedOption)
    -> el::List<Finding> {
    _progress.beginPhase("Discovering source files"_el);
    const auto jobs = inventory(requested, requestedOption);
    _fileCount = jobs.count();
    _progress.beginPhase("Checking source files"_el, jobs.count());
    auto cache = FindingCache{_files, _rules};
    auto misses = el::List<Job>{};
    auto findings = el::List<Finding>{};
    auto selected = el::StringSet{};
    for (const auto &job : jobs) {
        selected.insert(job.relative.toString());
        if (const auto *cached = cache.get(job.relative, job.signature)) {
            ++_cacheHits;
            _progress.advance();
            findings.append(*cached);
        } else {
            misses.append(job);
        }
    }
    _cacheMisses = misses.count();
    if (!misses.isEmpty()) {
        const auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(workers));
        const auto results = pool->map(
                                     misses,
                                     [&](const Job &job) -> Result {
                                         auto result = scanFile(job);
                                         _progress.advance();
                                         return result;
                                     })
                                 .get();
        for (auto index = el::ItemIndex::zero(); index.isWithin(misses.count()); ++index) {
            const auto &job = misses.getRef(index);
            const auto &result = results.getRef(index);
            findings.append(result.findings);
            if (job.signature == result.signature) {
                cache.put(job.relative, job.signature, result.findings);
            }
        }
    }
    _progress.beginPhase("Collecting findings"_el);
    if (requested.isEmpty()) {
        cache.retain(selected);
    }
    cache.save();
    findings.map([&](Finding finding) -> Finding {
        if (finding.inlineReason.isEmpty()) {
            finding.configReason = _policy.reason(finding.path, finding.rule);
        }
        return finding;
    });
    findings.sort([](const Finding &left, const Finding &right) -> bool {
        if (left.severity != right.severity) {
            return left.severity < right.severity;
        }
        if (left.rule != right.rule) {
            return left.rule < right.rule;
        }
        const auto paths = left.path.toString().compare(right.path.toString(), el::Char::compareCaseFolded);
        if (paths != std::strong_ordering::equal) {
            return paths == std::strong_ordering::less;
        }
        return left.lines.beginIndex() < right.lines.beginIndex();
    });
    return findings;
}

}
