#pragma once

#include <ql/quantlib.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

namespace QuantLib {
	class NLiborForwardModel : public LiborForwardModel {
	public:
		NLiborForwardModel(
			const ext::shared_ptr<LiborForwardModelProcess>& process,
			const ext::shared_ptr<LmVolatilityModel>& volaModel,
			const ext::shared_ptr<LmCorrelationModel>& corrModel
		): LiborForwardModel(process, volaModel, corrModel)
		{}

		// approx. swaption matrix using Rebonato's approx.
		// fix and floating leg have the same frequency
		ext::shared_ptr<SwaptionVolatilityMatrix> getSwaptionVolatilityMatrix() const override {
            if (swaptionVola != nullptr) {
                return swaptionVola;
            }

            const ext::shared_ptr<IborIndex> index = process_->index();
            const Date today = process_->fixingDates()[0];

            const Size size = process_->size() / 2;
            Matrix volatilities(size, size);

            std::vector<Date> exercises(process_->fixingDates().begin() + 1,
                process_->fixingDates().begin() + size + 1);

            std::vector<Period> lengths(size);
            for (Size i = 0; i < size; ++i) {
                lengths[i] = (i + 1) * index->tenor();
            }

            const Array f = process_->initialValues();
            for (Size k = 0; k < size; ++k) {
                const Size alpha = k;
                const Time t_alpha = process_->fixingTimes()[alpha + 1];

                Matrix var(size, size);
                for (Size i = alpha + 1; i <= k + size; ++i) {
                    for (Size j = i; j <= k + size; ++j) {
                        var[i - alpha - 1][j - alpha - 1] = var[j - alpha - 1][i - alpha - 1] =
                            covarProxy_->integratedCovariance(i, j, t_alpha);
                    }
                }

                for (Size l = 1; l <= size; ++l) {
                    const Size beta = l + k;
                    const Array w = w_0(alpha, beta);

                    Real sum = 0.0;
                    for (Size i = alpha + 1; i <= beta; ++i) {
                        for (Size j = alpha + 1; j <= beta; ++j) {
                            //sum += w[i] * w[j] * f[i] * f[j] * var[i - alpha - 1][j - alpha - 1];
                            sum += w[i] * w[j] * var[i - alpha - 1][j - alpha - 1];
                        }
                    }
					// since std::sqrt(sum / t_alpha) is the annual normal volatility of the swap rate ==> std::sqrt(sum / t_alpha) / S_0(alpha, beta) is the annual lognormal volatility of the swap rate
                    //volatilities[k][l - 1] = std::sqrt(sum / t_alpha) / S_0(alpha, beta);
                    volatilities[k][l - 1] = std::sqrt(sum / t_alpha);
                }
            }

            return swaptionVola = ext::make_shared<SwaptionVolatilityMatrix>(
                today, NullCalendar(), Following,
                exercises, lengths, volatilities,
                index->dayCounter(), false, VolatilityType::Normal);
		}

		Real discountBondOption(
			Option::Type type,
			Real strike,
			Time maturity,
			Time bondMaturity
		) const override {
            const std::vector<Time>& accrualStartTimes
                = process_->accrualStartTimes();
            const std::vector<Time>& accrualEndTimes
                = process_->accrualEndTimes();

            QL_REQUIRE(accrualStartTimes.front() <= maturity
                && accrualStartTimes.back() >= maturity,
                "capet maturity does not fit to the process");

            const Size i = std::lower_bound(accrualStartTimes.begin(),
                accrualStartTimes.end(),
                maturity) - accrualStartTimes.begin();

            QL_REQUIRE(i < process_->size()
                && std::fabs(maturity - accrualStartTimes[i])
                < 100 * std::numeric_limits<Real>::epsilon()
                && std::fabs(bondMaturity - accrualEndTimes[i])
                < 100 * std::numeric_limits<Real>::epsilon(),
                "irregular fixings are not (yet) supported");

            const Real tenor = accrualEndTimes[i] - accrualStartTimes[i];
            const Real forward = process_->initialValues()[i];
            const Real capRate = (1.0 / strike - 1.0) / tenor;
            const Volatility var = covarProxy_
                ->integratedCovariance(i, i, process_->fixingTimes()[i]);
            const DiscountFactor dis =
                process_->index()->forwardingTermStructure()->discount(bondMaturity);

            const Real bachelier = bachelierBlackFormula(
                (type == Option::Put ? Option::Call : Option::Put),
                capRate, forward, std::sqrt(var));

            const Real npv = dis * tenor * bachelier;

            return npv / (1.0 + capRate * tenor);
		}
	};
}
