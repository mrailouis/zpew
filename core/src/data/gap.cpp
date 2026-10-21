#include <zpew/data/bar.hpp>
#include <zpew/data/gap.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/error.hpp>

#include <cstddef>
#include <format>
#include <span>
#include <vector>

namespace zpew::data {

std::vector<Gap> find_gaps(std::span<const Bar> bars, TradingCalendar calendar) {
    std::vector<Gap> gaps;
    for (std::size_t i = 1; i < bars.size(); ++i) {
        const Bar& previous = bars[i - 1];
        const Bar& current = bars[i];
        if (current.date <= previous.date) {
            throw InvalidArgument{
                std::format("find_gaps requires strictly increasing dates; {} follows {}",
                            format_iso_date(current.date), format_iso_date(previous.date))};
        }
        const int missing = sessions_between(previous.date, current.date, calendar);
        if (missing > max_missing_sessions(calendar)) {
            gaps.push_back({.last_before = previous.date,
                            .first_after = current.date,
                            .missing_sessions = missing});
        }
    }
    return gaps;
}

}
