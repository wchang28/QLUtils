#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        template <
            Frequency UNIT = Monthly
        >
        class Ramp {
        public:
            struct segment {
                Real value_start;
                Real value_end;
                Size length;
                static Real get_x(
                    Size offset
                ) {
                    return (Real)offset / (Real)UNIT;
                }
                static Size get_offset(
                    Real x
                ) {
                    return static_cast<Size>(std::round(x * (Real)UNIT));
                }
                bool is_flat() const {
                    return (value_end == Null<Real>());
                }
                bool is_extrapolated() const {
                    return (length == Null<Size>());
                }
                bool is_trapezoidal() const {
                    return !is_flat();
                }
                void assertValid() const {
                    QL_ASSERT(value_start != Null<Real>(), "Invalid segment: value_start is null");
                    if (length == Null<Size>()) {
                        QL_ASSERT(value_end == Null<Real>(), "Invalid segment: value_end should be null for extrapolated segment");
                    }
                    else {
                        QL_ASSERT(length > 0, "Invalid segment: length should be positive");
                    }
                }
                segment(
                    Real value_start = 0.0,
                    Size length = Null<Size>(),
                    Real value_end = Null<Real>()
                ) : value_start(value_start), length(length), value_end(value_end)
                {
                    assertValid();
                }
                segment& operator = (
                    const segment& rhs
                ) {
                    rhs.assertValid();
                    this->length = rhs.length;
                    this->value_start = rhs.value_start;
                    this->value_end = rhs.value_end;
                    return *this;
                }
                segment(const segment& src) {
                    *this = src;
                }
                Real xWidth() const {
                    if (length == Null<Size>()) {
                        return Null<Real>();
                    }
                    else {
                        return get_x(length);
                    }
                }
                Real slope() const {
                    if (value_end == Null<Real>() || length == Null<Size>() || value_start == value_end)
                        return 0.0;
                    else {
                        return (value_end - value_start) / xWidth();
                    }
                }
                bool offsetIsInRange(
                    Size offset
                ) const {
                    return (offset >= 0) && (length == Null<Size>() ? true : offset < length);
                }
                bool xIsInRange(
                    Real x
                ) const {
                    return (x >= 0.0) && (xWidth() == Null<Real>() ? true : x < xWidth());
                }
                Real value_impl(Real x) const {
                    if (value_end == Null<Real>())    // flat or extrapolated
                        return value_start;
                    else {
                        auto slope = this->slope();
                        return value_start + slope * x;
                    }
                }
                std::string xRangeNotation() const {
                    if (xWidth() == Null<Real>()) {
                        return "[0, inf)";
                    }
                    else {
                        return "[0, " + std::to_string(xWidth()) + ")";
                    }
                }
                std::string offsetRangeNotation() const {
                    if (length == Null<Size>()) {
                        return "[0, inf)";
                    }
                    else {
                        return "[0, " + std::to_string(length) + ")";
                    }
                }
                Real value(
                    Real x
                ) const {
                    QL_REQUIRE(xIsInRange(x), "x (" << x << ") is out of range " << xRangeNotation());
                    return value_impl(x);
                }
                Real value_at(
                    Size offset
                ) const {
                    QL_REQUIRE(offsetIsInRange(offset), "offset (" << offset << ") is out of range " << offsetRangeNotation());
                    return value_impl(get_x(offset));
                }
                Real primitive(
                    Real x = Null<Real>()    // if null, calculate for the whole segment
                ) const {
                    if (x == Null<Real>()) {    // all area
                        if (xWidth() == Null<Real>()) // extrapolated
                            return Null<Real>();
                        else if (value_end == Null<Real>())    // flat
                            return value_start * xWidth();
                        else {    // ramp
                            return (value_start + value_end) * xWidth() * 0.5;
                        }
                    }
                    else {
                        QL_REQUIRE(x >= 0.0, "x (" << x << ") is negative");
                        if (xWidth() != Null<Real>()) {
                            QL_REQUIRE(x <= xWidth(), "x (" << x << ") must be less or equal to " << xWidth());
                        }
                        if (value_end == Null<Real>()) {    // flat
                            return value_start * x;
                        }
                        else {    // ramp
                            return value_start * x + 0.5 * slope() * x * x;
                        }
                    }
                }
                Real derivative(
                    Real x
                ) const {
                    QL_REQUIRE(xIsInRange(x), "x (" << x << ") is out of range " << xRangeNotation());
                    return slope();
                }
                Real secondDerivative(
                    Real x
                ) {
                    QL_REQUIRE(xIsInRange(x), "x (" << x << ") is out of range " << xRangeNotation());
                    return 0.0;
                }
                std::string to_string(
                    std::streamsize precision = 6
                ) const {
                    std::ostringstream oss;
                    oss << std::fixed;
                    oss << std::setprecision(precision) << value_start;
                    if (!is_extrapolated()) {
                        if (is_flat()) {
                            if (length > 1) {
                                oss << "/" << length;
                            }
                        }
                        else {
                            oss << "r" << length;
                        }
                    }
                    return oss.str();
                }
                operator std::string() const {
                    return to_string();
                }
                segment& operator += (
                    const Real& value
                ) {
                    this->value_start += value;
                    if (!this->is_flat()) {
                        this->value_end += value;
                    }
                    return *this;
                }
                segment operator + (
                    const Real& value
                ) const {
                    segment s(*this);
                    s += value;
                    return s;
                }
                segment& operator -= (
                    const Real& value
                ) {
                    this->value_start -= value;
                    if (!this->is_flat()) {
                        this->value_end -= value;
                    }
                    return *this;
                }
                segment operator - (
                    const Real& value
                ) const {
                    segment s(*this);
                    s -= value;
                    return s;
                }
                segment& operator *= (
                    const Real& value
                ) {
                    this->value_start *= value;
                    if (!this->is_flat()) {
                        this->value_end *= value;
                    }
                    return *this;
                }
                segment operator * (
                    const Real& value
                ) const {
                    segment s(*this);
                    s *= value;
                    return s;
                }
                segment& operator /= (
                    const Real& value
                ) {
                    this->value_start /= value;
                    if (!this->is_flat()) {
                        this->value_end /= value;
                    }
                    return *this;
                }
                segment operator / (
                    const Real& value
                ) const {
                    segment s(*this);
                    s /= value;
                    return s;
                }
            };
            using SegmentIndex = Size;
        protected:
            std::vector<segment> segments_;
        protected:
            std::pair<SegmentIndex, Size> get_segment_index_by_offset(
                Size offset
            ) const {
                QL_REQUIRE(offset >= 0, "offset (" << offset << ") is negative");
                SegmentIndex index = 0;
                Size accum = 0;
                while (true) {
                    const auto& segment = segments_[index];
                    if (segment.is_extrapolated()) {
                        break;
                    }
                    else {
                        if (offset < accum + segment.length) {
                            break;
                        }
                        else {
                            accum += segment.length;
                            index++;
                        }
                    }
                }
                return std::pair<SegmentIndex, Size>(index, offset - accum);
            }
            std::pair<SegmentIndex, Real> get_segment_index_by_x(
                Real x
            )  const {
                QL_REQUIRE(x >= 0.0, "x (" << x << ") is negative");
                SegmentIndex index = 0;
                Real accum = 0.0;
                while (true) {
                    const auto& segment = segments_[index];
                    if (segment.is_extrapolated()) {
                        break;
                    }
                    else {
                        if (x < accum + segment.xWidth()) {
                            break;
                        }
                        else {
                            accum += segment.xWidth();
                            index++;
                        }
                    }
                }
                return std::pair<SegmentIndex, Real>(index, x - accum);
            }
            // Trim from left (beginning)
            static void ltrim(std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
            }
            // Trim from right (end)
            static void rtrim(std::string& s) {
                s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
            }
            // Trim from both ends
            static void trim(std::string& s) {
                ltrim(s);
                rtrim(s);
            }
            // Split a string by comma and trim each part
            static std::vector<std::string> splitCSV(
                const std::string csv
            ) {
                std::vector<std::string> result;
                std::istringstream ss(csv);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    trim(item);
                    result.push_back(item);
                }
                return result;
            }
            bool is_double(
                const std::string& str
            ) {
                try {
                    size_t pos;
                    auto value = std::stod(str, &pos); // Try converting string to double
                    return pos == str.size(); // Ensure the ENTIRE string was consumed
                }
                catch (...) {
                    return false; // Throws std::invalid_argument or std::out_of_range
                }
            }
            bool is_positive_Integer(
                const std::string& s
            ) {
                return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
            }
            Real parse_for_value(
                const std::string& s,
                Real defaultValue = 0.0
            ) {
                Real value_start = Null<Real>();
                if (s.empty()) {
                    return defaultValue;
                }
                else {
                    QL_REQUIRE(is_double(s), "Invalid double/flaot value: " << s);
                    return std::stod(s);
                }
            }
            Size parse_for_length(
                const std::string& s,
                Size defaultValue = 1
            ) {
                Real value_start = Null<Real>();
                if (s.empty()) {
                    return defaultValue;
                }
                else {
                    QL_REQUIRE(is_positive_Integer(s), "Invalid positive integer value: " << s);
                    return static_cast<Size>(std::stoul(s));
                }
            }
            void parseNotation(std::string notation) {
                segments_.clear();
                trim(notation);
                if (notation.empty()) {
                    return;
                }
                auto parts = splitCSV(notation);
                if (parts.empty()) {
                    return;
                }
                Size n = parts.size();    // n > 0
                for (Size i = 0; i < n; ++i) {
                    auto last = (i == n - 1);
                    auto part = parts[i];
                    if (part.empty()) {
                        part = "0";
                    }
                    if (last) {    // the last segment
                        try {
                            auto value_start = parse_for_value(part);
                            segments_.emplace_back(value_start);
                        }
                        catch (const std::exception& e) {
                            QL_FAIL("Invalid notation for the last segment: " << part << ". Error: " << e.what());
                        }
                    }
                    else {    // not a last segment, must have length
                        size_t pos = part.find('/');
                        if (pos != std::string::npos) {    // flat segment with length, e.g. "2/12"
                            auto part_1 = part.substr(0, pos);
                            auto part_2 = part.substr(pos + 1);
                            auto value_start = parse_for_value(part_1);
                            auto length = parse_for_length(part_2);
                            segments_.emplace_back(value_start, length);
                        }
                        else {
                            pos = part.find('r');
                            if (pos != std::string::npos) {    // ramp/trapezoidal segment, e.g. "2r12"
                                auto part_1 = part.substr(0, pos);
                                auto part_2 = part.substr(pos + 1);
                                auto value_start = parse_for_value(part_1);
                                auto length = parse_for_length(part_2);
                                segments_.emplace_back(value_start, length, 0.0);
                            }
                            else {    // flat segment without length, e.g. "2"
                                auto value_start = parse_for_value(part);
                                segments_.emplace_back(value_start, 1);
                            }
                        }
                    }
                }
                QL_ASSERT(segments_.size() == n, "Invalid state: segments size (" << segments_.size() << " ) does not match parts size (" << n << ")");
                for (Size i = 0; i < n - 1; i++) {
                    if (segments_[i].is_trapezoidal()) {
                        segments_[i].value_end = segments_[i + 1].value_start;
                    }
                }
            }
            void assertNotEmpty() const {
                QL_ASSERT(!empty(), "Ramp is empty");
            }
        public:
            Ramp() {}
            Ramp(
                const std::string& notation
            ) {
                parseNotation(notation);
            }
            bool empty() const {
                return segments_.empty();
            }
            bool is_flat() const {
                if (empty()) {
                    return false;
                }
                else {
                    Real flat_value = Null<Real>();
                    for (const auto& segment : segments_) {
                        if (!segment.is_flat()) {
                            return false;
                        }
                        else {    // flat segment
                            if (flat_value == Null<Real>()) {
                                flat_value = segment.value_start;
                            }
                            else if (flat_value != segment.value_start) {
                                return false;
                            }
                        }
                    }
                    QL_ASSERT(flat_value != Null<Real>(), "Invalid state: flat_value is null");
                    return true;
                }
            }
            Ramp& operator = (
                const std::string& rhs
            ) {
                parseNotation(rhs);
                return *this;
            }
            Real operator [] (
                Size offset
            ) const {
                assertNotEmpty();
                auto pr = get_segment_index_by_offset(offset);
                return segments_[pr.first].value_at(pr.second);
            }
            Real operator () (
                Size offset
            ) const {
                return this->operator[](offset);
            }
            Real value_at(
                Size offset
            ) const {
                return this->operator[](offset);
            }
            Size num_intervals() const {
                return segments_.size();
            }
            Size ramp_length() const {
                Size len = 0;
                for (const auto& segment : segments_) {
                    if (!segment.is_extrapolated()) {
                        len += segment.length;
                    }
                }
                return len;
            }
            Real xWidth() const {
                auto len = ramp_length();
                return segment::get_x(len);
            }
            const Real& left_value() const {
                assertNotEmpty();
                return segments_.front().value_start;
            }
            const Real& right_value() const {
                assertNotEmpty();
                return segments_.back().value_start;
            }
            Real value(Real x) const {
                assertNotEmpty();
                const auto& [index, x_offset] = get_segment_index_by_x(x);
                return segments_[index].value(x_offset);
            }
            Real primitive(Real x) const {
                assertNotEmpty();
                const auto& [index, x_offset] = get_segment_index_by_x(x);
                Real primitive = segments_[index].primitive(x_offset);
                for (Size i = 0; i < index; ++i) {
                    primitive += segments_[i].primitive();
                }
                return primitive;
            }
            Real derivative(Real x) const {
                assertNotEmpty();
                const auto& [index, x_offset] = get_segment_index_by_x(x);
                return segments_[index].derivative(x_offset);
            }
            Real secondDerivative(Real x) const {
                assertNotEmpty();
                const auto& [index, x_offset] = get_segment_index_by_x(x);
                return segments_[index].secondDerivative(x_offset);
            }
            std::string notation(
                std::streamsize precision = 6
            ) const {
                if (empty()) {
                    return "";
                }
                else {
                    std::ostringstream oss;
                    Size n = segments_.size();
                    for (Size i = 0; i < n; ++i) {
                        const auto& segment = segments_[i];
                        if (i > 0) {
                            oss << ",";
                        }
                        oss << segment.to_string(precision);
                    }
                    return oss.str();
                }
            }
            std::string to_string(
                std::streamsize precision = 6
            ) const {
                return notation(precision);
            }
            operator std::string() const {
                return to_string();
            }
            Ramp& operator += (
                const Real& value
            ) {
                if (empty()) {
                    segments_.emplace_back(0.0);
                }
                for (auto& segment : segments_) {
                    segment += value;
                }
                return *this;
            }
            Ramp& operator -= (
                const Real& value
            ) {
                if (empty()) {
                    segments_.emplace_back(0.0);
                }
                for (auto& segment : segments_) {
                    segment -= value;
                }
                return *this;
            }
            Ramp& operator *= (
                const Real& value
            ) {
                if (empty()) {
                    segments_.emplace_back(0.0);
                }
                for (auto& segment : segments_) {
                    segment *= value;
                }
                return *this;
            }
            Ramp& operator /= (
                const Real& value
            ) {
                if (empty()) {
                    segments_.emplace_back(0.0);
                }
                for (auto& segment : segments_) {
                    segment /= value;
                }
                return *this;
            }
            Ramp operator + (
                const Real& value
            ) const {
                Ramp r(*this);
                r += value;
                return r;
            }
            Ramp operator - (
                const Real& value
            ) const {
                Ramp r(*this);
                r -= value;
                return r;
            }
            Ramp operator * (
                const Real& value
            ) const {
                Ramp r(*this);
                r *= value;
                return r;
            }
            Ramp operator / (
                const Real& value
            ) const {
                Ramp r(*this);
                r /= value;
                return r;
            }
        };
    }
}
