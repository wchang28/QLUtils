#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace detail {
        struct basis_point_holder {
            explicit basis_point_holder(Real value) : value(value) {}
            Real value;
        };
        inline std::ostream& operator<<(std::ostream& out, const basis_point_holder& holder) {
            std::ios::fmtflags flags = out.flags();
            Size width = (Size)out.width();
            if (width > 3)
                out.width(width - 3); // eat space used by percent sign
            out << std::fixed;
            if (holder.value == Null<Real>())
                out << "null";
            else
                out << holder.value * 10000.0 << " bp";
            out.flags(flags);
            return out;
        }
    }
    namespace io {
        inline detail::basis_point_holder basis_point(Spread s) {
            return detail::basis_point_holder(s);
        }
        inline detail::basis_point_holder normal_volatility(Volatility v) {
            return detail::basis_point_holder(v);
        }
        inline detail::basis_point_holder oas(Spread s) {
            return detail::basis_point_holder(s);
        }
        inline detail::basis_point_holder z_spread(Spread s) {
            return detail::basis_point_holder(s);
        }
        inline detail::basis_point_holder e_spread(Spread s) {
            return detail::basis_point_holder(s);
        }
        inline detail::basis_point_holder i_spread(Spread s) {
            return detail::basis_point_holder(s);
        }
        inline detail::basis_point_holder discount_margin(Spread s) {
            return detail::basis_point_holder(s);
        }
    }
}