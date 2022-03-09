
#ifndef GUDOKU_BITUTILS_H
#define GUDOKU_BITUTILS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef USE_STD_INT
#define USE_STD_INT     1
#endif

#if USE_STD_INT
#include <stdint.h>
#include <stddef.h>
#endif
#include <assert.h>

#if (defined(_MSC_VER) && (_MSC_VER >= 1500)) || defined(__MINGW32__) || defined(__CYGWIN__)
	#include <intrin.h>
#endif

#if (defined(_MSC_VER))
    #include <nmmintrin.h>      // For SSE 4.2, _mm_popcnt_u32(), _mm_popcnt_u64()
#endif

#if (defined(__GNUC__) && (__GNUC__ * 1000 + __GNUC_MINOR__ >= 4005)) \
 || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 900)) || defined(__clang__)
	#include <x86intrin.h>
#endif

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
  #define JSTD_IS_X86       1
  #define JSTD_IS_X86_64    1
  #define JSTD_WORD_SIZE    64
#else
  #if defined(WIN32) || defined(_WIN32) || defined (_M_IX86) || defined(__i386__)
    #define JSTD_IS_X86     1
    #define JSTD_IS_X86_32  1
  #endif
  #define JSTD_WORD_SIZE    32
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1500) // >= VC 2008
    #pragma intrinsic(_BitScanReverse)
    #pragma intrinsic(_BitScanForward)
    #if defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64)
        #pragma intrinsic(_BitScanReverse64)
        #pragma intrinsic(_BitScanForward64)
    #endif
#endif // (_MSC_VER && _MSC_VER >= 1500)

namespace gudoku {

#if (USE_STD_INT == 0)
#if (defined(_MSC_VER) && (_MSC_VER >= 1500)) || defined(__MINGW32__) || defined(__CYGWIN__)
typedef __int64             int64_t;
typedef unsigned __int64    uint64_t;
#else
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
#endif // (_MSC_VER && _MSC_VER >= 1500)

#if (JSTD_WORD_SIZE == 64)
typedef int64_t         ssize_t;
typedef uint64_t        size_t;
#else
typedef int             ssize_t;
typedef unsigned int    size_t;
#endif
#endif // (USE_STD_INT == 0)

struct BitUtils {

    //
    // popcount() algorithm
    //
    // See: http://www.cnblogs.com/Martinium/articles/popcount.html
    // See: https://en.wikipedia.org/wiki/Hamming_weight
    // See: https://stackoverflow.com/questions/757059/position-of-least-significant-bit-that-is-set
    //

    static inline
    unsigned int __internal_popcnt(unsigned int x) { 
        x -=  ((x >> 1U) & 0x55555555U);
        x  = (((x >> 2U) & 0x33333333U) + (x & 0x33333333U));
        x  = (((x >> 4U) + x) & 0x0F0F0F0FU);
        x +=   (x >> 8U);
        x +=   (x >> 16U);
        x  = x & 0x0000003FU;
        assert(x >= 0 && x <= 32);
        return x;
    }

    static inline
    unsigned int __internal_popcnt_slow(unsigned int x) {
        x = (x & 0x55555555UL) + ((x >>  1U) & 0x55555555UL);
        x = (x & 0x33333333UL) + ((x >>  2U) & 0x33333333UL);
        x = (x & 0x0F0F0F0FUL) + ((x >>  4U) & 0x0F0F0F0FUL);
        x = (x & 0x00FF00FFUL) + ((x >>  8U) & 0x00FF00FFUL);
        x = (x & 0x0000FFFFUL) + ((x >> 16U) & 0x0000FFFFUL);
        assert(x >= 0 && x <= 32);
        return x;
    }

    static inline
    unsigned int __internal_hakmem_popcnt(unsigned int x) {
        unsigned int tmp;
        tmp = x - ((x >> 1) & 033333333333) - ((x >> 2) & 011111111111);
        return (((tmp + (tmp >> 3)) & 030707070707) % 63U);
    }

