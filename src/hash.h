#ifndef HASH_H
#define HASH_H

#include "MurmurHash/MurmurHash3.h"

#define MURMUR3_SEED (0xdeadbeef)

inline u32 hash32(i32 value, u32 seed = MURMUR3_SEED)
{
    u32 hashed;
    MurmurHash3_x86_32(&value, sizeof value, seed, &hashed);
    return hashed;
}

inline u32 hash32(i64 value, u32 seed = MURMUR3_SEED)
{
    u32 hashed;
    MurmurHash3_x86_32(&value, sizeof value, seed, &hashed);
    return hashed;
}

inline u32 hash32(u32 value, u32 seed = MURMUR3_SEED)
{
    u32 hashed;
    MurmurHash3_x86_32(&value, sizeof value, seed, &hashed);
    return hashed;
}

inline u32 hash32(u64 value, u32 seed = MURMUR3_SEED)
{
    u32 hashed;
    MurmurHash3_x86_32(&value, sizeof value, seed, &hashed);
    return hashed;
}

inline u32 hash32(void *data, i32 size, u32 seed = MURMUR3_SEED)
{
    u32 hashed;
    MurmurHash3_x86_32(data, size, seed, &hashed);
    return hashed;
}

inline u32 hash32(String str, u32 seed = MURMUR3_SEED)
{
    u32 hashed;
    MurmurHash3_x86_32(str.data, str.length, seed, &hashed);
    return hashed;
}

#endif // HASH_H
