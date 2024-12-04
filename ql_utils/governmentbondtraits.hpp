#pragma once
// traits for goverment secutities
// must use template specialization to provide the actual implementation for each currency that support government securities
#include <ql/quantlib.hpp>
#include <stdexcept>

namespace QLUtils {
    template <typename QL_CURRENCY>
    struct GovernmentSecurityTraits {
        QuantLib::Calendar settlementCalendar(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::Natural settlementDays(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::Real parNotional(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
    };

    template <typename QL_CURRENCY>
    struct GovernmentBillTraits {
        typedef GovernmentSecurityTraits<QL_CURRENCY> SecurityTraits;
        QuantLib::DayCounter dayCounter(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::Frequency bondEquivCouponFrequency(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::DayCounter discountRateDayCounter(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::DayCounter parYieldSplineDayCounter(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
    };

    template <typename QL_CURRENCY>
    struct GovernmentBondTraits {
        typedef GovernmentSecurityTraits<QL_CURRENCY> SecurityTraits;
        QuantLib::Frequency couponFrequency(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::DayCounter accruedDayCounter(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        bool endOfMonth(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::Calendar scheduleCalendar(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::BusinessDayConvention convention(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::BusinessDayConvention terminationDateConvention(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
        QuantLib::DayCounter parYieldSplineDayCounter(const QuantLib::Period& tenor) const {
            throw std::logic_error("not implemented");
        }
    };
}