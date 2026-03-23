#pragma once

#include <stdexcept>
#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        // traits class for a government coupon paying bond of a particular currency
        template <
            typename QL_CURRENCY
        >
        struct GovtBondTraits {
            // bond's settlement calendar
            Calendar settlementCalendar(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // number of days to settle the bond (T+n)
            Natural settlementDays(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // bond's notional/face amount, usually 100.
            Real parNotional(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // how often the bond pays its coupon
            Frequency couponFrequency(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // calendar for projecting bond accrual periods
            Calendar accrualScheduleCalendar(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // adjustment method if accrual start/end date fall on the a holiday in the above accrual calendar
            BusinessDayConvention accrualConvention(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // accrual start/end date end-of-month sticky flag
            bool accrualEndOfMonth(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // day counter for bond's coupon accrual period
            DayCounter accrualDayCounter(const Period& tenor, Schedule accrualSchedule = {}) const {
                throw std::logic_error("not implemented");
            }
            Calendar paymentCalendar(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            BusinessDayConvention paymentConvention(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // day counter used when calculating yield to maturity (YTM)
            DayCounter yieldCalcDayCounter(const Period& tenor, Schedule accrualSchedule = {}) const {
                throw std::logic_error("not implemented");
            }
            DayCounter parYieldSplineDayCounter(const Period& tenor, Schedule accrualSchedule = {}) const {
                throw std::logic_error("not implemented");
            }
        };

        // a government bond which pays no/zero coupon with maturity <= 1 yr.
        // this traits class makes the bond/bill looks like a cash deposit
        template <
            typename QL_CURRENCY
        >
        struct GovtBillTraits {
            typedef GovtBondTraits<QL_CURRENCY> BondTraits;
            // day counter for calculating market convention yield which is different from YTM calc day counter for the bond
            DayCounter marketConventionYieldCalcDayCounter(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
            // day counter for the discount rate
            DayCounter discountRateDayCounter(const Period& tenor) const {
                throw std::logic_error("not implemented");
            }
        };
    }
}
