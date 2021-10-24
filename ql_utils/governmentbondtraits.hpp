#pragma once

#include <ql/quantlib.hpp>

namespace QLUtils {
    template <typename QL_CURRENCY>
    struct GovernmentSecurityTraits {
        QuantLib::Calendar settlementCalendar(const QuantLib::Period& tenor) const {}
        QuantLib::Natural settlementDays(const QuantLib::Period& tenor) const {}
        QuantLib::Real parNotional(const QuantLib::Period& tenor) const {}
    };

    template <typename QL_CURRENCY>
    struct GovernmentBillTraits {
        typedef GovernmentSecurityTraits<QL_CURRENCY> SecurityTraits;
        QuantLib::DayCounter dayCounter(const QuantLib::Period& tenor) const {}
        QuantLib::Frequency bondEquivCouponFrequency(const QuantLib::Period& tenor) const {}
        QuantLib::DayCounter discountRateDayCounter(const QuantLib::Period& tenor) const {}
        QuantLib::DayCounter parYieldSplineDayCounter(const QuantLib::Period& tenor) const {}
    };

    template <typename QL_CURRENCY>
    struct GovernmentBondTraits {
        typedef GovernmentSecurityTraits<QL_CURRENCY> SecurityTraits;
        QuantLib::Frequency couponFrequency(const QuantLib::Period& tenor) const {}
        QuantLib::DayCounter accruedDayCounter(const QuantLib::Period& tenor) const {}
        bool endOfMonth(const QuantLib::Period& tenor) const {}
        QuantLib::Calendar scheduleCalendar(const QuantLib::Period& tenor) const {}
        QuantLib::BusinessDayConvention convention(const QuantLib::Period& tenor) const {}
        QuantLib::BusinessDayConvention terminationDateConvention(const QuantLib::Period& tenor) const {}
        QuantLib::DayCounter parYieldSplineDayCounter(const QuantLib::Period& tenor) const {}
    };
}