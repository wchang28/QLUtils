#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/bootstrap.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

namespace QLUtils {
    template <typename ZeroCurveInterp = QuantLib::Linear>
    class ParYieldSplineBootstrap : public Bootstrapper {
    public:
        // input
        pInstruments instruments;
        // output
        std::vector<QuantLib::Time> parTerms;
        std::vector<QuantLib::Rate> parYields;
        pInstruments parInstruments;
		QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<ZeroCurveInterp>> discountZeroCurve;

        struct DefaultParInstrumentsFactory {
            pInstruments operator() (
                const QuantLib::Interpolation&,
                const pInstruments& instruments
                ) const {
                return instruments; // default just assume all input instruments are all par instruments
            }
        };
        struct DefaultActualVsImpliedComparison {
            QuantLib::Rate operator() (
                std::ostream& os,
                const std::shared_ptr<BootstrapInstrument>& inst,
                const QuantLib::Rate& actual,
                const QuantLib::Rate& implied
                ) const {
                os << inst->tenor();
                os << "," << inst->ticker();
                os << "," << "actual=" << actual * 100.0;
                os << "," << "implied=" << implied * 100.0;
                os << "," << "diff=" << (implied - actual) * 10000.0 << " bp";
                os << std::endl;
                return implied - actual;
            }
        };
        void clearOutputs() {
            parTerms.resize(0);
            parYields.resize(0);
            parInstruments = nullptr;
            discountZeroCurve = nullptr;
        }
    private:
        void checkInstruments() const {
            QL_REQUIRE(instruments != nullptr, "instruments is not set");
            QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
            for (auto const& i : *instruments) {
                auto parYieldTSInst = std::dynamic_pointer_cast<QLUtils::ParYieldTermStructInstrument>(i);
                QL_REQUIRE(parYieldTSInst != nullptr, "instrument " << i->ticker() << " is not a par yield term structure instrument");
            }
        }
        void checkParInstruments() const {
            QL_REQUIRE(parInstruments != nullptr, "par instruments is not set");
            QL_REQUIRE(!parInstruments->empty(), "par instruments cannot be empty");
            for (auto const& inst : *parInstruments) {
                auto parInstrument = std::dynamic_pointer_cast<QLUtils::ParInstrument>(inst);
                QL_REQUIRE(parInstrument != nullptr, "instrument " << inst->ticker() << " is not a par instrument");
            }
        }
    public:
        template<typename ParYieldInterp, typename ParInstrumentsFactory = DefaultParInstrumentsFactory>
        void bootstrap(
            const QuantLib::Date& curveReferenceDate,
            const ParInstrumentsFactory& parInstromentsFactory = ParInstrumentsFactory(),
            const ParYieldInterp& parYiledInterp = ParYieldInterp(),
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const ZeroCurveInterp& zcInterp = ZeroCurveInterp()
        ) {
            clearOutputs();
            checkInstruments();
            // create the par yield term structure interplation
            for (auto const& i : *instruments) {
                auto parYieldTSInst = std::dynamic_pointer_cast<QLUtils::ParYieldTermStructInstrument>(i);
                auto parTerm = parYieldTSInst->parTerm();
                auto parYield = parYieldTSInst->parYield();
                parTerms.push_back(parTerm);
                parYields.push_back(parYield);
            }
            auto parYieldInterpolation = parYiledInterp.interpolate(parTerms.begin(), parTerms.end(), parYields.begin());
            // with the par yield interpolation and the instruments, create the par instrments for bootstrapping
            parInstruments = parInstromentsFactory(parYieldInterpolation, instruments);
            // check the par instruments before the bootstrap
            checkParInstruments();
            // perform the bootstrap
            ZeroCurvesBootstrap<ZeroCurveInterp> bootstrap;
            bootstrap.instruments = parInstruments;
            bootstrap.bootstrap(curveReferenceDate, dayCounter, zcInterp);
            discountZeroCurve = bootstrap.discountZeroCurve;
        }
        template<typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison>
        QuantLib::Rate verify(
            std::ostream& os,
            std::streamsize precision = 16,
            const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
        ) const {
            QL_REQUIRE(discountZeroCurve != nullptr, "discount zero curve cannot be null");
            checkParInstruments();
            QuantLib::Handle<QuantLib::YieldTermStructure> discountingTermStructure(discountZeroCurve);
            return verifyImpl(
                parInstruments,
                [&discountingTermStructure](const pInstrument& inst) -> QuantLib::Rate {
                    auto parInstrument = std::dynamic_pointer_cast<ParInstrument>(inst);
                    return parInstrument->impliedParRate(discountingTermStructure);
                },
                os,
                precision,
                compare
            );
        }
    };
}