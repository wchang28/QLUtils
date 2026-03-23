#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/government-bond-traits.hpp>

namespace QuantLib {
    namespace Utils {
        // US treasury note/bond traits specialization
        ////////////////////////////////////////////////////////////////////////////////////////////
        template<>
        inline Calendar GovtBondTraits<USDCurrency>::settlementCalendar(const Period&) const {
            return UnitedStates(UnitedStates::GovernmentBond);
        }
        template<>
        inline Natural GovtBondTraits<USDCurrency>::settlementDays(const Period&) const {
            return 1;
        }
        template<>
        inline Real GovtBondTraits<USDCurrency>::parNotional(const Period&) const {
            return 100.0;
        }
        template<>
        inline Frequency GovtBondTraits<USDCurrency>::couponFrequency(const Period&) const {
            return Frequency::Semiannual;
        }
        template<>
        inline Calendar GovtBondTraits<USDCurrency>::accrualScheduleCalendar(const Period&) const {
            return UnitedStates(UnitedStates::GovernmentBond);
        }
        template<>
        inline BusinessDayConvention GovtBondTraits<USDCurrency>::accrualConvention(const Period&) const {
            return BusinessDayConvention::Unadjusted;
        }
        template<>
        inline bool GovtBondTraits<USDCurrency>::accrualEndOfMonth(const Period&) const {
            return true;
        }
        template<>
        inline DayCounter GovtBondTraits<USDCurrency>::accrualDayCounter(const Period&, Schedule accrualSchedule) const {
            return ActualActual(ActualActual::Bond, accrualSchedule);
        }
        template<>
        inline Calendar GovtBondTraits<USDCurrency>::paymentCalendar(const Period&) const {
            return UnitedStates(UnitedStates::GovernmentBond);
        }
        template<>
        inline BusinessDayConvention GovtBondTraits<USDCurrency>::paymentConvention(const Period&) const {
            return BusinessDayConvention::Following;
        }
        template<>
        inline DayCounter GovtBondTraits<USDCurrency>::yieldCalcDayCounter(const Period&, Schedule accrualSchedule) const {
            return ActualActual(ActualActual::Bond, accrualSchedule);
        }
        template<>
        inline DayCounter GovtBondTraits<USDCurrency>::parYieldSplineDayCounter(const Period&, Schedule accrualSchedule) const {
            return ActualActual(ActualActual::Bond, accrualSchedule);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////

        // US treasury bill traits specialization
        ////////////////////////////////////////////////////////////////////////////////////////////
        template<>
        inline DayCounter GovtBillTraits<USDCurrency>::marketConventionYieldCalcDayCounter(const Period&) const {
            return ActualActual(ActualActual::ISDA);
        }
        template<>
        inline DayCounter GovtBillTraits<USDCurrency>::discountRateDayCounter(const Period&) const {
            return Actual360();
        }
        ////////////////////////////////////////////////////////////////////////////////////////////
    }
}
