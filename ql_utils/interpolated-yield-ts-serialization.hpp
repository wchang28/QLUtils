#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <ql_utils/types.hpp>

namespace QuantLib {
    namespace Utils {
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedYieldTermStructSerializer {
        public:
            typedef VALUE_TYPE value_type;
            struct Row {
                Date date;
                Time term;
                value_type value;
                Rate zeroRate;
                Rate forwardRate;
                Rate simpleRate;
                DiscountFactor discountFactor;
                Row(
                    Date date = Date(),
                    Time term = 0.0,
                    value_type value = 0.0,
                    Rate zeroRate = 0.0,
                    Rate forwardRate = 0.0,
                    Rate simpleRate = 0.0,
                    DiscountFactor discountFactor = 1.0
                ) : date(date), term(term), value(value), zeroRate(zeroRate), forwardRate(forwardRate), simpleRate(simpleRate), discountFactor(discountFactor)
                {}
            };
        private:
            Date marketDate_;
            Date referenceDate_;
            TermStructureDayCountConv dayCountConv_;
            YieldTermStructureInterpolation interpolation_;
            std::vector<Row> termStructure_;
        public:
            static std::vector<Row> getCurveTermStructRows(
                const std::vector<Date>& dates,
                const std::vector<value_type>& data,
                const ext::shared_ptr<YieldTermStructure>& curve
            ) {
                QL_REQUIRE(dates.size() == data.size(), "dates and data size mismatch");
                QL_ASSERT(curve != nullptr, "curve cannot be nullptr");
                auto dc = curve->dayCounter();
                auto n = dates.size();
                std::vector<Row> ret;
                for (size_t i = 0; i < n; ++i) {
                    const auto& date = dates[i];
                    const auto& value = data[i];
                    auto term = dc.yearFraction(curve->referenceDate(), date);
                    auto zeroRate = curve->zeroRate(date, dc, Continuous, NoFrequency, true).rate();
                    auto forwardRate = curve->forwardRate(date, date, dc, Continuous, NoFrequency, true).rate();
                    auto simpleRate = curve->zeroRate(date, dc, Simple, NoFrequency, true).rate();
                    auto discountFactor = curve->discount(date);
                    ret.push_back(Row{ date, term, value, zeroRate, forwardRate, simpleRate, discountFactor });
                }
                return ret;
            }
            static std::pair<YieldTermStructureInterpolation, std::vector<Row>> from_termstructure(
                const ext::shared_ptr<YieldTermStructure>& curve
            ) {
                QL_REQUIRE(curve != nullptr, "curve cannot be nullptr");
                auto pLinearContZeroCurve = ext::dynamic_pointer_cast<InterpolatedZeroCurve<Linear>>(curve);
                auto pLinearSimpleZeroCurve = ext::dynamic_pointer_cast<InterpolatedSimpleZeroCurve<Linear>>(curve);
                auto pBackwardFlatContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<BackwardFlat>>(curve);
                auto pSmoothContForwardCurve = ext::dynamic_pointer_cast<InterpolatedForwardCurve<ConvexMonotone>>(curve);
                if (pLinearContZeroCurve != nullptr) {
                    return std::make_pair(YieldTermStructureInterpolation::ytsiPiecewiseLinearCont, getCurveTermStructRows(pLinearContZeroCurve->dates(), pLinearContZeroCurve->data(), curve));
                }
                else if (pLinearSimpleZeroCurve != nullptr) {
                    return std::make_pair(YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple, getCurveTermStructRows(pLinearSimpleZeroCurve->dates(), pLinearSimpleZeroCurve->data(), curve));
                }
                else if (pBackwardFlatContForwardCurve != nullptr) {
                    return std::make_pair(YieldTermStructureInterpolation::ytsiStepForwardCont, getCurveTermStructRows(pBackwardFlatContForwardCurve->dates(), pBackwardFlatContForwardCurve->data(), curve));
                }
                else if (pSmoothContForwardCurve != nullptr) {
                    return std::make_pair(YieldTermStructureInterpolation::ytsiSmoothForwardCont, getCurveTermStructRows(pSmoothContForwardCurve->dates(), pSmoothContForwardCurve->data(), curve));
                }
                else {
                    QL_FAIL("unsupported term structure type for output");
                }
            }    
        public:
            InterpolatedYieldTermStructSerializer(
                const ext::shared_ptr<YieldTermStructure>& curve,
                Date marketDate = Date()
            ) :
                marketDate_(marketDate == Date() ? Settings::instance().evaluationDate() : marketDate),
                referenceDate_(curve->referenceDate()),
                dayCountConv_(TermStructureDayCountConverter::from_daycounter(curve->dayCounter())),
                interpolation_(YieldTermStructureInterpolation::ytsiPiecewiseLinearCont)
            {
                auto [interp, rows] = from_termstructure(curve);
                interpolation_ = interp;
                termStructure_ = rows;
            }
            const Date& marketDate() const { return marketDate_; }
            Date& marketDate() { return marketDate_; }
            const Date& referenceDate() const { return referenceDate_; }
            TermStructureDayCountConv dayCountConv() const { return dayCountConv_; }
            YieldTermStructureInterpolation interpolation() const { return interpolation_; }
            const std::vector<Row>& termStructure() const { return termStructure_; }
        };
        
