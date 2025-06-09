/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2005, 2006 Klaus Spanderen

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file lmfixedvolmodel.hpp
    \brief model of constant volatilities for libor market models
*/

#ifndef quantlib_libor_market_fixed_volatility_model_fixed_hpp
#define quantlib_libor_market_fixed_volatility_model_fixed_hpp

#include <ql/legacy/libormarketmodels/lmvolmodel.hpp>
#include <utility>
#include <algorithm>

namespace QuantLib {

    class LmFixedVolatilityModelFixed : public LmVolatilityModel {
      public:
        LmFixedVolatilityModelFixed(
            const std::vector<Time>& fixingTimes,
            std::vector<Volatility> volatilities
        ): LmVolatilityModel(fixingTimes.size(), 0), volatilities_(std::move(volatilities)), fixingTimes_(fixingTimes) {
            QL_REQUIRE(fixingTimes_.size()>1, "too few dates");
            QL_REQUIRE(volatilities_.size() == fixingTimes_.size(),
                   "volatility array and fixing time array have to have "
                   "the same size");
            for (Size i = 1; i < fixingTimes_.size(); i++) {
                QL_REQUIRE(fixingTimes_[i] > fixingTimes_[i-1], "invalid time (" << fixingTimes_[i] << ", vs " << fixingTimes_[i-1] << ")");
            }
            QL_REQUIRE(volatilities_[0] == 0.0, "volatilities[0] (" << volatilities_[0] << ") must be zero");
        }

        Array volatility(Time t, const Array& x = {}) const override {
            QL_REQUIRE(t >= fixingTimes_.front() && t <= fixingTimes_.back(),
                       "invalid time given for volatility model");
            auto less_than_or_equal_to = [](const Time& t, const Time& t_i) -> bool {return t <= t_i; };
            const Size ti = std::upper_bound(fixingTimes_.begin(), fixingTimes_.end(), t, less_than_or_equal_to) - fixingTimes_.begin();

            Array tmp(size_, 0.0);

            for (Size i=ti; i<size_; ++i) {
                tmp[i] = volatilities_[i-ti];
            }

            return tmp;
        }
        Volatility volatility(Size i, Time t, const Array& x = {}) const override {
            QL_REQUIRE(t >= fixingTimes_.front() && t <= fixingTimes_.back(),
                   "invalid time given for volatility model");
            auto less_than_or_equal_to = [](const Time& t, const Time& t_i) -> bool {return t <= t_i; };
            const Size ti = std::upper_bound(fixingTimes_.begin(), fixingTimes_.end(), t, less_than_or_equal_to) - fixingTimes_.begin();

            return (i >= ti ? volatilities_[i-ti] : Volatility(0.0));
        }

      private:
        void generateArguments() override {}

        const std::vector<Time> fixingTimes_;
        const std::vector<Volatility> volatilities_;
    };

}


#endif
