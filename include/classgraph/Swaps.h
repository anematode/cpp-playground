#pragma once

#include "Layout.h"
#include <vector>

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include <tuple>
#include <iostream>

namespace classgraph {
    /**
     * Rapidly swap u16 values in the range [begin, end]
     */
    template<bool UseNative=true>
    void swap_small_points(uint16_t* begin, const uint16_t* end, uint16_t a, uint16_t b) {
#if defined(__AVX512VL__) && defined(__AVX512BW__)
        if constexpr (UseNative) {
            if (end - begin >= 64) {  // only faster above a certain threshold
                __m512i splat_a = _mm512_set1_epi16(a), splat_b = _mm512_set1_epi16(b);

                auto do_with_mask = [&] (int mask_shift) {
                    __mmask32 mask = mask_shift >= 32 ? -1 : (((uint32_t)1 << mask_shift) - 1);
                    __m512i load = _mm512_maskz_loadu_epi16(mask, (const __m512i*) begin);

                    __mmask32 matches_a = _mm512_cmpeq_epi16_mask(load, splat_a);
                    __mmask32 matches_b = _mm512_cmpeq_epi16_mask(load, splat_b);

                    __m512i a_swap = _mm512_mask_mov_epi16(load, matches_a, splat_b);
                    __m512i result = _mm512_mask_mov_epi16(a_swap, matches_b, splat_a);

                    _mm512_mask_storeu_epi16((void*) begin, mask, result);
                };

                // Align me to 64-byte boundary
                int rem = (uintptr_t)begin % 64;
                if (rem != 0) {
                    rem = 32 - (rem >> 1);
                    do_with_mask(rem);
                    begin += rem;
                }

                while (begin < end) {
                    ptrdiff_t mask_shift = end - begin;
                    do_with_mask(mask_shift);
                    begin += 32;
                }

                return;
            }
        }

#elif defined(__AVX2__)
        if constexpr (UseNative) {
#define ITER(prefix, si, vtype, stride) {\
                vtype splat_a = prefix##_set1_epi16(a), splat_b = prefix##_set1_epi16(b); \
                while (begin + stride - 1 < end) { \
                    vtype load = prefix##_loadu_##si((const vtype*) begin); \
                    vtype matches_a = prefix##_cmpeq_epi16(load, splat_a); \
                    vtype matches_b = prefix##_cmpeq_epi16(load, splat_b); \
                    vtype result = prefix##_blendv_epi8(load, splat_b, matches_a); \
                    result = prefix##_blendv_epi8(result, splat_a,  matches_b); \
                    prefix##_storeu_##si((vtype*) begin, result); \
                    begin += stride; \
                }\
            }

            ITER(_mm256, si256, __m256i, 16)
            ITER(_mm, si128, __m128i, 8)
        }
#undef ITER
#endif

