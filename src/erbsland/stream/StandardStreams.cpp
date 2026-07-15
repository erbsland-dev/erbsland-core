// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreams.hpp"

#include "impl/StandardStreamRegistry.hpp"
#include "impl/StandardStreamSlot.hpp"

#include <utility>

namespace erbsland::stream {

auto stdIn() -> TextInputStreamPtr {
    return impl::standardStreamRegistry().inputProxy();
}

auto stdOut() -> TextOutputStreamPtr {
    return impl::standardStreamRegistry().outputProxy();
}

auto stdErr() -> TextOutputStreamPtr {
    return impl::standardStreamRegistry().errorProxy();
}

auto redirectStdOut(TextOutputStreamPtr output) -> StandardStreamRedirect {
    return StandardStreamRedirect{
        impl::standardStreamRegistry().replace(impl::StandardStreamSlot::Out, {}, std::move(output), {})};
}

auto redirectStdErr(TextOutputStreamPtr error) -> StandardStreamRedirect {
    return StandardStreamRedirect{
        impl::standardStreamRegistry().replace(impl::StandardStreamSlot::Err, {}, {}, std::move(error))};
}

auto redirectStdIn(TextInputStreamPtr input) -> StandardStreamRedirect {
    return StandardStreamRedirect{
        impl::standardStreamRegistry().replace(impl::StandardStreamSlot::In, std::move(input), {}, {})};
}

auto redirectStandardStreams(TextOutputStreamPtr output, TextOutputStreamPtr error) -> StandardStreamRedirect {
    return StandardStreamRedirect{impl::standardStreamRegistry().replace(
        impl::StandardStreamSlot::Both, {}, std::move(output), std::move(error))};
}

}
