#pragma once

#include <ql/quantlib.hpp>
#include <utility>

namespace QuantLib {
    namespace Utils {
        // is if p1 is a multiple of p2?
        inline std::pair<bool, Integer> isMultiple(
            const Period& p1,
            const Period& p2
        ) {
            // Basic check: if p2 is longer than p1, p1 can't be a multiple of p2 (unless 0)
            // Note: QuantLib throws if comparison is undecidable (e.g. 1M vs 30D)
            try {
                if (p1 < p2) return { false, Null<Integer>()};
            }
            catch (...) {
                // Handle undecidable cases or return false/error
                return { false, Null<Integer>() };
            }
            // Example logic for matching units:
            if (p1.units() == p2.units()) {
                auto multiple = (p1.length() % p2.length() == 0);
                return { multiple, (multiple ? p1.length()/ p2.length() : Null<Integer>())};
            }
            // Convert Years to Months for comparison
            if ((p1.units() == Years || p1.units() == Months) &&
                (p2.units() == Years || p2.units() == Months)) {
                int p1_months = (p1.units() == Years) ? p1.length() * 12 : p1.length();
                int p2_months = (p2.units() == Years) ? p2.length() * 12 : p2.length();
                auto multiple = (p1_months % p2_months == 0);
                return { multiple, (multiple ? p1_months/ p2_months : Null<Integer>())};
            }
            // Convert Weeks to Days for comparison
            if ((p1.units() == Weeks || p1.units() == Days) &&
                (p2.units() == Weeks || p2.units() == Days)) {
                int p1_days = (p1.units() == Weeks) ? p1.length() * 7 : p1.length();
                int p2_days = (p2.units() == Weeks) ? p2.length() * 7 : p2.length();
                auto multiple = (p1_days % p2_days == 0);
                return { multiple, (multiple ? p1_days/ p2_days : Null<Integer>())};
            }
            return { false, Null<Integer>() }; // Units are incompatible (e.g., Months vs Days)
        }        
    }
}