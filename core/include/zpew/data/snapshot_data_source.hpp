#pragma once

#include <zpew/data/bar.hpp>
#include <zpew/data/data_source.hpp>
#include <zpew/data/manifest.hpp>
#include <zpew/data/snapshot.hpp>

#include <filesystem>
#include <string_view>
#include <vector>

namespace zpew::data {

class SnapshotDataSource final : public DataSource {
public:
    explicit SnapshotDataSource(const std::filesystem::path& manifest_path);

    [[nodiscard]] std::vector<Bar> load(std::string_view symbol) const override;
    [[nodiscard]] Snapshot load_snapshot(std::string_view symbol) const;
    [[nodiscard]] const Manifest& manifest() const noexcept;

private:
    Manifest manifest_;
};

}
