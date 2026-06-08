#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/termstructures/yield/interpolatedsimplezerocurve.hpp>

namespace QuantLib {
    namespace Utils {
        namespace BugFix {
            //! Simple Zero-curve traits
            struct SimpleZeroYield {
                // interpolated curve type
                template <class Interpolator>
                struct curve {
                    typedef BugFix::InterpolatedSimpleZeroCurve<Interpolator> type;
                };
                // helper class
                typedef BootstrapHelper<YieldTermStructure> helper;

                // start of curve data
                static Date initialDate(const YieldTermStructure* c) {
                    return c->referenceDate();
                }
                // dummy value at reference date
                static Real initialValue(const YieldTermStructure*) {
                    return detail::avgRate;
                }

                // guesses
                template <class C>
                static Real guess(Size i,
                                  const C* c,
                                  bool validData,
                                  Size) // firstAliveHelper
                {
                    if (validData) // previous iteration value
                        return c->data()[i];

                    if (i==1) // first pillar
                        return detail::avgRate;

                    // extrapolate
                    Date d = c->dates()[i];
                    return c->zeroRate(d, c->dayCounter(),
                                       Simple, Annual, true);
                }

                // possible constraints based on previous values
                template <class C>
                static Real minValueAfter(Size i,
                                          const C* c,
                                          bool validData,
                                          Size) // firstAliveHelper
                {
                    Real result;
                    if (validData) {
                        Real r = *(std::min_element(c->data().begin(), c->data().end()));
                        result = r<0.0 ? Real(r*2.0) : r/2.0;
                    } else {
                        // no constraints.
                        // We choose as min a value very unlikely to be exceeded.
                        result = -detail::maxRate;
                    }
                    return std::max(result, -1.0 / c->times()[i] + 1E-8);
                }
                template <class C>
                static Real maxValueAfter(Size,
                                          const C* c,
                                          bool validData,
                                          Size) // firstAliveHelper
                {
                    if (validData) {
                        Real r = *(std::max_element(c->data().begin(), c->data().end()));
                        return r<0.0 ? Real(r/2.0) : r*2.0;
                    }
                    // no constraints.
                    // We choose as max a value very unlikely to be exceeded.
                    return detail::maxRate;
                }

                // transformation to add constraints to an unconstrained optimization
                template <class C>
                static Real transformDirect(Real x, Size i, const C* c)
                {
                    return std::exp(x) + (-1.0 / c->times()[i] + 1E-8);
                }
                template <class C>
                static Real transformInverse(Real x, Size i, const C* c)
                {
                    return std::log(x - (-1.0 / c->times()[i] + 1E-8));
                }

                // root-finding update
                static void updateGuess(std::vector<Real>& data,
                                        Real rate,
                                        Size i) {
                    data[i] = rate;
                    if (i==1)
                        data[0] = rate; // first point is updated as well
                }
                // upper bound for convergence loop
                static Size maxIterations() { return 100; }
            };
        }   // namespace BugFix
    }   // namespace Utils
}   // namespace QuantLib
