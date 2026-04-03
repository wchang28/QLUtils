#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        class FixingDateAdjustment {
        private:
            Currency currency_;
            Calendar fixingCalendar_;
        public:
            FixingDateAdjustment(
                const Currency& currency,
                const Calendar& fixingCalendar
            ): currency_(currency), fixingCalendar_(fixingCalendar)
            {}
            static BusinessDayConvention adjConvention (
                const Currency& currency
            ) {
                if (currency == USDCurrency())
                    return Preceding;
                else if (currency == EURCurrency())
                    return Preceding;
                else if (currency == GBPCurrency())
                    return Following;
                else
                    return Preceding;
            }
            const Currency& currency() const {
                return currency_;
            }
            const Calendar& fixingCalendar() const {
                return fixingCalendar_;
            }
            Date adjust(
                const Date& d
            ) const {
                auto convention = adjConvention(currency_);
                return fixingCalendar_.adjust(d, convention);
            }
        };
    }
}
