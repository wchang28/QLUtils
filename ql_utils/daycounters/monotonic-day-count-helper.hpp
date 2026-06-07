#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

namespace QuantLib {
    namespace Utils {
        struct MonotonicDayCountHelper {
            static DayCounter to_daycounter(
                MonotonicDayCountConv dayCountConv
            ) {
                switch (dayCountConv) {
                case MonotonicDayCountConv::mdccActual365Fixed:
                    return Actual365Fixed{};
                case MonotonicDayCountConv::mdccActual360:
                    return Actual360{};
                case MonotonicDayCountConv::mdccActual36525:
                    return Actual36525{};
                case MonotonicDayCountConv::mdccActual364:
                    return Actual364{};
                case MonotonicDayCountConv::mdccActual366:
                    return Actual366{};
                case MonotonicDayCountConv::mdccActualActualISDA:
                    return ActualActual{ActualActual::ISDA};
                default:
                    QL_FAIL("unsupported/unknown monotonic day count convention: " << dayCountConv);
                }
            }
            static MonotonicDayCountConv from_daycounter(
                const DayCounter& dc
            ) {
                if (dc == Actual365Fixed{}) {
                    return MonotonicDayCountConv::mdccActual365Fixed;
                }
                else if (dc == Actual360{}) {
                    return MonotonicDayCountConv::mdccActual360;
                }
                else if (dc == Actual36525{}) {
                    return MonotonicDayCountConv::mdccActual36525;
                }
                else if (dc == Actual364{}) {
                    return MonotonicDayCountConv::mdccActual364;
                }
                else if (dc == Actual366{}) {
                    return MonotonicDayCountConv::mdccActual366;
                }
                else if (dc == ActualActual{ActualActual::ISDA}) {
                    return MonotonicDayCountConv::mdccActualActualISDA;
                }
                else {
                    QL_FAIL(dc.name() << " is not a supported monotonic day count convention");
                }
            }
            static Real days_per_year(
                MonotonicDayCountConv dayCountConv
            ) {
                switch (dayCountConv) {
                case MonotonicDayCountConv::mdccActual365Fixed:
                    return 365.0;
                case MonotonicDayCountConv::mdccActual360:
                    return 360.0;
                case MonotonicDayCountConv::mdccActual36525:
                    return 365.25;
                case MonotonicDayCountConv::mdccActual364:
                    return 364.0;
                case MonotonicDayCountConv::mdccActual366:
                    return 366.0;
                case MonotonicDayCountConv::mdccActualActualISDA:
                    return Null<Real>();
                default:
                    QL_FAIL("unsupported/unknown monotonic day count convention: " << dayCountConv);
                }
            }
            static Date dateFromYearFraction_ActualActualISDA(
                const Date& baseDate,
                Time yearFraction
            ) {
                auto days = (Date::serial_type)std::round(yearFraction * 365.25);
                Date d = baseDate + days;
                const Date::serial_type daysWindow = 2;
                Date d1 = d - daysWindow;
                Date d2 = d + daysWindow;
                std::vector<std::pair<Date, Real>> v;
                DayCounter dc = ActualActual(ActualActual::ISDA);
                d = d1;
                while(d <= d2) {
                    auto yf = dc.yearFraction(baseDate, d);
                    v.push_back(std::pair<Date, Real>{d, std::abs(yf - yearFraction)});
                    d++;
                }
                std::sort(
                    v.begin(),
                    v.end(),
                    [](const std::pair<Date, Real>& a, const std::pair<Date, Real>& b) {
                        return a.second < b.second;
                    }
                );
                const auto& pr = v.front();
                d = pr.first;
                return d;
            }
            static Date dateFromYearFraction(
                const Date& baseDate,
                MonotonicDayCountConv dayCountConv,
                Time yearFraction
            ) {
                Real dpy = days_per_year(dayCountConv);
                if (dpy != Null<Real>()) {
                    auto days = (Date::serial_type)std::round(yearFraction * dpy);
                    return baseDate + days;
                } else if (dayCountConv == MonotonicDayCountConv::mdccActualActualISDA) {
                    return dateFromYearFraction_ActualActualISDA(baseDate, yearFraction);
                } else {
                    QL_FAIL("unsupported/unknown monotonic day count convention: " << dayCountConv);
                }
            }
        };
    }
}
