static u32
RandomU32(void)
{
	// TODO(ariel) Seed using RDSEED or RDRAND instruction?
	static u32 X = 1234;
	X ^= X << 13;
	X ^= X >> 17;
	X ^= X << 5;
	return X;
}
