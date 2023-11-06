
#pragma once

#include <utility>
#include <cassert>

namespace anematode {
    template <typename Result, typename In,
            std::enable_if_t<std::is_integral_v<In> && std::is_integral_v<Result>>...>
    Result checked_int_cast(In val) {
        assert(std::in_range<Result>(val));

        return static_cast<Result>(val);
    }
}