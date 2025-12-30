/* Copyright (C) 2021 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once

#ifndef TWS_API_CLIENT_DECIMAL_H
#define TWS_API_CLIENT_DECIMAL_H

#include <boost/multiprecision/cpp_dec_float.hpp>

// 使用 16 位精度
typedef boost::multiprecision::number<boost::multiprecision::cpp_dec_float<16>> Decimal;

// 定义 UNSET 常量
const Decimal UNSET_DECIMAL("1.7976931348623157E308");

// 核心运算逻辑（去掉了判断，由 Boost 自动处理）
inline Decimal add(Decimal d1, Decimal d2) { return d1 + d2; }
inline Decimal sub(Decimal d1, Decimal d2) { return d1 - d2; }
inline Decimal mul(Decimal d1, Decimal d2) { return d1 * d2; }
inline Decimal div(Decimal d1, Decimal d2) { return (d2 == 0) ? Decimal(0) : (d1 / d2); }

// 转换逻辑
inline double decimalToDouble(Decimal d) { return d.convert_to<double>(); }
inline Decimal doubleToDecimal(double d) { return Decimal(d); }

inline Decimal stringToDecimal(std::string str) {
    if (str.empty() || str == "2147483647" || str == "9223372036854775807" || str == "1.7976931348623157E308")
        return UNSET_DECIMAL;
    try { return Decimal(str); } catch (...) { return Decimal(0); }
}

inline std::string decimalToString(Decimal v) {
    return (v == UNSET_DECIMAL) ? "" : v.str();
}

// 这个函数在原版中就很复杂，新版其实已经比原版简单了
inline std::string decimalStringToDisplay(Decimal v) {
    if (v == UNSET_DECIMAL) return "";
    // 直接利用 Boost 的精度格式化功能
    std::stringstream ss;
    ss << std::setprecision(16) << std::noshowpoint << v;
    return ss.str();
}

inline Decimal decimal_from_int(int x)
{
    // 直接利用 Boost 的构造函数将 int 转换为 Decimal
    return Decimal(x);
}

inline int decimal_to_int(Decimal x)
{
    // 直接利用 Boost 的转换函数
    // 如果 x 的值超过了 int 的表达范围，Boost 默认会抛出异常或根据配置处理
    return x.convert_to<int>();
}
inline double decimal_To_double(Decimal decimal)
{
    // 直接利用 Boost 的转换函数将十进制高精度类型转为标准的 double (binary64)
    // 这样做不再需要外部的 __bid64_to_binary64 函数，也不再需要处理 flags 标志位
    return decimal.convert_to<double>();
}

#endif




// 以下是原来IBKR提供的Decimal实现，使用了IntelRDFPMathLib，现在用自己的实现，使用boost把它换掉
/*
#ifndef TWS_API_CLIENT_DECIMAL_H
#define TWS_API_CLIENT_DECIMAL_H

#include <sstream>
#include <climits>

// Decimal type
typedef unsigned long long Decimal;

#define UNSET_DECIMAL ULLONG_MAX

// external functions
extern "C" Decimal __bid64_add(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_sub(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_mul(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_div(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_from_string(char*, unsigned int, unsigned int*);
extern "C" void __bid64_to_string(char*, Decimal, unsigned int*);
extern "C" double __bid64_to_binary64(Decimal, unsigned int, unsigned int*);
extern "C" Decimal __binary64_to_bid64(double, unsigned int, unsigned int*);

// inline functions
inline Decimal add(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_add(decimal1, decimal2, 0, &flags);
}

inline Decimal sub(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_sub(decimal1, decimal2, 0, &flags);
}

inline Decimal mul(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_mul(decimal1, decimal2, 0, &flags);
}

inline Decimal div(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_div(decimal1, decimal2, 0, &flags);
}

inline double decimalToDouble(Decimal decimal) {
    unsigned int flags;
    return __bid64_to_binary64(decimal, 0, &flags);
}

inline Decimal doubleToDecimal(double d) {
    unsigned int flags;
    return __binary64_to_bid64(d, 0, &flags);
}

inline Decimal stringToDecimal(std::string str) {
    unsigned int flags;
    if (str.compare(std::string{ "2147483647" }) == 0 || str.compare(std::string{ "9223372036854775807" }) == 0 || str.compare(std::string{ "1.7976931348623157E308" }) == 0) {
        str.clear();
    }
    return __bid64_from_string(const_cast<char*>(str.c_str()), 0, &flags);
}

inline std::string decimalToString(Decimal value) {
    char buf[64];
    unsigned int flags;
    __bid64_to_string(buf, value, &flags); // convert Decimal value to string using bid64_to_string function
    return buf;
}

inline std::string decimalStringToDisplay(Decimal value) {
    // convert string with scientific notation to string with decimal notation (e.g. +1E-2 to 0.01)
    std::string tempStr = decimalToString(value);

    if (tempStr.compare(std::string{ "+NaN" }) == 0 || tempStr.compare(std::string{ "-SNaN" }) == 0) {
        return ""; // if is invalid, then return empty string
    }

    int expPos = tempStr.find("E"); // find position of 'E' char (e.g. 2)
    if (expPos < 0) {
        return tempStr; // if 'E' char is missing, then return string as-is
    }
    std::string expStr = tempStr.substr(expPos); // extract exp string (e.g. E-2)

    // calculate exp
    int exp = 0;
    for (unsigned int i = 2; i < expStr.size(); i++) {
        exp = exp * 10 + (expStr[i] - '0');
    }
    if (expStr[1] == '-') {
        exp *= -1;
    }
    int numLength = tempStr.size() - expStr.size() - 1; // length of numbers substring

    // check sign
    bool isNegative = false;
    if (tempStr.substr(0, 1).compare(std::string{ "-" }) == 0) { 
        isNegative = true;
    }

    std::string numbers = tempStr.substr(1, numLength); // extract numbers (e.g. 1)
    if (exp == 0) {
        return isNegative ? '-' + numbers : numbers; // if exp is zero, then return numbers as-is
    }

    std::string result = isNegative ? "-" : "";
    bool decPtAdded = false;

    // add zero(s) and decimal point
    for (int i = numLength; i <= (-exp); i++) {
        result += '0';
        if (i == numLength) {
            result += '.';
            decPtAdded = true;
        }
    }

    // add numbers and decimal point
    for (int i = 0; i < numLength; i++) {
        if (numLength - i == (-exp) && !decPtAdded) {
            result += '.';
        }
        result += numbers[i];
    }
    return result;
}

#endif
*/