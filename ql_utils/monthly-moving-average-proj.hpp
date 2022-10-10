#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <ql_utils/types.hpp>

namespace QLUtils {
    template <
        size_t MovingAverageMonths = 12
    >
    struct MonthlyMovingAverageProjection {
        std::shared_ptr<MonthlyForwardCurve> operator() (
            const HistoricalMonthlyRates& historicalMonthlyRates,
            const MonthlyForwardCurve& baseFwdCurve
        ) const {
            QL_REQUIRE(MovingAverageMonths > 0, "number of moving average months (" << MovingAverageMonths << ") must be positive");
            QL_REQUIRE(historicalMonthlyRates.size() == MovingAverageMonths, "number of historical monthly rates (" << historicalMonthlyRates.size() << ") must be exactly " << MovingAverageMonths);
            QL_REQUIRE(!baseFwdCurve.empty(), "base forward curve is empty");
            MonthlyRates monthlyRatesAll(historicalMonthlyRates);  // historical + porjection. load the historical rates first
            auto n = baseFwdCurve.size();
            // load the forward projection
            for (decltype(n) fwdMonth = 1; fwdMonth < n; ++fwdMonth) {  // n-1 iterations, one for each postitve fwdMonth
                monthlyRatesAll.push_back(baseFwdCurve[fwdMonth]);
            }
            std::shared_ptr<MonthlyForwardCurve> movingAverageCurve(new MonthlyForwardCurve(n)); // the length of the moving average projection is equal to the length of the base rate projection
            for (decltype(n) fwdMonth = 0; fwdMonth < n; ++fwdMonth) {  // n iterations, one for each fwdMonth including 0 (spot)
                double movingAverage = 0.0;
                movingAverage = std::accumulate(monthlyRatesAll.begin() + fwdMonth, monthlyRatesAll.begin() + fwdMonth + MovingAverageMonths, movingAverage);
                movingAverage /= (double)MovingAverageMonths;
                movingAverageCurve->at(fwdMonth) = movingAverage;
            }
            return movingAverageCurve;
        }
    };
}