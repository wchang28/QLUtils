#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <ql_utils/types.hpp>
#include <ql_utils/daycounters/monotonic-day-count-helper.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>

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
                QLUtils::RateUnit valueUnit;
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
            typedef YieldTermStructureInterpolation InterpolationType;
        private:
            Date marketDate_;
            Date referenceDate_;
            MonotonicDayCountConv dayCountConv_;
            InterpolationType interpolation_;
            std::vector<Row> termStructure_;
        protected:
            static std::vector<Row> getCurveTermStructRows(
                const std::vector<Date>& dates,
                const std::vector<value_type>& data,
                const YieldTermStructure& curve,
                QLUtils::RateUnit valueUnit
            ) {
                QL_ASSERT(dates.size() == data.size(), "dates and data size mismatch");
                Date curveRefDate = curve.referenceDate();
                DayCounter dc = curve.dayCounter();
                Size n = dates.size();
                std::vector<Row> rows;
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
            static std::pair<InterpolationType, std::vector<Row>> from_termstructure(
                const YieldTermStructurePtr& curve
            ) {
                QL_ASSERT(curve != nullptr, "yield term structure cannot be null");
                auto pLinearContZeroCurve = ext::dynamic_pointer_cast<InterpolatedZeroCurve<Linear>>(curve);
                auto pLinearSimpleZeroCurve = ext::dynamic_pointer_cast<InterpolatedSimpleZeroCurve<Linear>>(curve);
                auto pBackwardFlatContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<BackwardFlat>>(curve);
                auto pSmoothContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<ConvexMonotone>>(curve);
                auto pLinearContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<Linear>>(curve);
                auto valueUnit = QLUtils::RateUnit::Percent;
                if (pLinearContZeroCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::ytsiPiecewiseLinearCont,
                        getCurveTermStructRows(
                            pLinearContZeroCurve->dates(),
                            pLinearContZeroCurve->data(),
                            *curve,
                            valueUnit
                        )
                    );
                }
                else if (pLinearSimpleZeroCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::ytsiPiecewiseLinearSimple,
                        getCurveTermStructRows(
                            pLinearSimpleZeroCurve->dates(),
                            pLinearSimpleZeroCurve->data(),
                            *curve,
                            valueUnit
                        )
                    );
                }
                else if (pBackwardFlatContForwardCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::ytsiStepForwardCont,
                        getCurveTermStructRows(
                            pBackwardFlatContForwardCurve->dates(),
                            pBackwardFlatContForwardCurve->data(),
                            *curve,
                            valueUnit
                        )
                    );
                }
                else if (pSmoothContForwardCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::ytsiSmoothForwardCont,
                        getCurveTermStructRows(
                            pSmoothContForwardCurve->dates(),
                            pSmoothContForwardCurve->data(),
                            *curve,
                            valueUnit
                        ));
                }
                else if (pLinearContForwardCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::ytsiPiecewiseLinearForwardCont,
                        getCurveTermStructRows(
                            pLinearContForwardCurve->dates(),
                            pLinearContForwardCurve->data(),
                            *curve,
                            valueUnit
                        )
                    );
                }
                else {
                    QL_FAIL("unsupported yield term structure traits/interpolation type");
                }
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
            const std::vector<Row>& termStructure() const { return termStructure_; }
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
            typedef YieldTermStructureInterpolation InterpolationType;
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
            DayCounter dayCounter() const {
                return MonotonicDayCountHelper::to_daycounter(dayCountConv_);
            }
            operator YieldTermStructurePtr() const {
                checkPillars();
                DayCounter dc = this->dayCounter();
                switch (interpolation_) {
                case InterpolationType::ytsiPiecewiseLinearCont:
                    return ext::make_shared<InterpolatedZeroCurve<Linear>>(dates_, values_, dc);
                case InterpolationType::ytsiPiecewiseLinearSimple:
                    return ext::make_shared<InterpolatedSimpleZeroCurve<Linear>>(dates_, values_, dc);
                case InterpolationType::ytsiStepForwardCont:
                    return ext::make_shared<InterpolatedForwardCurve<BackwardFlat>>(dates_, values_, dc);
                case InterpolationType::ytsiSmoothForwardCont:
                    return ext::make_shared<InterpolatedForwardCurve<ConvexMonotone>>(dates_, values_, dc);
                case InterpolationType::ytsiPiecewiseLinearForwardCont:
                    return ext::make_shared<InterpolatedForwardCurve<Linear>>(dates_, values_, dc);
                default:
                    QL_FAIL("unsupported yield term structure traits/interpolation type: " << interpolation_);
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
                QLUtils::RateUnit valueUnit;
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
            typedef ForwardSpreadInterpolation InterpolationType;
        private:
            Date marketDate_;
            Date referenceDate_;
            MonotonicDayCountConv dayCountConv_;
            InterpolationType interpolation_;
            std::vector<Row> termStructure_;
        protected:
            static std::vector<Row> getCurveTermStructRows(
                const std::vector<Date>& dates,
                const std::vector<value_type>& data,
                const YieldTermStructure& curve,
                QLUtils::RateUnit valueUnit
            ) {
                QL_ASSERT(dates.size() == data.size(), "dates and data size mismatch");
                Date curveRefDate = curve.referenceDate();
                DayCounter dc = curve.dayCounter();
                Size n = dates.size();
                std::vector<Row> rows;
                for (Size i = 0; i < n; ++i) {  // for each row
                    const auto& date = dates[i];
                    const auto& value = data[i];
                    Time term = dc.yearFraction(curveRefDate, date);
                    Rate R_Spread = curve.zeroRate(date, dc, Continuous, NoFrequency, true).rate();
                    Real primitive_spread = R_Spread * term;
                    rows.push_back(Row{date, term, value, valueUnit, primitive_spread});
                }
                return rows;
            }
            static std::pair<InterpolationType, std::vector<Row>> from_termstructure(
                const YieldTermStructurePtr& curve
            ) {
                QL_ASSERT(curve != nullptr, "yield term structure cannot be null");
                auto pBackwardFlatContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<BackwardFlat>>(curve);
                auto pLinearContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<Linear>>(curve);
                auto valueUnit = QLUtils::RateUnit::BasisPoint;
                if (pBackwardFlatContForwardCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::fsiStep,
                        getCurveTermStructRows(
                            pBackwardFlatContForwardCurve->dates(),
                            pBackwardFlatContForwardCurve->data(),
                            *curve,
                            valueUnit
                        )
                    );
                }
                else if (pLinearContForwardCurve != nullptr) {
                    return std::make_pair(
                        InterpolationType::fsiLinear,
                        getCurveTermStructRows(
                            pLinearContForwardCurve->dates(),
                            pLinearContForwardCurve->data(),
                            *curve,
                            valueUnit
                        )
                    );
                }
                else {
                    QL_FAIL("unsupported forward spread term structure interpolation type");
                }
            }    
        public:
            InterpolatedForwardSpreadTermStructSerializer(
                const YieldTermStructurePtr& curve,
                Date marketDate = Date()
            ) :
                marketDate_(marketDate == Date() ? Settings::instance().evaluationDate() : marketDate),
                referenceDate_(curve->referenceDate()),
                dayCountConv_(MonotonicDayCountHelper::from_daycounter(curve->dayCounter())),
                interpolation_(InterpolationType::fsiStep)
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
            const std::vector<Row>& termStructure() const { return termStructure_; }
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
        };
        
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedForwardSpreadTermStructDeserializer {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef VALUE_TYPE value_type;
            typedef typename InterpolatedYieldTermStructDeserializer<VALUE_TYPE>::Row_Type Row_Type;
            typedef ForwardSpreadInterpolation InterpolationType;
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
            DayCounter dayCounter() const {
                return MonotonicDayCountHelper::to_daycounter(dayCountConv_);
            }
            operator YieldTermStructurePtr() const {
                checkPillars();
                DayCounter dc = this->dayCounter();
                switch (interpolation_) {
                case InterpolationType::fsiStep:
                    return ext::make_shared<InterpolatedForwardCurve<BackwardFlat>>(dates_, values_, dc);
                case InterpolationType::fsiLinear:
                    return ext::make_shared<InterpolatedForwardCurve<Linear>>(dates_, values_, dc);
                default:
                    QL_FAIL("unsupported forward spread term structure interpolation type: " << interpolation_);
                }
            }
            InterpolatedForwardSpreadTermStructSerializer<value_type> get_serializer(
                Date marketDate = Date()
            ) const {
                YieldTermStructurePtr curve = *this;
				marketDate = (marketDate == Date() ? (marketDate_ == Date() ? Settings::instance().evaluationDate(): marketDate_) : marketDate);
                return InterpolatedForwardSpreadTermStructSerializer<value_type>(curve, marketDate);
            }
            YieldTermStructurePtr forwardSpreadedTermStructure (
                const Handle<YieldTermStructure>& baseCurve
            ) const {
                checkPillars();
                QL_REQUIRE(referenceDate_ != Date(), "curve's reference date is not set");
                QL_REQUIRE(baseCurve->referenceDate() == referenceDate_, "base curve's reference date (" << ISODateConv::to_str(baseCurve->referenceDate()) << ") is not what's expected (" << ISODateConv::to_str(referenceDate_) << ")");
                QL_REQUIRE(baseCurve->dayCounter().name() == this->dayCounter().name(), "base curve's day counter (" << baseCurve->dayCounter().name() << ") is not what's expected (" << this->dayCounter().name() << ")");
                std::vector<Handle<Quote>> quotes;
                for (Size i = 0; i < dates_.size(); ++i) {
                    const auto& spread = values_[i];
                    quotes.push_back(Handle<Quote>(ext::make_shared<SimpleQuote>(spread)));
                }
                switch (interpolation_) {
                case InterpolationType::fsiStep:
                    return ext::make_shared<InterpolatedPiecewiseForwardSpreadedTermStructure<BackwardFlat>>(baseCurve, quotes, dates_);
                case InterpolationType::fsiLinear:
                    return ext::make_shared<InterpolatedPiecewiseForwardSpreadedTermStructure<Linear>>(baseCurve, quotes, dates_);
                default:
                    QL_FAIL("unsupported forward spread term structure interpolation type: " << interpolation_);
                }
            }
        };
    }
}
