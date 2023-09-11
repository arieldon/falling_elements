#ifndef RANDOM_H
#define RANDOM_H

typedef struct random_sequence random_sequence;
struct random_sequence
{
	// NOTE(ariel) `State` represents the internal state that the generator uses
	// to output the next pseudorandom value. `Selection` represents the random
	// sequence (of pow(2, 63) possible sequences in the entire state space) that
	// `State` iterates -- it shouldn't change over the generator's lifetime.
	u64 State;
	u64 Selection;
};

static thread_local random_sequence RandomSequence = {0};

// NOTE(ariel) Call SeedRandom() before any other function related to
// psuedorandom number generation.
static random_sequence
SeedRandom(void)
{
	StaticAssert(sizeof(unsigned long long) == sizeof(u64));
	_rdseed64_step((unsigned long long *)&RandomSequence.State);
	RandomSequence.Selection = ((uintptr_t)SeedRandom << 1) | 1;

	Assert(RandomSequence.State);
	return RandomSequence;
}

static inline u32
RotateRight(u32 Value, u32 Count)
{
#if defined(__clang__)
	u32 Result =	__builtin_rotateright32(Value, Count);
#elif defined(__GNUC__)
	u32 Result = _rotr(Value, Count);
#else
#error "Implement RotateRight() for this compiler."
#endif
	return Result;
}

static inline u32
RandomU32(void)
{
	const u64 CONSTANT_WITH_HIGH_EMPIRICAL_CONFIDENCE = 6364136223846793005ull;

	// NOTE(ariel) This is an implementation of PCG-XSH-RR (permuted congruential
	// generator, xorshift high bits, random rotate) based on Melissa O'Neill's
	// paper and library.
	u64 PreviousState = RandomSequence.State;
	u64 NextState = PreviousState * CONSTANT_WITH_HIGH_EMPIRICAL_CONFIDENCE + RandomSequence.Selection;

	// NOTE(ariel) Perform a xorshift to produce more statistically solid high
	// bits, where top 5 bits of 32 bits specify shift, hence right shift 27, and
	// floor((32+5) / 2) = 18 is another arbitrary constant as far as I can tell
	// necessary for non-zero xor output. Rotate bits to produce a full period,
	// where again top 5 bits of 64 bits specify rotation, hence right shift 59.
	u32 Rotate = (u32)(PreviousState >> 59);
	u32 Xorshift = (u32)(((PreviousState >> 18) ^ PreviousState) >> 27);
	u32 Result = RotateRight(Xorshift, Rotate);

	RandomSequence.State = NextState;
	return Result;
}

static inline u32
RandomU32InRange(u32 Min, u32 Max)
{
	// NOTE(ariel) Is it user error if Min == Max?
	Assert(Min <= Max);
	u32 Result = (RandomU32() % ((Max + 1) - Min)) + Min;
	Assert(Min <= Result);
	Assert(Result <= Max);
	return Result;
}

#endif
