#pragma once

#include <zpew/data/bar.hpp>
#include <zpew/data/gap.hpp>
#include <zpew/data/manifest.hpp>

#include <vector>

namespace zpew::data {

struct Snapshot {
    ManifestAsset asset;
    std::vector<Bar> bars;
    std::vector<Gap> gaps;
};

}
