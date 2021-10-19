#pragma once

#include <ql/quantlib.hpp>
#include <vector>

namespace QLUtils {
    // bond scheduler without knowing bond's issue date
    class BondSechdulerWithoutIssueDate {
    private:
        QuantLib::Natural settlementDays_;
        QuantLib::Calendar settlementCalendar_;
        QuantLib::Frequency frequency_;
        bool endOfMonth_;
        QuantLib::Calendar scheduleCalendar_;
        QuantLib::BusinessDayConvention convention_;
        QuantLib::BusinessDayConvention terminationDateConvention_;
    public:
        BondSechdulerWithoutIssueDate(
            QuantLib::Natural settlementDays,
            const QuantLib::Calendar& settlementCalendar,
            QuantLib::Frequency frequency = QuantLib::Semiannual,
            bool endOfMonth = true,
            const QuantLib::Calendar& scheduleCalendar = QuantLib::NullCalendar(),
            QuantLib::BusinessDayConvention convention = QuantLib::Unadjusted,
            QuantLib::BusinessDayConvention terminationDateConvention = QuantLib::Unadjusted
        ) : settlementDays_(settlementDays),
            settlementCalendar_(settlementCalendar),
            frequency_(frequency),
            endOfMonth_(endOfMonth),
            scheduleCalendar_(scheduleCalendar),
            convention_(convention),
            terminationDateConvention_(terminationDateConvention)
        {}
        const QuantLib::Natural& settlementDays() const {
            return settlementDays_;
        }
        QuantLib::Natural& settlementDays() {
            return settlementDays_;
        }
        const QuantLib::Calendar& settlementCalendar() const {
            return settlementCalendar_;
        }
        QuantLib::Calendar& settlementCalendar() {
            return settlementCalendar_;
        }
        const QuantLib::Frequency& frequency() const {
            return frequency_;
        };
        QuantLib::Frequency& frequency() {
            return frequency_;
        };
        const bool& endOfMonth() const {
            return endOfMonth_;
        };
        bool& endOfMonth() {
            return endOfMonth_;
        };
        const QuantLib::Calendar& scheduleCalendar() const {
            return scheduleCalendar_;
        }
        QuantLib::Calendar& scheduleCalendar() {
            return scheduleCalendar_;
        }
        const QuantLib::BusinessDayConvention& convention() const {
            return convention_;
        };
        QuantLib::BusinessDayConvention& convention() {
            return convention_;
        };
        const QuantLib::BusinessDayConvention& terminationDateConvention() const {
            return terminationDateConvention_;
        }
        QuantLib::BusinessDayConvention& terminationDateConvention() {
            return terminationDateConvention_;
        }
        QuantLib::Date settlementDate() const {
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            auto dt = settlementCalendar().adjust(today);
            return settlementCalendar().advance(dt, settlementDays() * QuantLib::Days);
        }
        QuantLib::Schedule operator() (const QuantLib::Date& maturityDate) const {
            auto settlementDt = settlementDate();
            QuantLib::Period tenor(frequency());
            auto tenorPlus1Month = tenor + 1 * QuantLib::Months;
            auto start = settlementDt - tenorPlus1Month;
            QuantLib::Schedule schedule(start, maturityDate, tenor, scheduleCalendar(), convention(), terminationDateConvention(), QuantLib::DateGeneration::Backward, endOfMonth());
            for (auto p = schedule.begin(); p != schedule.end(); ++p) {
                if (settlementDt < *p) {
                    // --p points to the previous coupon date
                    std::vector<QuantLib::Date> dates(--p, schedule.end());
                    // settlementCalendar and convention are just meta information for the client so the client can use it to calculate bond's settlement date (given the number of settlement days)
                    return QuantLib::Schedule(dates, settlementCalendar(), convention());
                }
            }
            QL_FAIL("bad scheduling logic");
        }
    };
}