#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/governmentbondtraits.hpp>

namespace QLUtils {
    template<>
    inline QuantLib::Calendar GovernmentSecurityTraits<QuantLib::USDCurrency>::settlementCalendar(const QuantLib::Period&) const {
        return QuantLib::UnitedStates(QuantLib::UnitedStates::GovernmentBond);
    }
    template<>
    inline QuantLib::Natural GovernmentSecurityTraits<QuantLib::USDCurrency>::settlementDays(const QuantLib::Period&) const {
        return 1;
    }
    template<>
    inline QuantLib::Real GovernmentSecurityTraits<QuantLib::USDCurrency>::parNotional(const QuantLib::Period&) const {
        return 100.0;
    }
    template<>
    inline QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::dayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
    }
    template<>
    inline QuantLib::Frequency GovernmentBillTraits<QuantLib::USDCurrency>::referenceCouponFrequency(const QuantLib::Period&) const {
        return QuantLib::Semiannual;
    }
    template<>
    inline QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::discountRateDayCounter(const QuantLib::Period&) const {
        return QuantLib::Actual360();
    }
    template<>
    inline QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::parYieldSplineDayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
    }
    template<>
    inline QuantLib::Frequency GovernmentBondTraits<QuantLib::USDCurrency>::couponFrequency(const QuantLib::Period&) const {
        return QuantLib::Semiannual;
    }
    template<>
    inline QuantLib::DayCounter GovernmentBondTraits<QuantLib::USDCurrency>::accruedDayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::Bond);
    }
    template<>
    inline bool GovernmentBondTraits<QuantLib::USDCurrency>::endOfMonth(const QuantLib::Period&) const {
        return true;
    }
    template<>
    inline QuantLib::Calendar GovernmentBondTraits<QuantLib::USDCurrency>::scheduleCalendar(const QuantLib::Period&) const {
        return QuantLib::NullCalendar();
    }
    template<>
    inline QuantLib::BusinessDayConvention GovernmentBondTraits<QuantLib::USDCurrency>::convention(const QuantLib::Period&) const {
        return QuantLib::Unadjusted;
    }
    template<>
    inline QuantLib::BusinessDayConvention GovernmentBondTraits<QuantLib::USDCurrency>::terminationDateConvention(const QuantLib::Period&) const {
        return QuantLib::Unadjusted;
    }
    template<>
    inline QuantLib::DayCounter GovernmentBondTraits<QuantLib::USDCurrency>::parYieldSplineDayCounter(const QuantLib::Period&) const {
        return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
    }
}