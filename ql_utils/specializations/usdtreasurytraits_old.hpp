#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/governmentbondtraits.hpp>

namespace QLUtils {
    // US treasury security traits specialization
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
    // US treasury bill traits specialization
    template<>
    inline QuantLib::DayCounter GovernmentBillTraits<QuantLib::USDCurrency>::yieldCalcDayCounter(const QuantLib::Period&) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
    }
    template<>
    inline QuantLib::Frequency GovernmentBillTraits<QuantLib::USDCurrency>::bondEquivCouponFrequency(const QuantLib::Period&) const {
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
	// US treasury note/bond traits specialization
    template<>
    inline QuantLib::Frequency GovernmentBondTraits<QuantLib::USDCurrency>::couponFrequency(const QuantLib::Period&) const {
        return QuantLib::Semiannual;
    }
    template<>
    inline QuantLib::DayCounter GovernmentBondTraits<QuantLib::USDCurrency>::accrualDayCounter(const QuantLib::Period&, QuantLib::Schedule schedule) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::Bond, schedule);
    }
    template<>
    inline QuantLib::DayCounter GovernmentBondTraits<QuantLib::USDCurrency>::yieldCalcDayCounter(const QuantLib::Period&, QuantLib::Schedule schedule) const {
        return QuantLib::ActualActual(QuantLib::ActualActual::Bond, schedule);
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
        return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
    }
}