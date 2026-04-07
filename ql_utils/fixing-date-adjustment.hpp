#pragma once

#include <ql/quantlib.hpp>
#include <utility>

namespace QuantLib {
    namespace Utils {
        class FixingDateAdjustment {
        private:
            Natural fixingDays_;
            Calendar fixingCalendar_;
        public:
            typedef Date FixingDate;
            typedef Date ValueDate;
        public:
            FixingDateAdjustment(
                Natural fixingDays,
                const Calendar& fixingCalendar
            ): fixingDays_(fixingDays), fixingCalendar_(fixingCalendar)
            {}
            static BusinessDayConvention adjConvention (
                Natural fixingDays
            ) {
                return (fixingDays == 0 ? Following : Preceding);
            }
            Natural fixingDays() const {
                return fixingDays_;
            }
            const Calendar& fixingCalendar() const {
                return fixingCalendar_;
            }
            // adjust a candidate fixing date to a correct fixing date base on the number of days of fixing
            Date adjust(
                const Date& d	// candidate fixing date
            ) const {
                QL_REQUIRE(d != Date(), "date cannot be null");
                auto convention = adjConvention(fixingDays_);
                return fixingCalendar_.adjust(d, convention);
            }
            std::pair<FixingDate, ValueDate> calculate(
                const Date& d	// candidate fixing date
            ) const {
                auto fixingDate = adjust(d);
                auto valueDate = fixingCalendar_.advance(fixingDate, fixingDays_ * Days, Following, false);
                QL_ASSERT(valueDate >= d, "value date (" << valueDate << ") is less than the base reference date (" << d << ")");
                return std::pair<FixingDate, ValueDate>(fixingDate, valueDate);
            }
        };
    }
}
