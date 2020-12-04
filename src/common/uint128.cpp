// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef _MSC_VER
#include <intrin.h>

#pragma intrinsic(_umul128)
#pragma intrinsic(_udiv128)
#endif
#include <cstring>
#include "common/uint128.h"

namespace Common {

#ifdef _MSC_VER

u64 MultiplyAndDivide64(u64 a, u64 b, u64 d) {
    u128 r{};
    r[0] = _umul128(a, b, &r[1]);
    u64 remainder;
#if _MSC_VER < 1923
    return udiv128(r[1], r[0], d, &remainder);
#else
    return _udiv128(r[1], r[0], d, &remainder);
#endif
}

#else

u64 MultiplyAndDivide64(u64 a, u64 b, u64 d) {
    const u64 diva = a / d;
    const u64 moda = a % d;
    const u64 divb = b / d;
    const u64 modb = b % d;
    return diva * b + moda * divb + moda * modb / d;
}

#endif

u128 Multiply64Into128(u64 a, u64 b) {
    u128 result;
#ifdef _MSC_VER
    result[0] = _umul128(a, b, &result[1]);
#else
    unsigned __int128 tmp = a;
    tmp *= b;
    std::memcpy(&result, &tmp, sizeof(u128));
#endif
    return result;
}

} // namespace Common