    static inline
    unsigned int __internal_popcnt64(uint64_t x) {
    #if 1
        x -=  ((x >> 1U) & 0x55555555U);
        x  = (((x >> 2U) & 0x33333333U) + (x & 0x33333333U));
        x  = (((x >> 4U) + x) & 0x0F0F0F0FU);
        x +=   (x >> 8U);
        x +=   (x >> 16U);
        x +=   (x >> 32U);
        x  = x & 0x0000007FU;
        assert(x >= 0 && x <= 64);
        return (unsigned int)x;
    #elif 0
        x = (x & 0x5555555555555555ULL) + ((x >>  1U) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >>  2U) & 0x3333333333333333ULL);
        x = (x & 0x0F0F0F0F0F0F0F0FULL) + ((x >>  4U) & 0x0F0F0F0F0F0F0F0FULL);
        x = (x & 0x00FF00FF00FF00FFULL) + ((x >>  8U) & 0x00FF00FF00FF00FFULL);
        x = (x & 0x0000FFFF0000FFFFULL) + ((x >> 16U) & 0x0000FFFF0000FFFFULL);
        x = (x & 0x00000000FFFFFFFFULL) + ((x >> 32U) & 0x00000000FFFFFFFFULL);
        assert(x >= 0 && x <= 64);
        return (unsigned int)x;
    #else
        unsigned int high, low;
        unsigned int n1, n2;
        high = (unsigned int) (x & 0x00000000FFFFFFFFULL);
        low  = (unsigned int)((x & 0xFFFFFFFF00000000ULL) >> 32U);
        n1 = __internal_popcnt(high);
        n2 = __internal_popcnt(low);
        return (n1 + n2);
    #endif
    }

    static inline
    unsigned int __internal_clz(unsigned int x) {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return (unsigned int)(32U - __internal_popcnt(x));
    }

    static inline
    unsigned int __internal_clzll(uint64_t x) {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        x |= (x >> 32);
        return (unsigned int)(64U - __internal_popcnt64(x));
    }

    static inline
    unsigned int __internal_ctz(unsigned int x) {
        return __internal_popcnt((x & -(int)x) - 1);
    }

    static inline
    unsigned int __internal_ctzll(uint64_t x) {
        return __internal_popcnt64((x & -(int64_t)x) - 1);
    }

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4146)
#endif

    //
    // The least significant 1 bit (LSB)
    //
    static inline uint32_t ls1b32(uint32_t x) {
        return (x & (uint32_t)-x);
    }

    static inline uint64_t ls1b64(uint64_t x) {
        return (x & (uint64_t)-x);
    }

    static inline size_t ls1b(size_t x) {
        return (x & (size_t)-x);
    }

    //
    // The most significant 1 bit (MSB)
    //

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#if (defined(_MSC_VER) && (_MSC_VER >= 1500)) || defined(__MINGW32__) || defined(__CYGWIN__)

    static inline
    unsigned int bsf32(unsigned int x) {
        assert(x != 0);
        unsigned long index;
        ::_BitScanForward(&index, (unsigned long)x);
        return (unsigned int)index;
    }

#if (JSTD_WORD_SIZE == 64)
    static inline
    unsigned int bsf64(unsigned __int64 x) {
        assert(x != 0);
        unsigned long index;
        ::_BitScanForward64(&index, x);
        return (unsigned int)index;
    }
#endif

    static inline
    unsigned int bsr32(unsigned int x) {
        assert(x != 0);
        unsigned long index;
        ::_BitScanReverse(&index, (unsigned long)x);
        return (unsigned int)index;
    }

#if (JSTD_WORD_SIZE == 64)
    static inline
    unsigned int bsr64(unsigned __int64 x) {
        assert(x != 0);
        unsigned long index;
        ::_BitScanReverse64(&index, x);
        return (unsigned int)index;
    }
#endif

    static inline
    unsigned int popcnt32(unsigned int x) {
#if defined(__POPCNT__)
        int popcount = _mm_popcnt_u32(x);
        return (unsigned int)popcount;
#else
        int popcount = __internal_popcnt(x);
        return (unsigned int)popcount;
#endif
    }

