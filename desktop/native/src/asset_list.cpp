#include "asset_list.hpp"

#include "async_worker.hpp"
#include "js_error.hpp"

#include <zpew/data/iso_date.hpp>
#include <zpew/data/manifest.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/error.hpp>

#include <cstddef>
#include <memory>
#include <napi.h>
#include <string>
#include <utility>
#include <vector>

namespace zpew::native {

namespace {

struct AssetRow {
    std::string symbol;
    std::string name;
    std::string asset_class;
    std::string calendar;
    std::string source;
    std::string first_date;
    std::string last_date;
    std::size_t rows{};
};

class ListAssetsWorker final : public Napi::AsyncWorker {
public:
    ListAssetsWorker(Napi::Env env, std::string manifest_path)
        : Napi::AsyncWorker{env}, deferred_{Napi::Promise::Deferred::New(env)},
          manifest_path_{std::move(manifest_path)} {}

    [[nodiscard]] Napi::Promise promise() const { return deferred_.Promise(); }

    void Execute() override {
        try {
            const data::Manifest manifest = data::load_manifest(manifest_path_);
            for (const data::ManifestAsset& asset : manifest.assets()) {
                rows_.push_back({.symbol = asset.symbol,
                                 .name = asset.name,
                                 .asset_class = asset.asset_class,
                                 .calendar = std::string{data::to_string(asset.calendar)},
                                 .source = asset.source,
                                 .first_date = data::format_iso_date(asset.first_date),
                                 .last_date = data::format_iso_date(asset.last_date),
                                 .rows = asset.rows});
            }
        } catch (const Error& error) {
            SetError(error.what());
        }
    }

    void OnOK() override {
        const Napi::Env env = Env();
        Napi::Array result = Napi::Array::New(env, rows_.size());
        for (std::size_t i = 0; i < rows_.size(); ++i) {
            const AssetRow& row = rows_[i];
            Napi::Object item = Napi::Object::New(env);
            item.Set("symbol", row.symbol);
            item.Set("name", row.name);
            item.Set("assetClass", row.asset_class);
            item.Set("calendar", row.calendar);
            item.Set("source", row.source);
            item.Set("firstDate", row.first_date);
            item.Set("lastDate", row.last_date);
            item.Set("rows", static_cast<double>(row.rows));
            result.Set(static_cast<std::uint32_t>(i), item);
        }
        deferred_.Resolve(result);
    }

    void OnError(const Napi::Error& error) override { deferred_.Reject(error.Value()); }

private:
    Napi::Promise::Deferred deferred_;
    std::string manifest_path_;
    std::vector<AssetRow> rows_;
};

}

Napi::Value list_assets(const Napi::CallbackInfo& info) {
    return queue_worker(
        std::make_unique<ListAssetsWorker>(info.Env(), require_string(info, 0, "manifestPath")));
}

}
