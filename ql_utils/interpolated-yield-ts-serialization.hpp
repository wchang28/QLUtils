#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <ql_utils/types.hpp>
#include <ql_utils/interpolation-traits.hpp>
#include <ql_utils/daycounters/monotonic-day-count-helper.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>

#define INTERP_BASE_CURVE_TYPE(INTERP)  typename InterpTraits<InterpolationType::INTERP>::BaseCurveType
#define INTERP_FORWARD_SPREADED_CURVE_TYPE(INTERP)  typename InterpTraits<InterpolationType::INTERP>::ForwardSpreadedCurveType
#define HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(INTERP, CURVE) { \
        auto base_curve = ext::dynamic_pointer_cast<INTERP_BASE_CURVE_TYPE(INTERP)>(CURVE); \
        if (base_curve != nullptr) {\
            return std::make_pair( \
                InterpolationType::INTERP, \
                getCurveTermStructRows(base_curve->dates(), base_curve->data(), *CURVE, valueUnit) \
            ); \
        }   \
    }
#define HANDLE_INTERP_RETURN_BASE_CURVE(INTERP) case InterpolationType::INTERP: \
        return YieldTermStructurePtr(new INTERP_BASE_CURVE_TYPE(INTERP)(dates_, values_, dc))
#define HANDLE_INTERP_RETURN_FORWARD_SPREADED_CURVE(INTERP) case InterpolationType::INTERP: \
        return YieldTermStructurePtr(new INTERP_FORWARD_SPREADED_CURVE_TYPE(INTERP)(baseCurve, quotes, dates_))

