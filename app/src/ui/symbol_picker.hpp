#pragma once

#include <zpew/data/manifest.hpp>

#include <optional>
#include <string>

namespace zpew::app {

class SymbolPicker {
public:
    [[nodiscard]] std::optional<std::string> draw(const data::Manifest& manifest,
                                                  const std::string& selected);

private:
    std::string filter_;
};

}
