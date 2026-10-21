#include "ui/symbol_picker.hpp"

#include <zpew/data/manifest.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <format>
#include <imgui.h>
#include <optional>
#include <string>
#include <string_view>

namespace zpew::app {

namespace {

constexpr std::size_t kFilterCapacity = 64;

std::string lower(std::string_view text) {
    std::string result{text};
    std::ranges::transform(result, result.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

bool matches(const data::ManifestAsset& asset, const std::string& filter) {
    if (filter.empty()) {
        return true;
    }
    const std::string needle = lower(filter);
    return lower(asset.symbol).find(needle) != std::string::npos ||
           lower(asset.name).find(needle) != std::string::npos ||
           lower(asset.asset_class).find(needle) != std::string::npos;
}

}

std::optional<std::string> SymbolPicker::draw(const data::Manifest& manifest,
                                              const std::string& selected) {
    std::array<char, kFilterCapacity> buffer{};
    std::ranges::copy(std::string_view{filter_}.substr(0, kFilterCapacity - 1), buffer.begin());
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::InputTextWithHint("##filter", "filter symbols", buffer.data(), buffer.size())) {
        filter_ = buffer.data();
    }

    std::optional<std::string> chosen;
    if (!ImGui::BeginChild("##symbols")) {
        ImGui::EndChild();
        return chosen;
    }
    std::string_view current_class;
    for (const data::ManifestAsset& asset : manifest.assets()) {
        if (!matches(asset, filter_)) {
            continue;
        }
        if (asset.asset_class != current_class) {
            current_class = asset.asset_class;
            ImGui::SeparatorText(asset.asset_class.c_str());
        }
        const std::string label = std::format("{:<8} {}", asset.symbol, asset.name);
        if (ImGui::Selectable(label.c_str(), asset.symbol == selected)) {
            chosen = asset.symbol;
        }
    }
    ImGui::EndChild();
    return chosen;
}

}
