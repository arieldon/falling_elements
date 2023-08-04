static inline u32
RandomU32(void)
{
	// TODO(ariel) Seed initial state using RDSEED or RDRAND instruction?
	static u64 State = 1234;
	State = State * 6364136223846793005ull;
	u32 Output = (u32)((State ^ (State >> 22)) >> (22 + (State >> 61)));
	return Output;
}
