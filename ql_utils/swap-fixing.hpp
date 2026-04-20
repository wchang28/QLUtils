#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/fixing-date-adjustment.hpp>
#include <utility>

namespace QuantLib {
    // helper class that helps determining the start/effective/fixing date of a forward starting/spot swap
    class SwapFixing {
    public:
        enum FixingMethod {
            sfmForwardSwap = 0,		// settling a forward swap, advance on swap settlement days first then advance on forward
            sfmSwaption = 1,		// for swaption related calculation, advance on forward first then advance on swap settlement days
        };
		typedef Date FixingDate;
		typedef Date StartDate;
    protected:
        Calendar fixingCalendar_;
        Natural settlementDays_;
        FixingMethod fixingMethod_;
    public:
        SwapFixing(
            const Calendar& fixingCalendar,	// fixing calendar of the swap
            Natural settlementDays = 0,
            FixingMethod fixingMethod = FixingMethod::sfmForwardSwap
        ) :
            fixingCalendar_(fixingCalendar),
            settlementDays_(settlementDays),
            fixingMethod_(fixingMethod)
        {}
        const Calendar& fixingCalendar() const {
            return fixingCalendar_;
        }
        Natural settlementDays() const {
            return settlementDays_;
        }
        FixingMethod fixingMethod() const {
            return fixingMethod_;
        }
        // how to apply business day adjustment when advances forward in time
        // for forward swap settling, this is usually BusinessDayConvention::Following
        // for swaption, this will be the option excercise convention which is usually based on the index's business day convention (usually DayConvention::ModifiedFollowing)
        // this method is overridable by the derived class to customize the behavior
        virtual BusinessDayConvention forwardBusinessDayAdj() const {
            switch (fixingMethod_) {
            case FixingMethod::sfmForwardSwap:
                return BusinessDayConvention::Following;
            case FixingMethod::sfmSwaption:
            default:
                return BusinessDayConvention::ModifiedFollowing;
            }
        }
        static Date baseReferenceDate(
            const Calendar& fixingCalendar,	// swap fixing calendar
            Natural settlementDays,
            const Date& today = Date()
        ) {
            auto refDate = (today == Date() ? (Date)Settings::instance().evaluationDate() : today);	// reference date/today
			Utils::FixingDateAdjustment fixingAdj(settlementDays, fixingCalendar);	// make sure the base reference day is a business day on the fixing calendar by adjusting it with swap settlement days
            refDate = fixingAdj.adjust(refDate);
            return refDate;
        }
        Date baseReferenceDate(
            const Date& today = Date()
        ) const {
            return baseReferenceDate(fixingCalendar_, settlementDays_, today);
        }
        // returns fixing date for a spot swap, given a swap fixing calendar and a "today" as a base reference
        static Date spotSwapFixingDate(
            const Calendar& fixingCalendar,	// swap fixing calendar
            Natural settlementDays,
            const Date& today = Date()
        ) {
            return baseReferenceDate(fixingCalendar, settlementDays, today);
        }
        // returns fixing date for a spot swap, given a "today" as a base reference
        Date spotSwapFixingDate(
            const Date& today = Date()
        ) const {
            return baseReferenceDate(fixingCalendar_, settlementDays_, today);
        }
        // returns the start date/effective date of a forward starting or spot swap
        std::pair<FixingDate, StartDate> calculate(
            const Period& forwardStart = 0 * Days,
            const Date& today = Date()
        ) const {
            QL_REQUIRE(forwardStart.length() >= 0, "foward start must be either spot or in the future");
            auto refDate = baseReferenceDate(today);	// start with a base reference date
            if (fixingMethod_ == FixingMethod::sfmForwardSwap) {
                auto spotDate = fixingCalendar_.advance(refDate, settlementDays_ * Days);	// advance swap settlement days to get the spot date. spot date is the spot starting swap's effective date
                auto effectiveDate = fixingCalendar_.advance(spotDate, forwardStart, forwardBusinessDayAdj());	// advance on forward time period to get the swap start date
                auto fixingDate = fixingCalendar_.advance(effectiveDate, -static_cast<Integer>(settlementDays_), Days);
                return std::make_pair(fixingDate, effectiveDate);
            }
            else if (fixingMethod_ == FixingMethod::sfmSwaption) {
                auto fixingDate = fixingCalendar_.advance(refDate, forwardStart, forwardBusinessDayAdj());	// advance on forward time period to get the swap fixing date/option excercise date
                auto effectiveDate = fixingCalendar_.advance(fixingDate, settlementDays_ * Days);	// advance swap settlement days to get the swap start date
                return std::make_pair(fixingDate, effectiveDate);
            }
            else {
                QL_FAIL("unsupported swap fixing method (" << fixingMethod_ << ")");
            }
        }
        // returns the start date/effective date of a forward starting or spot swap
        Date startDate(
            const Period& forwardStart = 0 * Days,
            const Date& today = Date()
        ) const {
            auto [_, effectiveDate] = calculate(forwardStart, today);
            return effectiveDate;
        }
        // returns the fixing date of a forward starting or spot swap
        Date fixingDate(
            const Period& forwardStart = 0 * Days,
            const Date& today = Date()
        ) const {
            auto [fixingDate, _] = calculate(forwardStart, today);
            return fixingDate;
        }
    };
}