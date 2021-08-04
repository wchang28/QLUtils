#pragma once

#include <ql/quantlib.hpp>
#include <boost/lexical_cast.hpp>
#include <ql_utils/types.hpp>

namespace QLUtils {
    template <typename _Elem = char>
    struct DateFormat {
        static QuantLib::Date from_yyyymmdd(const string_t<_Elem>& yyyymmdd, bool hasHyphen = false) {
            auto yyyy = yyyymmdd.substr(0, 4);
            auto mm = yyyymmdd.substr(4 + (std::size_t)(hasHyphen ? 1 : 0), 2);
            auto dd = yyyymmdd.substr(6 + (std::size_t)(hasHyphen ? 2 : 0));
            auto year = boost::lexical_cast<QuantLib::Year>(yyyy);
            auto month = QuantLib::Month(boost::lexical_cast<int>(mm));
            auto day = boost::lexical_cast<QuantLib::Day>(dd);
            return QuantLib::Date(day, month, year);
        }
        static string_t<_Elem> to_yyyymmdd(const QuantLib::Date& d, bool hyphen = false) {
            auto d_i = d.year() * 10000 + int(d.month()) * 100 + d.dayOfMonth();
            auto s = boost::lexical_cast<string_t<_Elem>>(d_i);
            if (hyphen) {
                auto yyyy = s.substr(0, 4);
                auto mm = s.substr(4, 2);
                auto dd = s.substr(6);
                ostringstream_t<_Elem> os;
                os << yyyy << "-" << mm << "-" << dd;
                return os.str();
            }
            else {
                return s;
            }
        }
    };
}