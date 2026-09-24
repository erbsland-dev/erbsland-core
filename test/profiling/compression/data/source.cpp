// Original source-like input used only as compression profiling data.

#include <cstdint>
#include <string_view>
#include <vector>

namespace example::archive {

struct Record {
    std::uint64_t identifier{};
    std::string_view category;
    std::vector<std::uint8_t> payload;
};

class RecordBatch final {
public:
    explicit RecordBatch(const std::size_t capacity) { _records.reserve(capacity); }

    void append(Record record) { _records.push_back(std::move(record)); }

    [[nodiscard]] auto payloadSize() const noexcept -> std::size_t {
        auto result = std::size_t{};
        for (const auto &record : _records) {
            result += record.payload.size();
        }
        return result;
    }

    [[nodiscard]] auto find(const std::uint64_t identifier) const noexcept -> const Record * {
        for (const auto &record : _records) {
            if (record.identifier == identifier) {
                return &record;
            }
        }
        return nullptr;
    }

private:
    std::vector<Record> _records;
};

}