        template<
            typename VALUE_TYPE = Real
        >
        class InterpolatedYieldTermStructDeserializer {
        public:
			typedef VALUE_TYPE value_type;
            struct Row {
                Date date;
                value_type value;
            };
        private:
            Date referenceDate_;
            TermStructureDayCountConv dayCountConv_;
            YieldTermStructureInterpolation interpolation_;
            std::vector<Date> dates_;
            std::vector<value_type> values_;
        public:
            InterpolatedYieldTermStructDeserializer() :
                dayCountConv_(TermStructureDayCountConv::tsdccActual365Fixed),
                interpolation_(YieldTermStructureInterpolation::ytsiPiecewiseLinearCont)
            {}
            Date& referenceDate() { return referenceDate_; }
            const Date& referenceDate() const { return referenceDate_; }
            TermStructureDayCountConv& dayCountConv() { return dayCountConv_; }
            const TermStructureDayCountConv& dayCountConv() const { return dayCountConv_; }
            YieldTermStructureInterpolation& interpolation() { return interpolation_; }
            const YieldTermStructureInterpolation& interpolation() const { return interpolation_; }
            std::vector<Date>& dates() { return dates_; }
            const std::vector<Date>& dates() const { return dates_; }
            std::vector<value_type>& values() { return values_; }
            const std::vector<value_type>& values() const { return values_; }
            operator ext::shared_ptr<YieldTermStructure>() const {
                DayCounter dayCounter = TermStructureDayCountConverter::to_daycounter(dayCountConv_);
                switch (interpolation_) {
                case YieldTermStructureInterpolation::ytsiPiecewiseLinearCont:
                    return ext::make_shared<InterpolatedZeroCurve<Linear>>(dates_, values_, dayCounter);
                case YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple:
                    return ext::make_shared<InterpolatedSimpleZeroCurve<Linear>>(dates_, values_, dayCounter);
                case YieldTermStructureInterpolation::ytsiStepForwardCont:
                    return ext::make_shared<InterpolatedForwardCurve<BackwardFlat>>(dates_, values_, dayCounter);
                case YieldTermStructureInterpolation::ytsiSmoothForwardCont:
                    return ext::make_shared<InterpolatedForwardCurve<ConvexMonotone>>(dates_, values_, dayCounter);
                default:
                    QL_FAIL("unsupported interpolation type: " << interpolation_);
                }
            }
            InterpolatedYieldTermStructSerializer<value_type> get_serializer(
                Date marketDate = Date()
            ) const {
                ext::shared_ptr<YieldTermStructure> curve = *this;
                return InterpolatedYieldTermStructSerializer<value_type>(curve, marketDate);
            }
        };
    }
}