# Layout Renderer Profiler

`render-profile` measures tokenization, cold dependency-graph compilation, and warm rendering. Loader, resource-provider,
cache, fixture, filter, expected-count, and deterministic-validation work remains outside measured sections. Tokenizer
passes consume preloaded sources, compiler passes build fresh immutable graphs, and render passes reuse precompiled
graphs with deterministic contexts.

## Corpus and extension

The checked-in catalog in `src/Corpus.cpp` gives every fixture a stable ID, group, feature labels, source path,
repetition count, graph-wide byte/token/bytecode counts, output bytes, include executions, and deterministic digests.
The current corpus contains a loop-heavy HTML administration dashboard, a Unicode transactional email, a Markdown
incident report, expression- and control-flow-heavy layouts, raw-text-heavy input, custom delimiters, realistic and
stress include graphs, and realistic and stress inheritance graphs.
The inheritance graphs cover multi-level overrides, fallback and nested blocks, base-to-derived setup, direct and
captured `super` output, `super.super()`, loop-visible dispatch, and include/inheritance interaction.

To cover a new language feature:

1. Add a substantial valid source under `data/`.
2. Add one `CorpusEntry` with a new stable ID, group, feature labels, and repetition count.
3. Run a focused scenario once to obtain the validation values reported by the mismatch diagnostic, then check those
   values into the descriptor after reviewing the generated token, bytecode, and output counts.
4. Add the ID to `cmake/VerifyCoverage.cmake` and run the smoke, determinism, template, and coverage tests.

No workload code changes are needed for a new fixture or language feature.

## Configurations and commands

- `config/default.elcl` runs tokenizer, compiler, and renderer snapshots for every corpus and the aggregate.
- `config/smoke.elcl` is the fast correctness-only configuration used by CTest/CI.
- `config/profile-aggregate.elcl` supplies long-running work for a sampling profiler.

Build and run an optimized snapshot with:

```shell
cmake -S . -B cmake-build-profile -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build cmake-build-profile --target render-profile -j 8
cmake-build-profile/profiling-apps/render-profile \
    --config test/profiling/render/config/default.elcl --threads 1
```

Use repeatable `--scenario` filters for focused work, for example
`snapshot:compile:inheritance-stress`, `snapshot:render:include-stress`, or `snapshot:render:all`. CI checks
deterministic output and corpus coverage; it intentionally has no timing threshold.
