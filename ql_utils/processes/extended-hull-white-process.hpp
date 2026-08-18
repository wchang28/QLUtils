#pragma once

#include <ql/quantlib.hpp>
#include <functional>
#include <cmath>
#include <vector>

namespace QuantLib {
    class ExtendedHullWhiteProcess : public StochasticProcess1D {
    private:
        std::vector<Time> times_;
        std::vector<Rate> alphas_;
        Interpolation alphaInterp_;
    protected:
        ext::shared_ptr<GeneralizedOrnsteinUhlenbeckProcess> process_;
        Handle<YieldTermStructure> h_;
        Real a_;
        std::function<Real(Time)> sigma_;
    public:
        ExtendedHullWhiteProcess(
            const Handle<YieldTermStructure>& h,
            Real a,
            std::function<Real(Time)> vol,
            const TimeGrid& timeGrid
        ) :
            h_(h),
            a_(a),
            sigma_(vol),
            process_(new GeneralizedOrnsteinUhlenbeckProcess(
                [a](Time) { return a; },    // speed function
                vol,    // vol function
                h->forwardRate(0.0, 0.0, Continuous, NoFrequency),    // x0 = f(0, 0)
                0.0    // level == 0
            ))
        {
            QL_REQUIRE(a_ >= 0.0, "negative a given");
            // alpha(t) calculation is very slow, so we pre-calculate it for the time grid
            ///////////////////////////////////////////////////////////////////////////////////////////////
            times_.assign(timeGrid.begin(), timeGrid.end());
            Size nTimes = times_.size();
            alphas_.resize(nTimes, 0.0);
            for (Size timeIndex = 0; timeIndex < nTimes; ++timeIndex) {
                Time t = times_[timeIndex];
                alphas_[timeIndex] = alpha(t, a, vol, h);
            }
            LinearFlat interpTraits;
            alphaInterp_ = interpTraits.interpolate(times_.begin(), times_.end(), alphas_.begin());
            alphaInterp_.enableExtrapolation();
            ///////////////////////////////////////////////////////////////////////////////////////////////
        }
    public:
        Real a() const {
            return a_;
        }
        std::function<Real(Time)> sigma() const {
            return sigma_;
        }
        // how much E(r(t)) drifts away from the ZV instantaneous forward rate f(0, t) due to the mean reversion of the Hull-White process
        // drift = integral_0^t sigma(u)^2 * (1 - exp(-a * (t - u))) * exp(-a * (t - u)) / a du
        // if sigma is constant, then drift = sigma^2/(2 * a^2) * (1 - exp(-a * t))^2
        static Rate zv_drift_term(
            Time t,
            Real a,
            const std::function<Real(Time)>& sigma
        ) {
            auto integrand = [&sigma, &t, &a](Time u) {
                Volatility vol = sigma(u);
                Real result = vol * vol * (1.0 - std::exp(-a * (t - u))) * std::exp(-a * (t - u)) / a;
                return result;
            };
            SimpsonIntegral integrator(1e-12, 1000);
            Real mr = integrator(integrand, 0.0, t);
            return mr;
        }
        // alpha(t) is E[r(t)]
        // alpha(t) = f(0, t) + integral_0^t sigma(u)^2 * (1 - exp(-a * (t - u))) * exp(-a * (t - u)) / a du
        static Rate alpha(
            Time t,
            Real a,
            const std::function<Real(Time)>& sigma,
            const Handle<YieldTermStructure>& h
        ) {
            Rate alfa = h->forwardRate(t, t, Continuous, NoFrequency);    // f(0, t): ZV instantaneous forward rate for forward time t
            Rate zv_drift = zv_drift_term(t, a, sigma);    // add the drift term to tho the ZV instantaneous forward rate
            alfa += zv_drift;
            return alfa;
        }
        // alpha(t) is E[r(t)]
        Rate alpha(Time t) const {
            return alphaInterp_(t);
        }
        // Hull-White model theta(t)
        // theta(t) = f'(0, t) + a * f(0, t) + integral_0^t sigma(u)^2 * exp(-2 * a * (t - u)) du
        // if sigma is constant, then integral_0^t sigma(u)^2 * exp(-2 * a * (t - u)) du = sigma^2/(2 * a) * (1 - exp(-2 * a * t))
        Real theta(Time t) const {
            Real a = a_;
            const auto& sigma = sigma_;
            auto drift_term = [&a, &sigma](Time t) {
                auto integrand = [&sigma, &t, &a](Time u) {
                    Volatility vol = sigma(u);
                    Real result = vol * vol * std::exp(-2.0 * a * (t - u));
                    return result;
                };
                SimpsonIntegral integrator(1e-12, 1000);
                Real mr = integrator(integrand, 0.0, t);
                return mr;
            };
            Real alpha_drift = drift_term(t);
            Real shift = 0.0001;
            Real f = h_->forwardRate(t, t, Continuous, NoFrequency);
            Real fup = h_->forwardRate(t + shift, t + shift, Continuous, NoFrequency);
            Real f_prime = (fup - f) / shift;
            alpha_drift += a_ * f + f_prime;
            return alpha_drift;
        }
        // Var[r(T)|r(t)] for T > t
        // Var[r(T)|r(t)] = integral_t^T sigma(u)^2 * exp(-2 * a * (T - u)) du
        static Real variance_t_T(
            Time t,
            Time T,
            Real a,
            std::function<Real(Time)> sigma
        ) {
            auto integrand = [&sigma, &T, &a](Time u) {
                Volatility vol = sigma(u);
                Real result = vol * vol * std::exp(-2.0 * a * (T - u));
                return result;
            };
            SimpsonIntegral integrator(1e-12, 1000);
            Real mr = integrator(integrand, t, T);
            return mr;
        }
        // Var[r(T)|r(t)] for T > t
        Real variance_t_T(
            Time t,
            Time T
        ) const {
            return variance_t_T(t, T, a_, sigma_);
        }

        Real x0() const override {
            return process_->x0();
        }
        Real drift(Time t, Real r) const override {
            return process_->drift(t, r) + theta(t);    // the process_->drift(t, r) would returns -a_ * r => this function returns [theta(t) - a_ * r], which is the drift of the Hull-White process
        }
        Real diffusion(Time t, Real r) const override {
            return process_->diffusion(t, r);    // process_->diffusion(t, r) returns sigma_(t), which is the diffusion of the Hull-White process
        }
        Real expectation(Time t0, Real r0, Time dt) const override {
            return process_->expectation(t0, r0, dt)
                + alpha(t0 + dt) - alpha(t0) * std::exp(-a_ * dt);
        }
        Real stdDeviation(Time t0, Real r0, Time dt) const override {
            return process_->stdDeviation(t0, r0, dt);
        }
        Real variance(Time t0, Real r0, Time dt) const override {
            return process_->variance(t0, r0, dt); // process_->variance(t0, r0, dt) returns 0.5*sigma_(t0)*sigma_(t0)/a_*(1.0 - std::exp(-2.0*a_*dt)), which is a close approximation of the variance of the Hull-White process over the interval [t0, t0 + dt]
        }
    };
}