#if (JSTD_WORD_SIZE == 64)
    static inline
    unsigned int popcnt64(unsigned __int64 x) {
#if defined(__POPCNT__)
        __int64 popcount = _mm_popcnt_u64(x);
        return (unsigned int)popcount;
#else
        unsigned __int64 popcount = __internal_popcnt64(x);
        return (unsigned int)popcount;
#endif
    }
#else
    static inline
    unsigned int popcnt64(unsigned __int64 x) {
#if defined(__POPCNT__)
        int popcount = _mm_popcnt_u32((unsigned int)(x >> 32)) + _mm_popcnt_u32((unsigned int)(x & 0xFFFFFFFFUL));
        return (unsigned int)popcount;
#else
        unsigned __int64 popcount = __internal_popcnt64(x);
        return (unsigned int)popcount;
#endif
    }
#endif

#elif (defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))) \
   || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 900)) \
   || defined(__clang__)

    static inline
    unsigned int bsf32(unsigned int x) {
        assert(x != 0);
        // gcc: __bsfd(x)
        return (unsigned int)__builtin_ctz(x);
    }

#if (JSTD_WORD_SIZE == 64)
    static inline
    unsigned int bsf64(unsigned long long x) {
        assert(x != 0);
        // gcc: __bsfq(x)
        return (unsigned int)__builtin_ctzll(x);
    }
#endif

    static inline
    unsigned int bsr32(unsigned int x) {
        assert(x != 0);
        // gcc: __bsrd(x)
        return (unsigned int)__builtin_clz(x);
    }

#if (JSTD_WORD_SIZE == 64)
    static inline
    unsigned int bsr64(unsigned long long x) {
        assert(x != 0);
        // gcc: __bsrq(x)
        return (unsigned int)__builtin_clzll(x);
    }
#endif

    static inline
    unsigned int popcnt32(unsigned int x) {
        // gcc: __bsfd(x)
        return (unsigned int)__builtin_popcount(x);
    }

#if (JSTD_WORD_SIZE == 64)
    static inline
    unsigned int popcnt64(unsigned long long x) {
        // gcc: __bsfq(x)
        return (unsigned int)__builtin_popcountll(x);
    }
#endif

#else

    static inline
    unsigned int bsf32(unsigned int x) {
        assert(x != 0);
        return BitUtils::__internal_ctz(x);
    }

    static inline
    unsigned int bsf64(unsigned long long x) {
        assert(x != 0);
        return BitUtils::__internal_ctzll(x);
    }

    static inline
    unsigned int bsr32(unsigned int x) {
        assert(x != 0);
        return BitUtils::__internal_clz(x);
    }

    static inline
    unsigned int bsr64(unsigned long long x) {
        assert(x != 0);
        return BitUtils::__internal_clzll(x);
    }

    static inline
    unsigned int popcnt32(unsigned int x) {
        return BitUtils::__internal_popcnt(x);
    }

    static inline
    unsigned int popcnt64(uint64_t x) {
        return BitUtils::__internal_popcnt64(x);
    }

#endif // (_MSC_VER && _MSC_VER >= 1500)

    static inline unsigned int bsf(size_t x) {
#if (JSTD_WORD_SIZE == 64)
        return BitUtils::bsf64(x);
#else
        return BitUtils::bsf32(x);
#endif
    }

    static inline unsigned int bsr(size_t x) {
#if (JSTD_WORD_SIZE == 64)
        return BitUtils::bsr64(x);
#else
        return BitUtils::bsr32(x);
#endif
    }

    static inline unsigned int popcnt(size_t x) {
#if (JSTD_WORD_SIZE == 64)
        return BitUtils::popcnt64(x);
#else
        return BitUtils::popcnt32(x);
#endif
    }
};

} // namespace gudoku

#undef JSTD_IS_X86
#undef JSTD_IS_X86_32
#undef JSTD_IS_X86_64
#undef JSTD_WORD_SIZE

#endif // GUDOKU_BITUTILS_H
