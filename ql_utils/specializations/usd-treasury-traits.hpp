#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/government-bond-traits.hpp>

namespace QuantLib {
    namespace Utils {
        // US treasury note/bond traits specialization
        ////////////////////////////////////////////////////////////////////////////////////////////
        template<>
        inline Calendar GovernmentBondTraits<USDCurrency>::settlementCalendar(const Period&) const {
            return UnitedStates(UnitedStates::GovernmentBond);
        }
        template<>
        inline Natural GovernmentBondTraits<USDCurrency>::settlementDays(const Period&) const {
            return 1;
        }
        template<>
        inline Real GovernmentBondTraits<USDCurrency>::parNotional(const Period&) const {
            return 100.0;
        }
        template<>
        inline Frequency GovernmentBondTraits<USDCurrency>::couponFrequency(const Period&) const {
            return Frequency::Semiannual;
        }
        template<>
        inline Calendar GovernmentBondTraits<USDCurrency>::accrualScheduleCalendar(const Period&) const {
            return UnitedStates(UnitedStates::GovernmentBond);
        }
        template<>
        inline BusinessDayConvention GovernmentBondTraits<USDCurrency>::accrualConvention(const Period&) const {
            return BusinessDayConvention::Unadjusted;
        }
        template<>
        inline bool GovernmentBondTraits<USDCurrency>::accrualEndOfMonth(const Period&) const {
            return true;
        }
        template<>
        inline DayCounter GovernmentBondTraits<USDCurrency>::accrualDayCounter(const Period&, Schedule accrualSchedule) const {
            return ActualActual(ActualActual::Bond, accrualSchedule);
        }
        template<>
        inline Calendar GovernmentBondTraits<USDCurrency>::paymentCalendar(const Period&) const {
            return UnitedStates(UnitedStates::GovernmentBond);
        }
        // Bloomberg uses BusinessDayConvention::Unadjusted (verified) for the calculation so the payment dates
        // and the accrual end dates are aligned. In real life, treadury bond/bill payment date should not fall
        // on a holiday in the payment calendar. In this case, BusinessDayConvention::Following should be used.
        template<>
        inline BusinessDayConvention GovernmentBondTraits<USDCurrency>::paymentConvention(const Period&) const {
            //return BusinessDayConvention::Following;
            return BusinessDayConvention::Unadjusted;
        }
        template<>
        inline DayCounter GovernmentBondTraits<USDCurrency>::yieldCalcDayCounter(const Period&, Schedule schedule) const {
            return ActualActual(ActualActual::Bond, schedule);
        }
        template<>
        inline DayCounter GovernmentBondTraits<USDCurrency>::parYieldSplineDayCounter(const Period&, Schedule schedule) const {
            return ActualActual(ActualActual::Bond, schedule);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////

        // US treasury bill traits specialization
        ////////////////////////////////////////////////////////////////////////////////////////////
        template<>
        inline DayCounter GovernmentBillTraits<USDCurrency>::marketConventionYieldCalcDayCounter(const Period&, Schedule schedule) const {
            return ActualActual(ActualActual::ISDA);
        }
        template<>
        inline DayCounter GovernmentBillTraits<USDCurrency>::discountRateDayCounter(const Period&) const {
            return Actual360();
        }
        ////////////////////////////////////////////////////////////////////////////////////////////
    }
}
