#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/governmentbondtraits.hpp>

namespace QLUtils {
    template<>
    QuantLib::Calendar GovernmentSecurityTraits<QuantLib::USDCurrency>::settlementCalendar(const QuantLib::Period&) const {
        return QuantLib::UnitedStates(QuantLib::UnitedStates::GovernmentBond);
    }
    template<>
    QuantLib::Natural GovernmentSecurityTraits<QuantLib::USDCurrency>::settlementDays(const QuantLib::Period&) const {
        return 1;
    }
    template<>
    QuantLib::Real GovernmentSecurityTraits<QuantLib::USDCurrency>::parNotional(const QuantLib::Period&) const {
        return 100.0;
    }
    template<>
    QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::dayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
    }
    template<>
    QuantLib::Frequency GovernmentBillTraits<QuantLib::USDCurrency>::referenceCouponFrequency(const QuantLib::Period&) const {
        return QuantLib::Semiannual;
    }
    template<>
    QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::discountRateDayCounter(const QuantLib::Period&) const {
        return QuantLib::Actual360();
    }
    template<>
    QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::parYieldSplineDayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
    }
    template<>
    QuantLib::Frequency GovernmentBondTraits<QuantLib::USDCurrency>::couponFrequency(const QuantLib::Period&) const {
        return QuantLib::Semiannual;
    }
    template<>
    QuantLib::DayCounter GovernmentBondTraits<QuantLib::USDCurrency>::accruedDayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::Bond);
    }
    template<>
    bool GovernmentBondTraits<QuantLib::USDCurrency>::endOfMonth(const QuantLib::Period&) const {
        return true;
    }
    template<>
    QuantLib::Calendar GovernmentBondTraits<QuantLib::USDCurrency>::scheduleCalendar(const QuantLib::Period&) const {
        return QuantLib::NullCalendar();
    }
    template<>
    QuantLib::BusinessDayConvention GovernmentBondTraits<QuantLib::USDCurrency>::convention(const QuantLib::Period&) const {
        return QuantLib::Unadjusted;
    }
    template<>
    QuantLib::BusinessDayConvention GovernmentBondTraits<QuantLib::USDCurrency>::terminationDateConvention(const QuantLib::Period&) const {
        return QuantLib::Unadjusted;
    }
    template<>
    QuantLib::DayCounter GovernmentBondTraits<QuantLib::USDCurrency>::parYieldSplineDayCounter(const QuantLib::Period&) const {
        return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
    }
}