namespace QuantLib {
    namespace Utils {
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedYieldTermStructSerializer {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef VALUE_TYPE value_type;
            struct Row {
                Date date;
                Time term;
                value_type value;
                QLUtils::RateUnit valueUnit;    // hint for the actual serializer on how to serialize the value
                Rate zeroRate;
                Rate forwardRate;
                Rate simpleRate;
                Rate semiannualZeroRate;
                DiscountFactor discountFactor;
                Row(
                    Date date = Date(),
                    Time term = 0.0,
                    value_type value = 0.0,
                    QLUtils::RateUnit valueUnit = QLUtils::RateUnit::Percent,
                    Rate zeroRate = 0.0,
                    Rate forwardRate = 0.0,
                    Rate simpleRate = 0.0,
                    Rate semiannualZeroRate = 0.0,
                    DiscountFactor discountFactor = 1.0
                ) :
                date(date),
                term(term),
                value(value),
                valueUnit(valueUnit),
                zeroRate(zeroRate),
                forwardRate(forwardRate),
                simpleRate(simpleRate),
                semiannualZeroRate(semiannualZeroRate),
                discountFactor(discountFactor)
                {}
            };
            typedef Row Row_Type;
            typedef std::vector<Row_Type> TermStructureRows;
            typedef YieldTermStructureInterpolation InterpolationType;
            template <
                InterpolationType INTERP
            >
            using InterpTraits = YieldTermStructureInterpTraits<INTERP>;
        private:
            Date marketDate_;
            Date referenceDate_;
            MonotonicDayCountConv dayCountConv_;
            InterpolationType interpolation_;
            TermStructureRows termStructure_;
        protected:
            static TermStructureRows getCurveTermStructRows(
                const std::vector<Date>& dates,
                const std::vector<value_type>& data,
                const YieldTermStructure& curve,
                QLUtils::RateUnit valueUnit
            ) {
                QL_ASSERT(dates.size() == data.size(), "dates and data size mismatch");
                Date curveRefDate = curve.referenceDate();
                DayCounter dc = curve.dayCounter();
                Size n = dates.size();
                TermStructureRows rows;
                for (Size i = 0; i < n; ++i) {  // for each row
                    const auto& date = dates[i];
                    const auto& value = data[i];
                    Time term = dc.yearFraction(curveRefDate, date);
                    Rate zeroRate = curve.zeroRate(date, dc, Continuous, NoFrequency, true).rate();
                    Rate forwardRate = curve.forwardRate(date, date, dc, Continuous, NoFrequency, true).rate();
                    Rate simpleRate = curve.zeroRate(date, dc, Simple, NoFrequency, true).rate();
                    Rate semiannualZeroRate = curve.zeroRate(date, dc, Compounded, Semiannual, true).rate();
                    DiscountFactor discountFactor = curve.discount(date);
                    rows.push_back(Row{ date, term, value, valueUnit, zeroRate, forwardRate, simpleRate, semiannualZeroRate, discountFactor });
                }
                return rows;
            }
            static std::pair<InterpolationType, TermStructureRows> from_termstructure(
                const YieldTermStructurePtr& curve
            ) {
                QL_ASSERT(curve != nullptr, "yield term structure cannot be null");
                auto valueUnit = QLUtils::RateUnit::Percent;    // hint for the actual serializer on how to serialize the rate/yield value (in this case, in percentage unit)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(ytsiPiecewiseLinearCont, curve)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(ytsiPiecewiseLinearSimple, curve)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(ytsiStepForwardCont, curve)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(ytsiSmoothForwardCont, curve)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(ytsiPiecewiseLinearForwardCont, curve)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(ytsiLogLinearDiscount, curve)
                QL_FAIL("unsupported yield term structure interpolation type");
            }
        public:
            InterpolatedYieldTermStructSerializer(
                const YieldTermStructurePtr& curve,
                Date marketDate = Date()
            ) :
                marketDate_(marketDate == Date() ? Settings::instance().evaluationDate() : marketDate),
                referenceDate_(curve->referenceDate()),
                dayCountConv_(MonotonicDayCountHelper::from_daycounter(curve->dayCounter())),
                interpolation_(InterpolationType::ytsiPiecewiseLinearCont)
            {
                auto [interp, rows] = from_termstructure(curve);
                interpolation_ = interp;
                termStructure_ = rows;
            }
            const Date& marketDate() const { return marketDate_; }
            Date& marketDate() { return marketDate_; }
            const Date& referenceDate() const { return referenceDate_; }
            MonotonicDayCountConv dayCountConv() const { return dayCountConv_; }
            InterpolationType interpolation() const { return interpolation_; }
            const TermStructureRows& termStructure() const { return termStructure_; }
            std::vector<Date> dates() const {
                std::vector<Date> ret;
                for (const auto& row : termStructure_) {
                    ret.push_back(row.date);
                }
                return ret;
            }
            std::vector<Real> data() const {
                std::vector<Real> ret;
                for (const auto& row : termStructure_) {
                    ret.push_back(row.value);
                }
                return ret;
            }
            std::vector<Time> times() const {
                std::vector<Time> ret;
                for (const auto& row : termStructure_) {
                    ret.push_back(row.term);
                }
                return ret;
            }
            std::vector<std::pair<Date, Real>> nodes() const {
                std::vector<std::pair<Date, Real>> results(termStructure_.size());
                for (Size i=0; i<termStructure_.size(); ++i) {
                    const auto& row = termStructure_[i];
                    results[i] = std::make_pair(row.date, row.value);
                }
                return results;
            }
        };
        
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedYieldTermStructDeserializer {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef VALUE_TYPE value_type;
            struct Row {
                Date date;
                value_type value;
                QLUtils::RateUnit valueUnit;
                Row(
                    Date date = Date(),
                    value_type value = 0.0,
                    QLUtils::RateUnit valueUnit = QLUtils::RateUnit::Percent
                ):
                date(date),
                value(value),
                valueUnit(valueUnit)
                {}
            };
            typedef Row Row_Type;
            typedef std::vector<Row_Type> TermStructureRows;
            typedef YieldTermStructureInterpolation InterpolationType;
            template <
                InterpolationType INTERP
            >
            using InterpTraits = YieldTermStructureInterpTraits<INTERP>;
        private:
            Date marketDate_;
            Date referenceDate_;
            MonotonicDayCountConv dayCountConv_;
            InterpolationType interpolation_;
            std::vector<Date> dates_;
            std::vector<value_type> values_;
        protected:
            void checkPillars() const {
                auto n = dates_.size();
                QL_ASSERT(n > 0, "no pillar dates for the term structure");
                QL_ASSERT(values_.size() == n, "the number of pillar values (" << values_.size() << ") is not what's expected (" << n << ") for the term structure");
            }
        public:
            InterpolatedYieldTermStructDeserializer() :
                dayCountConv_(MonotonicDayCountConv::mdccActual365Fixed),
                interpolation_(InterpolationType::ytsiPiecewiseLinearCont)
            {}
            const Date& marketDate() const { return marketDate_; }
            Date& marketDate() { return marketDate_; }
            Date& referenceDate() { return referenceDate_; }
            const Date& referenceDate() const { return referenceDate_; }
            MonotonicDayCountConv& dayCountConv() { return dayCountConv_; }
            const MonotonicDayCountConv& dayCountConv() const { return dayCountConv_; }
            InterpolationType& interpolation() { return interpolation_; }
            const InterpolationType& interpolation() const { return interpolation_; }
            std::vector<Date>& dates() { return dates_; }
            const std::vector<Date>& dates() const { return dates_; }
            std::vector<value_type>& values() { return values_; }
            const std::vector<value_type>& values() const { return values_; }
            std::vector<std::pair<Date, Real>> nodes() const {
                checkPillars();
                auto n = dates_.size();
                std::vector<std::pair<Date, Real>> results(n);
                for (Size i=0; i < n; ++i) {
                    results[i] = std::make_pair(dates_[i], values[i]);
                }
                return results;
            }
            DayCounter dayCounter() const {
                return MonotonicDayCountHelper::to_daycounter(dayCountConv_);
            }
            // get the actual yield term structure out of it
            operator YieldTermStructurePtr() const {
                checkPillars();
                DayCounter dc = this->dayCounter();
                switch (interpolation_) {
                HANDLE_INTERP_RETURN_BASE_CURVE(ytsiPiecewiseLinearCont);
                HANDLE_INTERP_RETURN_BASE_CURVE(ytsiPiecewiseLinearSimple);
                HANDLE_INTERP_RETURN_BASE_CURVE(ytsiStepForwardCont);
                HANDLE_INTERP_RETURN_BASE_CURVE(ytsiSmoothForwardCont);
                HANDLE_INTERP_RETURN_BASE_CURVE(ytsiPiecewiseLinearForwardCont);
                HANDLE_INTERP_RETURN_BASE_CURVE(ytsiLogLinearDiscount);
                default:
                    QL_FAIL("unsupported yield term structure interpolation type: " << interpolation_);
                }
            }
            InterpolatedYieldTermStructSerializer<value_type> get_serializer(
                Date marketDate = Date()
            ) const {
                YieldTermStructurePtr curve = *this;
                marketDate = (marketDate == Date() ? (marketDate_ == Date() ? Settings::instance().evaluationDate() : marketDate_) : marketDate);
                return InterpolatedYieldTermStructSerializer<value_type>(curve, marketDate);
            }
        };
        
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedForwardSpreadTermStructSerializer {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef VALUE_TYPE value_type;
            struct Row {
                Date date;
                Time term;
                value_type value;
                QLUtils::RateUnit valueUnit;    // hint for the actual serializer on how to serialize the value
                Real primitive;
                Row(
                    Date date = Date(),
                    Time term = 0.0,
                    value_type value = 0.0,
                    QLUtils::RateUnit valueUnit = QLUtils::RateUnit::BasisPoint,
                    Real primitive = 0.0
                ) :
                date(date),
                term(term),
                value(value),
                valueUnit(valueUnit),
                primitive(primitive)
                {}
            };
            typedef Row Row_Type;
            typedef std::vector<Row_Type> TermStructureRows;
            typedef ForwardSpreadInterpolation InterpolationType;
            template <
                InterpolationType INTERP
            >
            using InterpTraits = ForwardSpreadInterpTraits<INTERP>;
        private:
            Date marketDate_;
            Date referenceDate_;
            MonotonicDayCountConv dayCountConv_;
            InterpolationType interpolation_;
            TermStructureRows termStructure_;
        protected:
            static TermStructureRows getCurveTermStructRows(
                const std::vector<Date>& dates,
                const std::vector<value_type>& data,
                const YieldTermStructure& spreadsOnlyCurve,
                QLUtils::RateUnit valueUnit
            ) {
                QL_ASSERT(dates.size() == data.size(), "dates and data size mismatch");
                Date curveRefDate = spreadsOnlyCurve.referenceDate();
                DayCounter dc = spreadsOnlyCurve.dayCounter();
                Size n = dates.size();
                TermStructureRows rows;
                for (Size i = 0; i < n; ++i) {  // for each row
                    const auto& date = dates[i];
                    const auto& value = data[i];
                    Time term = dc.yearFraction(curveRefDate, date);
                    Rate R_Spread = spreadsOnlyCurve.zeroRate(date, dc, Continuous, NoFrequency, true).rate();
                    Real primitive_spread = R_Spread * term;
                    rows.push_back(Row{date, term, value, valueUnit, primitive_spread});
                }
                return rows;
            }
            static std::pair<InterpolationType, TermStructureRows> from_termstructure(
                const YieldTermStructurePtr& spreadsOnlyCurve
            ) {
                QL_ASSERT(spreadsOnlyCurve != nullptr, "yield term structure cannot be null");
                auto valueUnit = QLUtils::RateUnit::BasisPoint;    // hint for the actual serializer on how to serialize the rate/yield value (in this case, in bps unit)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(fsiStep, spreadsOnlyCurve)
                HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR(fsiLinear, spreadsOnlyCurve)
                QL_FAIL("unsupported forward spread term structure interpolation type");
            }    
        public:
            InterpolatedForwardSpreadTermStructSerializer(
                const YieldTermStructurePtr& spreadsOnlyCurve,
                Date marketDate = Date()
            ) :
                marketDate_(marketDate == Date() ? Settings::instance().evaluationDate() : marketDate),
                referenceDate_(spreadsOnlyCurve->referenceDate()),
                dayCountConv_(MonotonicDayCountHelper::from_daycounter(spreadsOnlyCurve->dayCounter())),
                interpolation_(InterpolationType::fsiStep)
            {
                auto [interp, rows] = from_termstructure(spreadsOnlyCurve);
                interpolation_ = interp;
                termStructure_ = rows;
            }
            const Date& marketDate() const { return marketDate_; }
            Date& marketDate() { return marketDate_; }
            const Date& referenceDate() const { return referenceDate_; }
            MonotonicDayCountConv dayCountConv() const { return dayCountConv_; }
            InterpolationType interpolation() const { return interpolation_; }
            const TermStructureRows& termStructure() const { return termStructure_; }
            std::vector<Date> dates() const {
                std::vector<Date> ret;
                for (const auto& row : termStructure_) {
                    ret.push_back(row.date);
                }
                return ret;
            }
            std::vector<Real> data() const {
                std::vector<Real> ret;
                for (const auto& row : termStructure_) {
                    ret.push_back(row.value);
                }
                return ret;
            }
            std::vector<Time> times() const {
                std::vector<Time> ret;
                for (const auto& row : termStructure_) {
                    ret.push_back(row.term);
                }
                return ret;
            }
            std::vector<std::pair<Date, Real>> nodes() const {
                std::vector<std::pair<Date, Real>> results(termStructure_.size());
                for (Size i=0; i<termStructure_.size(); ++i) {
                    const auto& row = termStructure_[i];
                    results[i] = std::make_pair(row.date, row.value);
                }
                return results;
            }
        };
        
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedForwardSpreadTermStructDeserializer {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef VALUE_TYPE value_type;
            typedef typename InterpolatedYieldTermStructDeserializer<VALUE_TYPE>::Row_Type Row_Type;
            typedef std::vector<Row_Type> TermStructureRows;
            typedef ForwardSpreadInterpolation InterpolationType;
            template <
                InterpolationType INTERP
            >
            using InterpTraits = ForwardSpreadInterpTraits<INTERP>;
        private:
            Date marketDate_;
            Date referenceDate_;
            MonotonicDayCountConv dayCountConv_;
            InterpolationType interpolation_;
            std::vector<Date> dates_;
            std::vector<value_type> values_;
        protected:
            void checkPillars() const {
                auto n = dates_.size();
                QL_ASSERT(n > 0, "no pillar dates for the term structure");
                QL_ASSERT(values_.size() == n, "the number of pillar values (" << values_.size() << ") is not what's expected (" << n << ") for the term structure");
            }
        public:
            InterpolatedForwardSpreadTermStructDeserializer() :
                dayCountConv_(MonotonicDayCountConv::mdccActual365Fixed),
                interpolation_(InterpolationType::fsiStep)
            {}
            const Date& marketDate() const { return marketDate_; }
            Date& marketDate() { return marketDate_; }
            Date& referenceDate() { return referenceDate_; }
            const Date& referenceDate() const { return referenceDate_; }
            MonotonicDayCountConv& dayCountConv() { return dayCountConv_; }
            const MonotonicDayCountConv& dayCountConv() const { return dayCountConv_; }
            InterpolationType& interpolation() { return interpolation_; }
            const InterpolationType& interpolation() const { return interpolation_; }
            std::vector<Date>& dates() { return dates_; }
            const std::vector<Date>& dates() const { return dates_; }
            std::vector<value_type>& values() { return values_; }
            const std::vector<value_type>& values() const { return values_; }
            const std::vector<Spread>& forwardSpreads() const { return values_; }
            std::vector<std::pair<Date, Real>> nodes() const {
                checkPillars();
                auto n = dates_.size();
                std::vector<std::pair<Date, Real>> results(n);
                for (Size i=0; i < n; ++i) {
                    results[i] = std::make_pair(dates_[i], values[i]);
                }
                return results;
            }
            std::vector<Handle<Quote>> forwardSpreadQuotes() const {
                std::vector<Handle<Quote>> quotes;
                for (Size i = 0; i < values_.size(); ++i) {
                    const auto& spread = values_[i];
                    quotes.push_back(Handle<Quote>(ext::make_shared<SimpleQuote>(spread)));
                }
                return quotes;
			}
            DayCounter dayCounter() const {
                return MonotonicDayCountHelper::to_daycounter(dayCountConv_);
            }
            operator YieldTermStructurePtr() const {
                checkPillars();
                DayCounter dc = this->dayCounter();
                switch (interpolation_) {
                HANDLE_INTERP_RETURN_BASE_CURVE(fsiStep);
                HANDLE_INTERP_RETURN_BASE_CURVE(fsiLinear);
                default:
                    QL_FAIL("unsupported forward spread term structure interpolation type: " << interpolation_);
                }
            }
            YieldTermStructurePtr spreadsOnlyTermStructure() const {
                return this->operator YieldTermStructurePtr();
			}
            InterpolatedForwardSpreadTermStructSerializer<value_type> get_serializer(
                Date marketDate = Date()
            ) const {
                YieldTermStructurePtr spreadsOnlyCurve = *this;
				marketDate = (marketDate == Date() ? (marketDate_ == Date() ? Settings::instance().evaluationDate(): marketDate_) : marketDate);
                return InterpolatedForwardSpreadTermStructSerializer<value_type>(spreadsOnlyCurve, marketDate);
            }
            YieldTermStructurePtr forwardSpreadedTermStructure (
                const Handle<YieldTermStructure>& baseCurve
            ) const {
                checkPillars();
                QL_REQUIRE(referenceDate_ != Date(), "curve's reference date is not set");
                QL_REQUIRE(baseCurve->referenceDate() == referenceDate_, "base curve's reference date (" << ISODateConv::to_str(baseCurve->referenceDate()) << ") is not what's expected (" << ISODateConv::to_str(referenceDate_) << ")");
                QL_REQUIRE(baseCurve->dayCounter().name() == this->dayCounter().name(), "base curve's day counter (" << baseCurve->dayCounter().name() << ") is not what's expected (" << this->dayCounter().name() << ")");
                auto quotes = forwardSpreadQuotes();
                switch (interpolation_) {
                HANDLE_INTERP_RETURN_FORWARD_SPREADED_CURVE(fsiStep);
                HANDLE_INTERP_RETURN_FORWARD_SPREADED_CURVE(fsiLinear);
                default:
                    QL_FAIL("unsupported forward spread term structure interpolation type: " << interpolation_);
                }
            }
        };
    }
}

#undef INTERP_BASE_CURVE_TYPE
#undef INTERP_FORWARD_SPREADED_CURVE_TYPE
#undef HANDLE_INTERP_MAKE_INTERP_ROWS_PAIR
#undef HANDLE_INTERP_RETURN_BASE_CURVE
#undef HANDLE_INTERP_RETURN_FORWARD_SPREADED_CURVE
