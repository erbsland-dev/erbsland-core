#include "WorkerServices.hpp"

namespace erbsland::util::impl::worker_services {

auto blocking() -> WorkerExecutor & {
    static auto *executor = new WorkerExecutor{};
    return *executor;
}

auto io() -> WorkerExecutor & {
    static auto *executor = new WorkerExecutor{};
    return *executor;
}

auto coroutine() -> WorkerExecutor & {
    static auto *executor = new WorkerExecutor{};
    return *executor;
}

}
