#pragma once

#include <ql/quantlib.hpp>
#include <vector>

namespace QLUtils {
    // bond scheduler without knowing bond's issue date
	// only bond's maturity date and settlement date are needed to make the schedule
    class BondSechdulerWithoutIssueDate {
    public:
        enum AnchorAtType {
            anchorAtMaturityDate = 0,
            anchorAtSettlementDate = 1,
        };
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
			QuantLib::Frequency frequency = QuantLib::Semiannual,   // bond coupon frequency, default to semiannual
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
        QuantLib::Date settlementDate(
			QuantLib::Date today = QuantLib::Date()
        ) const {
            if (today == QuantLib::Date()) {
                today = QuantLib::Settings::instance().evaluationDate();
            }
            auto dt = settlementCalendar().adjust(today);
            return settlementCalendar().advance(dt, settlementDays() * QuantLib::Days);
        }
    private:
        QuantLib::Schedule makeBackwardScheduleImpl(
            const QuantLib::Date& settlementDt,
            const QuantLib::Date& maturityDate
        ) const {
            QuantLib::Period tenor(frequency());    // tenor of the coupon period
            auto tenorPlus1Month = tenor + 1 * QuantLib::Months;
            auto start = settlementDt - tenorPlus1Month;
            QuantLib::DateGeneration::Rule rule = QuantLib::DateGeneration::Rule::Backward;
            QuantLib::Schedule schedule(start, maturityDate, tenor, scheduleCalendar(), convention(), terminationDateConvention(), rule, endOfMonth());
            for (auto p = schedule.begin(); p != schedule.end(); ++p) {
                if (settlementDt < *p) {
                    // --p points to the previous coupon date
                    std::vector<QuantLib::Date> dates(--p, schedule.end());
                    // settlementCalendar, convention, terminationDateConvention, tenor, rule, and endOfMonth are just meta information for the client so the client can use it to calculate bond's settlement date (given the number of settlement days)
                    return QuantLib::Schedule(
                        dates,
                        settlementCalendar(),
                        convention(),
                        terminationDateConvention(),
                        tenor,
                        rule,
                        endOfMonth()
                    );
                }
            }
            QL_FAIL("bad scheduling logic");
        }
        QuantLib::Schedule makeFowrardScheduleImpl(
            const QuantLib::Date& settlementDt,
            const QuantLib::Date& maturityDate
        ) const {
            QuantLib::Period tenor(frequency());    // tenor of the coupon period
            auto endDate = maturityDate + tenor;
            QuantLib::DateGeneration::Rule rule = QuantLib::DateGeneration::Rule::Forward;
            QuantLib::Schedule schedule(settlementDt, endDate, tenor, scheduleCalendar(), convention(), terminationDateConvention(), rule, endOfMonth());
            std::vector<QuantLib::Date> dates;
            for (auto const& d : schedule.dates()) {
                dates.push_back(d);
                if (d >= maturityDate) {
                    break;
                }
            }
            return QuantLib::Schedule(
                dates,
                scheduleCalendar(),
                convention(),
                terminationDateConvention(),
                tenor,
                rule,
                endOfMonth()
            );
        }
    public:
        // schedule maker call operator
        QuantLib::Schedule operator() (
            const QuantLib::Date& maturityDate,
            AnchorAtType anchorAt = AnchorAtType::anchorAtMaturityDate,
            QuantLib::Date today = QuantLib::Date()
        ) const {
            auto settlementDt = settlementDate(today);
            QL_REQUIRE(settlementDt < maturityDate, "Settlement date (" << settlementDt << ") must be before maturity date (" << maturityDate << ")");
            if (anchorAt == AnchorAtType::anchorAtMaturityDate) {
                return makeBackwardScheduleImpl(settlementDt, maturityDate);
            }
            else {
                return makeFowrardScheduleImpl(settlementDt, maturityDate);
            }
        }
        // make the schedule
        QuantLib::Schedule schedule(
            const QuantLib::Date& maturityDate,
            AnchorAtType anchorAt = AnchorAtType::anchorAtMaturityDate,
            QuantLib::Date today = QuantLib::Date()
        ) const {
			return this->operator()(maturityDate, anchorAt, today);
        }
    };
}