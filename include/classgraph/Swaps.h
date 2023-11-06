#pragma once

#include "Layout.h"

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

namespace classgraph {
    /**
     * Rapidly swap u16 values in the range [begin, end]
     */
    template<bool UseNative=true>
    void swap_small_points(uint16_t* begin, const uint16_t* end, uint16_t a, uint16_t b) {
#if defined(__AVX512VL__) && defined(__AVX512BW__)
        if constexpr (UseNative) {
            __m512i splat_a = _mm512_set1_epi16(a), splat_b = _mm512_set1_epi16(b);
            while (begin < end) {
                ptrdiff_t mask_shift = end - begin;
                __mmask32 mask = shift >= 32 ? -1 : (((uint32_t)1 << shift) - 1);
                __m512i load = _mm512_maskz_loadu_epi16(mask, (const __m512i*) begin);

                __mmask32 matches_a = _mm512_cmpeq_epi16_mask(load, splat_a);
                __mmask32 matches_b = _mm512_cmpeq_epi16_mask(load, splat_b);

                __m512i a_swap = _mm512_mask_mov_epi16(load, matches_a, splat_b);
                __m512i result = _mm512_mask_mov_epi16(a_swap, matches_b, splat_a);

                _mm512_mask_storeu_epi16((void*) begin, mask, result);
                begin += 32;
            }
        }

        return;
#endif
        
#ifdef __AVX2__
        if constexpr (UseNative) {
#define ITER(prefix, si, vtype, stride) {\
                vtype splat_a = prefix##_set1_epi16(a), splat_b = prefix##_set1_epi16(b); \
                while (begin + stride - 1 < end) { \
                    vtype load = prefix##_loadu_##si##((const vtype*) begin); \
                    vtype matches_a = prefix##_cmpeq_epi16(load, splat_a); \
                    vtype matches_b = prefix##_cmpeq_epi16(load, splat_b); \
                    vtype matches_any = prefix##_or_##si##(matches_a, matches_b); \
                    vtype keep = prefix##_andn_##si##(matches_any, load); \
                    vtype a_swap = prefix##_and_##si##(matches_a, splat_b); \
                    vtype b_swap = prefix##_and_##si##(matches_b, splat_a); \
                    vtype swapped = prefix##_or_##si##(a_swap, b_swap); \
                    vtype result = prefix##_or_##si##(keep, swapped); \
                    prefix##_storeu_##si##((vtype*) begin, result); \
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

    template <bool UseNative=true, typename T>
    std::enable_if_t<sizeof(T) % 2 == 0> swap_small_points_vector(std::vector<T>& vec, uint16_t a, uint16_t b) {
        swap_small_points<UseNative>(reinterpret_cast<uint16_t*>(&*vec.begin()),
                          reinterpret_cast<uint16_t*>(&*vec.end()),
                          a, b);
    }
}