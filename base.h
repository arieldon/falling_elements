#ifndef BASE_H
#define BASE_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef  uint8_t    u8;
typedef   int8_t    s8;
typedef uint16_t   u16;
typedef  int16_t   s16;
typedef uint32_t   u32;
typedef  int32_t   s32;
typedef uint64_t   u64;
typedef  int64_t   s64;
typedef   size_t usize;
typedef  ssize_t ssize;
typedef      s32   b32;
typedef    float   f32;
typedef   double   f64;

enum
{
	false = 0,
	true = 1,
};

#define Breakpoint() do { __asm__("int $3"); __asm__("nop"); } while (0)
#define Message(x) fprintf(stderr, "%s:%d: %s: assertion `%s` failed\n", __FILE__, __LINE__, __func__, #x)
#define Assert(x) do { if (!(x)) { Message(x); Breakpoint(); } } while (0)
#define StaticAssert(x) _Static_assert(x, "")

#define Min(A, B) ((A) < (B) ? (A) : (B))
#define Max(A, B) ((A) > (B) ? (A) : (B))
#define Clamp(Value, MinValue, MaxValue) (Max(MinValue, Min(MaxValue, Value)))

#define Swap(A, B) do { __typeof__(A) Temporary = A; A = B; B = Temporary; } while (0)
#define SwapIfLess(A, B) do { if (A < B) Swap(A, B); } while (0)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define KB(n) (n << 10)
#define MB(n) (n << 20)
#define GB(n) (((u64)n) << 30)
#define TB(n) (((u64)n) << 40)

#endif
