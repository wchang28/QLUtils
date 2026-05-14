#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <ql_utils/dateformat.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

namespace QuantLib {
    namespace Utils {
        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename Interpolator = Linear
        >
        class ParYieldSplineBootstrap : public Bootstrapper {
        public:
            typedef typename Traits::template curve<Interpolator>::type BaseCurveType;	// InterpolatedZeroCurve<Interpolator>, InterpolatedDiscountCurve<Interpolator>, InterpolatedForwardCurve<Interpolator>, or InterpolatedSimpleZeroCurve<Interpolator>
            typedef YieldCurvesBootstrap<Traits, Interpolator> YieldCurveBootstrapType;
        public:
            // input
            pInstruments instruments;   // instrument must be of type QLUtils::ParYieldTermStructInstrument
            // output
            std::vector<Time> parTerms;
            std::vector<Rate> parYields;
            pInstruments parInstruments;    // par instrument must be of type QLUtils::ParInstrument
            ext::shared_ptr<BaseCurveType> discountCurve;

            struct DefaultParInstrumentsFactory {
                pInstruments operator() (
                    const Interpolation&,
                    const pInstruments& instruments
                ) const {
                    return instruments; // default just assume all input instruments are all par instruments
                }
            };
            struct DefaultActualVsImpliedComparison {
                Rate operator() (
                    std::ostream& os,
                    const std::shared_ptr<QLUtils::BootstrapInstrument>& inst,
                    const Real& actual,
                    const Real& implied
                ) const {
                    using DtFormat = QLUtils::DateFormat<char>;
                    auto startDate = inst->startDate();
                    auto endDate = inst->maturityDate();
                    auto diff = implied - actual;
                    os << inst->tenor();
                    os << "," << inst->ticker();
                    os << "," << "[" << DtFormat::to_yyyymmdd(startDate, true) << "," << DtFormat::to_yyyymmdd(endDate, true) << ")";
                    os << "," << "actual=" << actual * inst->valueMultiplier();
                    os << "," << "implied=" << implied * inst->valueMultiplier();
                    os << "," << "diff=" << diff * inst->basisPointDiffMultiplier() << " bp";
                    os << std::endl;
                    return diff * inst->absoluteDiffMultiplier();
                }
            };
            void clearOutputs() {
                parTerms.resize(0);
                parYields.resize(0);
                parInstruments = nullptr;
                discountCurve = nullptr;
            }
        private:
            void checkInstruments() const {
                QL_REQUIRE(instruments != nullptr, "instruments is not set");
                QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
                for (auto const& i : *instruments) {
                    auto parYieldTSInst = std::dynamic_pointer_cast<QLUtils::IParYieldSplineNode>(i);
                    QL_REQUIRE(parYieldTSInst != nullptr, "instrument " << i->ticker() << " is not a par yield spline node");
                }
            }
            void checkParInstruments() const {
                QL_REQUIRE(parInstruments != nullptr, "par instruments is not set");
                QL_REQUIRE(!parInstruments->empty(), "par instruments cannot be empty");
                for (auto const& inst : *parInstruments) {
                    auto parInstrument = std::dynamic_pointer_cast<QLUtils::IParRateInstrument>(inst);
                    QL_REQUIRE(parInstrument != nullptr, "instrument " << inst->ticker() << " is not a par rate instrument");
                }
            }
        public:
            template<
                typename ParYieldInterp,
                typename ParInstrumentsFactory = DefaultParInstrumentsFactory
            >
            void bootstrap(
                const Date& curveReferenceDate,
                const ParYieldInterp& parYiledInterp = ParYieldInterp(),
                const ParInstrumentsFactory& parInstromentsFactory = ParInstrumentsFactory(),
                const DayCounter& dayCounter = Actual365Fixed(),
                const Interpolator& interp = Interpolator()
            ) {
                clearOutputs();
                checkInstruments();
                // create the par yield term structure interplation
                for (auto const& i : *instruments) {
                    if (i->use()) {
                        auto parYieldTSInst = std::dynamic_pointer_cast<QLUtils::IParYieldSplineNode>(i);
                        auto parTerm = parYieldTSInst->parTerm();
                        auto parYield = parYieldTSInst->parYield();
                        parTerms.push_back(parTerm);
                        parYields.push_back(parYield);
                    }
                }
                QL_REQUIRE(parTerms.size() >= 2, "require at least 2 points to spline par yields");
                auto parYieldInterpolation = parYiledInterp.interpolate(parTerms.begin(), parTerms.end(), parYields.begin());
                parYieldInterpolation.enableExtrapolation();
                // with the par yield interpolation and the instruments, create the par instrments for bootstrapping
                parInstruments = parInstromentsFactory(parYieldInterpolation, instruments);
                // check the par instruments before the bootstrap
                checkParInstruments();
                // perform the bootstrap
                YieldCurveBootstrapType bootstrapper;
                bootstrapper.instruments = parInstruments;
                bootstrapper.bootstrap(curveReferenceDate, dayCounter, interp);
                discountCurve = bootstrapper.discountCurve;
            }
            template<
                typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison
            >
            Rate verify(
                std::ostream& os,
                std::streamsize precision = 16,
                const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
            ) const {
                QL_REQUIRE(discountCurve != nullptr, "discount zero curve cannot be null");
                checkParInstruments();
                Handle<YieldTermStructure> discountingTermStructure(discountCurve);
                return verifyImpl(
                    parInstruments,
                    [&discountingTermStructure](const pInstrument& inst) -> Rate {
                        auto parInstrument = std::dynamic_pointer_cast<QLUtils::IParRateInstrument>(inst);
                        return parInstrument->impliedParRate(discountingTermStructure);
                    },
                    os,
                    precision,
                    compare
                );
            }
        };
    }
}
