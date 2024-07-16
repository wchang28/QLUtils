#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
	// forward rate helper with no calendar adjustment and no settlement days
	class NominalForwardRateHelper : public RateHelper {
	protected:
		Date baseReferenceDate_;	// base reference date for the forward period
		Period forward_;
		Period tenor_;
		DayCounter dayCounter_;
		Compounding compounding_;
		Frequency frequency_;
	public:
		Date baseReferenceDate() const {
			return (baseReferenceDate_ == Date() ? (Date)Settings::instance().evaluationDate() : baseReferenceDate_);
		}
		Period forward() const {
			return forward_;
		}
		Period tenor() const {
			return tenor_;
		}
	protected:
		void initializeDates() {
			earliestDate_ = baseReferenceDate() + forward();
			maturityDate_ = earliestDate_ + tenor();
			pillarDate_ = latestDate_ = latestRelevantDate_ = maturityDate_;
		}
	public:
		NominalForwardRateHelper(
			const Handle<Quote>& rate,
			const Period& forward,
			const Date& baseReferenceDate = Date(),
			const Period& tenor = Period(1, Months),
			const DayCounter& dayCounter = Thirty360(Thirty360::BondBasis),
			Compounding compounding = Compounding::Continuous,
			Frequency frequency = Frequency::NoFrequency
		) :
			RateHelper(rate),
			forward_(forward),
			tenor_(tenor),
			baseReferenceDate_(baseReferenceDate),
			dayCounter_(dayCounter),
			compounding_(compounding),
			frequency_(frequency)
		{
			initializeDates();
		}
		NominalForwardRateHelper(
			Rate rate,
			const Period& forward,
			const Date& baseReferenceDate = Date(),
			const Period& tenor = Period(1, Months),
			const DayCounter& dayCounter = Thirty360(Thirty360::BondBasis),
			Compounding compounding = Compounding::Continuous,
			Frequency frequency = Frequency::NoFrequency
		) :
			NominalForwardRateHelper(makeQuoteHandle(rate), forward, baseReferenceDate, tenor, dayCounter, compounding, frequency)
		{}
		Date startDate() const {
			return earliestDate_;
		}
		const DayCounter& dayCounter() const {
			return dayCounter_;
		}
		Compounding compounding() const {
			return compounding_;
		}
		Frequency frequency() const {
			return frequency_;
		}
		static Rate impliedRate(
			const YieldTermStructure& termStructure,
			const Date& startDate,
			const Date& endDate,
			const DayCounter& dayCounter = Thirty360(Thirty360::BondBasis),
			Compounding compounding = Compounding::Continuous,
			Frequency frequency = Frequency::NoFrequency
		) {
			auto d1 = termStructure.discount(startDate);
			auto d2 = termStructure.discount(endDate);
			auto compound = d1 / d2;
			auto ir = InterestRate::impliedRate(compound, dayCounter, compounding, frequency, startDate, endDate);
			return ir.rate();
		}
		static Rate impliedRate(
			const YieldTermStructure& termStructure,
			const Period& forward,
			const Period& tenor = Period(1, Months),
			const DayCounter& dayCounter = Thirty360(Thirty360::BondBasis),
			Compounding compounding = Compounding::Continuous,
			Frequency frequency = Frequency::NoFrequency
		) {
			auto startDate = termStructure.referenceDate() + forward;
			auto endDate = startDate + tenor;
			return impliedRate(termStructure, startDate, endDate, dayCounter, compounding, frequency);
		}
		Real impliedQuote() const {
			QL_REQUIRE(termStructure_ != nullptr, "term structure is null");
			return impliedRate(*termStructure_, startDate(), maturityDate(), dayCounter(), compounding(), frequency());
		}
		void accept(AcyclicVisitor& v) {
			auto* v1 = dynamic_cast<Visitor<NominalForwardRateHelper>*>(&v);
			if (v1 != nullptr)
				v1->visit(*this);
			else
				RateHelper::accept(v);
		}
	};
}