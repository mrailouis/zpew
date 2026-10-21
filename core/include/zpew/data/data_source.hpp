#pragma once

#include <zpew/data/bar.hpp>

#include <string_view>
#include <vector>

namespace zpew::data {

class DataSource {
public:
    DataSource() = default;
    virtual ~DataSource() = default;

    DataSource(const DataSource&) = delete;
    DataSource& operator=(const DataSource&) = delete;
    DataSource(DataSource&&) = delete;
    DataSource& operator=(DataSource&&) = delete;

    [[nodiscard]] virtual std::vector<Bar> load(std::string_view symbol) const = 0;
};

}
