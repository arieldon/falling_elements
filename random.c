static inline u32
RandomU32(void)
{
	// TODO(ariel) Seed initial state using RDSEED or RDRAND instruction?
	static u64 State = 1234;
	State = State * 6364136223846793005ull;
	u32 Output = (u32)((State ^ (State >> 22)) >> (22 + (State >> 61)));
	return Output;
}

static inline u32
RandomU32InRange(u32 Min, u32 Max)
{
	Assert(Min < Max);
	u32 Result = (RandomU32() % (Max - Min)) + Min;
	Assert(Min <= Result);
	Assert(Result <= Max);
	return Result;
}