        // clean up
        while (begin < end) {
            uint16_t val = *begin;

            if (val == a) {
                val = b;
            } else if (val == b) {
                val = a;
            }

            *(unsigned short*)begin = val;
            begin++;
        }
    }

    // Swap pairs (r, swap_i) and (r, swap_j) where row_min <= r <= row_max
    template <bool UseNative=true>
    void swap_rows(uint16_t* begin, const uint16_t* end, int swap_i, int swap_j, int col_min, int col_max) {
#if defined(__AVX512VL__) && defined(__AVX512BW__)
        while (begin < end) {
            break;
        }
#endif
        while (begin < end) {
            uint16_t val = *begin;
            uint8_t x = val & 0xff;
            uint8_t y = val >> 8;

            if (x >= col_min && x <= col_max) {
                if (y == swap_i) {
                    y = swap_j;
                } else if (y == swap_j) {
                    y = swap_i;
                }

                *begin = x | (y << 8);
            }
        }
    }

    template <bool UseNative=true, typename T>
    std::enable_if_t<sizeof(T) % 2 == 0> swap_small_points_vector(std::vector<T>& vec, uint16_t a, uint16_t b) {
        swap_small_points<UseNative>(reinterpret_cast<uint16_t*>(&*vec.begin()),
                          reinterpret_cast<uint16_t*>(&*vec.end()),
                          a, b);
    }

    struct IntersectionCounters {
        int proper{};
        int improper{};

        IntersectionCounters() = default;
        IntersectionCounters(int proper, int improper) : proper(proper), improper(improper) {}

        IntersectionCounters operator+= (const IntersectionCounters& other) {
            proper += other.proper;
            improper += other.improper;

            return *this;
        }

        bool operator== (const IntersectionCounters& other) const {
            return proper == other.proper && improper == other.improper;
        }
    };


    inline std::ostream& operator<<(std::ostream& out, const IntersectionCounters& counters) {
        out << "IntersectionCounters{ proper=" << counters.proper << ", improper=" << counters.improper << " }";
        return out;
    }

    namespace {
        int cross(Point a, Point b) {
            return a.x*b.y - a.y*b.x;
        }

        int orient(Point a, Point b, Point c) {
            return cross(b-a, c-a);
        }

        IntersectionCounters intersects(Point a, Point b, Point c, Point d) {
            int oa = orient(c,d,a), 
                ob = orient(c,d,b),            
                oc = orient(a,b,c),            
                od = orient(a,b,d);      

            int proper = oa * ob < 0 && oc * od < 0;
            int improper = oa * ob <= 0 && oc * od <= 0;

            return { .proper = proper, .improper = improper };
        }

        using PermType = const std::array<
            std::pair<int /* get this (x, y) pair */, bool /* swap x, y? */>, 4
            >&;

        constexpr std::pair<uint64_t, uint64_t> get_perm64(PermType perm) {
            uint64_t perm_64 = 0;

            int i = 0;
            for (const auto& pair : perm) {
                int get = pair.first;
                bool swap_xy = pair.second;

                uint64_t k = !swap_xy ? (((get * 2 + 1) << 8) | (get * 2) )
                    : (((get * 2) << 8) | (get * 2 + 1) );

                perm_64 |= (k << (16 * i));

                i++;
            }

            uint64_t perm_hi = perm_64 + 0x0808080808080808ULL;

            return { perm_hi, perm_64 };
        }

#ifdef __AVX2__
        __m256i perm_256_4xy8(PermType perm) {
            uint64_t perm_hi, perm_64;
            std::tie(perm_hi, perm_64) = get_perm64(perm);

            return _mm256_set_epi64x(perm_hi, perm_64, perm_hi, perm_64);
        }

        __m256i cross_product_4xy8_yx8(__m256i a, __m256i b) {
            // For each 32-bit pair 
            // Computes a.x * b.y - a.y * b.x across all 16-bit pairs in (a,b)

            const __m256i high8_only = _mm256_set1_epi16(0xff00);
            __m256i b2 = _mm256_sub_epi8(_mm256_setzero_si256(), b);

            b = _mm256_blendv_epi8(b, b2, high8_only);

            return _mm256_maddubs_epi16(a, b);
        }
#endif

#ifdef __AVX512F__
        __m512i perm_512_8xy8(PermType perm) {
            uint64_t perm_hi, perm_64;
            std::tie(perm_hi, perm_64) = get_perm64(perm);

            return _mm512_set_epi64(perm_hi, perm_64, perm_hi, perm_64,
                    perm_hi, perm_64, perm_hi, perm_64);
        }

        __m512i cross_product_8xy8_yx8(__m512i a, __m512i b) {
            // Computes 
            const __mmask64 high8_only = 0xaaaaaaaaaaaaaaaaULL;

            // negate b if high8_only ^ corresponding a is negative.
            // Work around 

            const __m512i zero = _mm512_setzero_si512();
            __mmask64 a_is_negative = _mm512_movepi8_mask(a);

            __m512i neg_b = _mm512_mask_sub_epi8(b, high8_only ^ a_is_negative, _mm512_setzero_si512(), b);
            __m512i abs_a = _mm512_abs_epi8(a);

            return _mm512_maddubs_epi16(abs_a, neg_b);
        }

        template <typename T, typename Vec>
        void print_vec(Vec vec) {
            const int count = sizeof(Vec) / sizeof(T);
            T arr[count]; 

            if constexpr (std::is_same_v<__m256i, Vec>) {
                _mm256_storeu_si256((__m256i*) arr, vec);
            } else if constexpr (std::is_same_v<__m512i, Vec>) { 
                _mm512_storeu_si512((__m512i*) arr, vec);
            } else {
                static_assert(!sizeof(Vec));
            }

            if constexpr (std::is_unsigned_v<T>) {
                for (int i = 0; i < count; ++i) {
                    std::cout << (uint64_t)arr[i] << ' ';
                }
            } else {
                for (int i = 0; i < count; ++i) {
                    std::cout << (int64_t)arr[i] << ' ';
                }
            }

            std::cout << '\n';
        }

        void calculate_signs(__m512i data, int* lt0, int* le0) {
            // oa = cross(d - c, a - c)
            // ob = cross(d - c, b - c)
            // oc = cross(b - a, c - a)
            // od = cross(b - a, d - a)

            constexpr int a = 0;
            constexpr int b = 1;
            constexpr int c = 2;
            constexpr int d = 3;

            using A = std::array<std::pair<int, bool>, 4>;

            static __m512i ddbb_xy = perm_512_8xy8(A{ { { d, false }, { d, false }, { b, false }, { b, false } } });
            static __m512i ccaa_xy = perm_512_8xy8(A{ { { c, false }, { c, false }, { a, false }, { a, false } } });
            static __m512i abcd_yx = perm_512_8xy8(A{ { { a, true }, { b, true }, { c, true }, { d, true } } });
            static __m512i ccaa_yx = perm_512_8xy8(A{ { { c, true }, { c, true }, { a, false }, { a, false } } }); 


            // d - c  d - c  b - a  b - a
            __m512i v1 = _mm512_sub_epi8( _mm512_shuffle_epi8(data, ddbb_xy), _mm512_shuffle_epi8(data, ccaa_xy));
            // a - c  b - c  c - a  d - a
            __m512i v2 = _mm512_sub_epi8( _mm512_shuffle_epi8(data, abcd_yx), _mm512_shuffle_epi8(data, ccaa_yx));

            __m512i cross = cross_product_8xy8_yx8(v1, v2);

            __m512i rolled = _mm512_rol_epi32(cross, 16);
            __m512i prods = _mm512_madd_epi16(cross, rolled);

            const __m512i zero = _mm512_setzero_si512();

            // There are now 16 32-bit products in prods. If two consecutive prods are < 0, there is a strict
            // intersection. If two consecutive prods are = 0, there is a non-strict intersection.

            int negatives = _mm512_movepi32_mask(prods);
            int nonpositives = negatives | _mm512_testn_epi32_mask(prods, prods);

            *lt0 = negatives & ((negatives & 0xaaaa) >> 1);
            *le0 = (nonpositives | (nonpositives >> 1)) & 0x5555;
#endif
        }

    }


    template <bool UseNative=true>
    IntersectionCounters count_intersections(const uint64_t* begin_, const uint64_t* end_) {
        static_assert(sizeof(SmallPoint) == 2);
        const SmallPoint* begin = (const SmallPoint*)begin_;
        const SmallPoint* end = (const SmallPoint*) end_;

        IntersectionCounters result;

#ifdef __AVX512BW__
        if constexpr (UseNative) {
            auto do_with_mask = [&] (int mask_shift) {
                int mask = mask_shift >= 8 ? -1 : (((uint32_t)1 << mask_shift) - 1);
                __m512i load = _mm512_maskz_loadu_epi32(mask, (const __m512i*) begin);

                int lt0, le0;
                calculate_signs(load, &lt0, &le0);

                lt0 &= mask;
                le0 &= mask;

                result += IntersectionCounters { __builtin_popcount(lt0), __builtin_popcount(le0) };
            };

#if 0
            // Align me to 64-byte boundary
            int rem = (uintptr_t)begin % 64;
            if (rem != 0) {
                rem = 32 - (rem >> 1);
                do_with_mask(rem);
                begin += rem;
            }
#endif

            while (begin < end) {
                ptrdiff_t mask_shift = (end - begin) >> 1;
                do_with_mask(mask_shift);
                begin += 32;
            }

            return result;
        }
#endif

        while (begin < end) {
            SmallPoint a = *begin, b = *(begin + 1), c = *(begin + 2), d = *(begin + 3);
            result += intersects(a.point(), b.point(), c.point(), d.point());
            begin += 4;
        }

        return result;
    }

    template <bool UseNative=true, typename T>
    IntersectionCounters count_intersections(const std::vector<T>& inter) {
        return count_intersections((const uint64_t*)&*inter.begin(), (const uint64_t*)&*inter.end());
    }
}
