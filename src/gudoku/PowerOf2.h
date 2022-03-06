
#ifndef GUDOKU_SUPPORT_POWEROF2_H
#define GUDOKU_SUPPORT_POWEROF2_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>      // For std::size_t
#include <cassert>
#include <type_traits>
#include <limits>       // For std::numeric_limits<T>::max()

//////////////////////////////////////////////////////////////////////////
//
// Bit Twiddling Hacks
//
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
//
//////////////////////////////////////////////////////////////////////////

namespace gudoku {

template <typename T>
struct integral_traits {
    typedef typename std::make_signed<T>::type      signed_type;
    typedef typename std::make_unsigned<T>::type    unsigned_type;

    static_assert(std::is_integral<T>::value,
        "Error: jstd::integral_traits<T> -- T must be a integral type.");

    // Bits
    static const size_t bytes = sizeof(T);
    static const size_t bits = bytes * 8;
    static const size_t max_shift = bits - 1;

    // 0xFFFFFFFFUL;
    static const unsigned_type max_num = static_cast<unsigned_type>(-1);
    // 0x80000000UL;
    static const unsigned_type max_power2 = static_cast<unsigned_type>(1) << max_shift;
};

} // namespace gzSudoku

//////////////////////////////////////////////////////////////////////////////////
//
// Msvc compiler: Maxinum recursive depth is about 497 layer.
//
//////////////////////////////////////////////////////////////////////////////////

namespace gzSudoku {
namespace compile_time {

//////////////////////////////////////////////////////////////////////////////////

//
// is_power2 = (N && ((N & (N - 1)) == 0);
// Here, N must be a unsigned number.
//
template <std::size_t N>
struct is_power2 {
    static const bool value = ((N & (N - 1)) == 0);
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_to_pow2_impl {
    static const std::size_t max_power2 = integral_traits<std::size_t>::max_power2;
    static const std::size_t max_num = integral_traits<std::size_t>::max_num;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N > max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N >= max_power2) ? max_power2 :
           (((Power2 == max_power2) || (Power2 >= N)) ? (Power2 / 2) :
            round_to_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct round_to_pow2_impl<N, 0> {
    static const std::size_t value = integral_traits<size_t>::max_power2;
};

template <std::size_t N>
struct round_to_pow2 {
    static const std::size_t value = is_power2<N>::value ? N : round_to_pow2_impl<N, 1>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_down_to_pow2<N>

template <std::size_t N>
struct round_down_to_pow2 {
    static const std::size_t value = (N != 0) ? round_to_pow2<N - 1>::value : 0;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_up_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_up_to_pow2_impl {
    static const std::size_t max_power2 = integral_traits<std::size_t>::max_power2;
    static const std::size_t max_num = integral_traits<std::size_t>::max_num;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N > max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 >= N)) ? Power2 :
            round_up_to_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct round_up_to_pow2_impl<N, 0> {
    static const std::size_t value = integral_traits<std::size_t>::max_num;
};

template <std::size_t N>
struct round_up_to_pow2 {
    static const std::size_t value = is_power2<N>::value ? N : round_up_to_pow2_impl<N, 1>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct next_pow2<N>

template <std::size_t N, std::size_t Power2>
struct next_pow2_impl {
    static const std::size_t max_power2 = integral_traits<std::size_t>::max_power2;
    static const std::size_t max_num = integral_traits<std::size_t>::max_num;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N >= max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 > N)) ? Power2 :
            next_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct next_pow2_impl<N, 0> {
    static const std::size_t value = integral_traits<size_t>::max_num;
};

template <std::size_t N>
struct next_pow2 {
    static const std::size_t value = next_pow2_impl<N, 1>::value;
};

template <>
struct next_pow2<0> {
    static const std::size_t value = 1;
};

//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4307)
#endif

//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////

// struct round_to_power2_impl<N>

template <std::size_t N>
struct round_to_power2_impl {
    static const std::size_t max_num = integral_traits<std::size_t>::max_num;
    static const std::size_t max_power2 = integral_traits<std::size_t>::max_power2;
    static const std::size_t N1 = N - 1;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = (N7 != max_num) ? ((N7 + 1) / 2) : max_power2;
#else
    static const std::size_t value = (N6 != max_num) ? ((N6 + 1) / 2) : max_power2;
#endif
};

template <std::size_t N>
struct round_to_power2 {
    static const std::size_t value = is_power2<N>::value ? N : round_to_power2_impl<N>::value;
};

// struct round_down_to_power2<N>

template <std::size_t N>
struct round_down_to_power2 {
    static const std::size_t value = (N != 0) ? round_to_power2<N - 1>::value : 0;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_up_to_power2<N>

template <std::size_t N>
struct round_up_to_power2_impl {
    static const std::size_t max_num = integral_traits<std::size_t>::max_num;
    static const std::size_t N1 = N - 1;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = (N7 != max_num) ? (N7 + 1) : max_num;
#else
    static const std::size_t value = (N6 != max_num) ? (N6 + 1) : max_num;
#endif
};

template <std::size_t N>
struct round_up_to_power2 {
    static const std::size_t value = is_power2<N>::value ? N : round_up_to_power2_impl<N>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct next_power2<N>

template <std::size_t N>
struct next_power2 {
    static const std::size_t max_num = integral_traits<std::size_t>::max_num;
    static const std::size_t value = (N < max_num) ? round_up_to_power2<N + 1>::value : max_num;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

//////////////////////////////////////////////////////////////////////////////////

} // namespace compile_time
} // namespace gudoku

#endif // GUDOKU_SUPPORT_POWEROF2_H
