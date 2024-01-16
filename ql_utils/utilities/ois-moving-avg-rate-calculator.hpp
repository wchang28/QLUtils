#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <algorithm>
#include <exception>

namespace QLUtils {
	template <
		template <typename>
		class SHARED_PTR = QuantLib::ext::shared_ptr
	>
	class OISMovingAvgRateCalculator {
	public:
		class AccruedPeriodBase {
		public:
			QuantLib::DayCounter dc;	// day counter for compounding factor/rate calculation
			QuantLib::Date startDate;	// accrued start date
			QuantLib::Date endDate;		// accrued end date
			AccruedPeriodBase(
				const QuantLib::DayCounter& dc,
				const QuantLib::Date& startDate = QuantLib::Date(),
				const QuantLib::Date& endDate = QuantLib::Date()
			) :
				dc(dc),
				startDate(startDate),
				endDate(endDate)
			{}
			QuantLib::Date::serial_type daysActual() const {
				return dc.dayCount(startDate, endDate);
			}
			QuantLib::Time yearFraction() const {
				return dc.yearFraction(startDate, endDate);
			}
		private:
			static QuantLib::Compounding compounding() {
				return QuantLib::Simple;
			}
			static QuantLib::Frequency frequency() {
				return QuantLib::NoFrequency;
			}
		protected:
			QuantLib::Rate impliedRateImpl
			(
				QuantLib::Real compoundFactor
			) const {
				auto ir = QuantLib::InterestRate::impliedRate(compoundFactor, dc, compounding(), frequency(), startDate, endDate);
				return ir.rate();
			}
			QuantLib::Real impliedCompoundFactorImpl
			(
				QuantLib::Rate rate
			) const {
				QuantLib::InterestRate ir(rate, dc, compounding(), frequency());
				return ir.compoundFactor(startDate, endDate);
			}
		};
		struct AccruedPeriod : public AccruedPeriodBase {
			QuantLib::Date fixingDate;	// fixing date
			QuantLib::Rate rate;		// fixing rate
			bool rateIsForecasted;		// is the fixing rate forecasted or historical/past
			AccruedPeriod(
				const QuantLib::DayCounter& dc,
				const QuantLib::Date& startDate = QuantLib::Date(),
				const QuantLib::Date& endDate = QuantLib::Date(),
				const QuantLib::Date& fixingDate = QuantLib::Date(),
				QuantLib::Rate rate = 0.,
				bool rateIsForecasted = false
			) :
				AccruedPeriodBase(dc, startDate, endDate),
				fixingDate(fixingDate),
				rate(rate),
				rateIsForecasted(rateIsForecasted)
			{}
			QuantLib::Real compoundFactor() const {
				return this->impliedCompoundFactorImpl(rate);
			}
		};
		struct AggrAccruedPeriod : public AccruedPeriodBase {
			QuantLib::Real compoundFactor;
			AggrAccruedPeriod(
				const QuantLib::DayCounter& dc,
				const QuantLib::Date& startDate = QuantLib::Date(),
				const QuantLib::Date& endDate = QuantLib::Date(),
				QuantLib::Real compoundFactor = 1.
			) :
				AccruedPeriodBase(dc, startDate, endDate),
				compoundFactor(compoundFactor)
			{}
			QuantLib::Rate impliedRate() const {
				return this->impliedRateImpl(compoundFactor);
			}
		};
		typedef SHARED_PTR<QuantLib::OvernightIndex> pOvernightIndex;
		typedef std::vector<AccruedPeriod> AccruedTable;
		typedef SHARED_PTR<AccruedTable> pAccruedTable;
		typedef SHARED_PTR<AggrAccruedPeriod> pAggrAccruedPeriod;
	public:
		// input
		pOvernightIndex overnightIndex;
		// output
		pAccruedTable accruedTable;
		pAggrAccruedPeriod aggrAccruedPeriod;
	public:
		OISMovingAvgRateCalculator() {}
		template <
			typename PAST_FIXING
		>
		QuantLib::Rate calcuate(
			const PAST_FIXING& pastFixing,
			const QuantLib::Date& valueDate = QuantLib::Date(),
			QuantLib::Natural numMovingAvgDays = 90
		) {
			auto valueDt = valueDate;
			if (valueDt == QuantLib::Date()) {
				valueDt = QuantLib::Settings::instance().evaluationDate();
			}
			QL_REQUIRE(numMovingAvgDays > 0, "number of moving avg. days (" << numMovingAvgDays << ") must be positive");
			QL_REQUIRE(overnightIndex != nullptr, "overnight index is null");
			accruedTable = nullptr;
			aggrAccruedPeriod = nullptr;
			auto endDate = valueDt;
			auto startDate = endDate - numMovingAvgDays;
			auto fixingCalendar = overnightIndex->fixingCalendar();
			const auto& indexDayCounter = overnightIndex->dayCounter();
			auto accruedStartDate = startDate;
			accruedTable.reset(new AccruedTable());
			while (true) {
				auto fixingDate = fixingCalendar.adjust(accruedStartDate, QuantLib::Preceding);
				auto nextFixingDate = fixingCalendar.advance(fixingDate, 1 * QuantLib::Days, QuantLib::Following, false);
				auto accruedEndDate = std::min(endDate, nextFixingDate);
				QuantLib::Rate rate = QuantLib::Null<QuantLib::Rate>();
				bool rateIsForecasted = false;
				try {
					rate = overnightIndex->fixing(fixingDate, true);	// DO forecast today's fixing
					rateIsForecasted = true;
				}
				catch (...) {
					try {
						rate = pastFixing(fixingDate);
						rateIsForecasted = false;
					}
					catch (const std::exception& e) {
						QL_FAIL("unable to get rate for fixing date (" << fixingDate << "): " << e.what());
					}
				}
				AccruedPeriod accruedPeriod(
					indexDayCounter,
					accruedStartDate,
					accruedEndDate,
					fixingDate,
					rate,
					rateIsForecasted
				);
				accruedTable->push_back(accruedPeriod);
				if (accruedEndDate == endDate) {
					break;
				}
				else {	// accruedEndDate < endDate
					// set the next period's accrued start date to be this period's accrued end date
					accruedStartDate = accruedEndDate;
				}
			}
			QuantLib::Real compoundFactor = 1.;
			decltype(numMovingAvgDays) totalDays = 0;
			for (const auto& period : *accruedTable) {
				totalDays += period.daysActual();
				compoundFactor *= period.compoundFactor();
			}
			QL_ASSERT(totalDays == numMovingAvgDays, "totalDays (" << totalDays << ") != numMovingAvgDays (" << numMovingAvgDays << ")");
			aggrAccruedPeriod.reset(new AggrAccruedPeriod(indexDayCounter, startDate, endDate, compoundFactor));
			auto calculatedAvg = aggrAccruedPeriod->impliedRate();
			return calculatedAvg;
		}
	};
}