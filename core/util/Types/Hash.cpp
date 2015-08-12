#include "pch.h"

// By Bob Jenkins, 2006. bob_jenkins@burtleburtle.net. You may use this
// code any way you wish, private, educational, or commercial. It's free.
// See http://burtleburtle.net/bob/hash/evahash.html
// See http://burtleburtle.net/bob/c/lookup3.c

static const ff::hash_t MAGIC_HASH_INIT_32 = 0x9e3779b9;

inline static ff::hash_t CreateHashResult32(DWORD b, DWORD c)
{
	static_assert(sizeof(ff::hash_t) == 8, "Hash type must be 64-bits");
	return (static_cast<ff::hash_t>(b) << 32) | static_cast<ff::hash_t>(c);
}

inline static DWORD RotateBits32(DWORD val, DWORD count)
{
	return (val << count) | (val >> (32 - count));
}

inline static void HashMix32(DWORD& a, DWORD& b, DWORD& c)
{
	a -= c; a ^= RotateBits32(c, 4); c += b;
	b -= a; b ^= RotateBits32(a, 6); a += c;
	c -= b; c ^= RotateBits32(b, 8); b += a;
	a -= c; a ^= RotateBits32(c, 16); c += b;
	b -= a; b ^= RotateBits32(a, 19); a += c;
	c -= b; c ^= RotateBits32(b, 4); b += a;
}

inline static void FinalHashMix32(DWORD& a, DWORD& b, DWORD& c)
{
	c ^= b; c -= RotateBits32(b, 14);
	a ^= c; a -= RotateBits32(c, 11);
	b ^= a; b -= RotateBits32(a, 25);
	c ^= b; c -= RotateBits32(b, 16);
	a ^= c; a -= RotateBits32(c, 4);
	b ^= a; b -= RotateBits32(a, 14);
	c ^= b; c -= RotateBits32(b, 24);
}

ff::hash_t ff::HashBytes(const void* data, size_t length)
{
	DWORD a = MAGIC_HASH_INIT_32 + (DWORD)length; // + (DWORD)nInitVal;
	DWORD b = a;
	DWORD c = a; // + (DWORD)(nInitVal >> 32);

	if (((size_t)data & 0x3) == 0)
	{
		const DWORD* keyData = (const DWORD*)data;

		while (length > 12)
		{
			a += keyData[0];
			b += keyData[1];
			c += keyData[2];

			HashMix32(a, b, c);

			length -= 12;
			keyData += 3;
		}

		switch (length)
		{
		case 12: c += keyData[2]; b += keyData[1]; a += keyData[0]; break;
		case 11: c += keyData[2] & 0xffffff; b += keyData[1]; a += keyData[0]; break;
		case 10: c += keyData[2] & 0xffff; b += keyData[1]; a += keyData[0]; break;
		case 9: c += keyData[2] & 0xff; b += keyData[1]; a += keyData[0]; break;
		case 8: b += keyData[1]; a += keyData[0]; break;
		case 7: b += keyData[1] & 0xffffff; a += keyData[0]; break;
		case 6: b += keyData[1] & 0xffff; a += keyData[0]; break;
		case 5: b += keyData[1] & 0xff; a += keyData[0]; break;
		case 4: a += keyData[0]; break;
		case 3: a += keyData[0] & 0xffffff; break;
		case 2: a += keyData[0] & 0xffff; break;
		case 1: a += keyData[0] & 0xff; break;
		case 0: return CreateHashResult32(b, c);
		}
	}
	else if (((size_t)data & 0x1) == 0)
	{
		const WORD* keyData = (const WORD*)data;

		while (length > 12)
		{
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			c += keyData[4] + (((DWORD)keyData[5]) << 16);

			HashMix32(a, b, c);

			length -= 12;
			keyData += 6;
		}

		const BYTE* byteData = (const BYTE*)keyData;

		switch (length)
		{
		case 12:
			c += keyData[4] + (((DWORD)keyData[5]) << 16);
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 11:
			c += ((DWORD)byteData[10]) << 16;
			__fallthrough;

		case 10:
			c += keyData[4];
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 9:
			c += byteData[8];
			__fallthrough;

		case 8:
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 7:
			b += ((DWORD)byteData[6]) << 16;
			__fallthrough;

		case 6:
			b += keyData[2];
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 5:
			b += byteData[4];
			__fallthrough;

		case 4:
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 3:
			a += ((DWORD)byteData[2]) << 16;
			__fallthrough;

		case 2:
			a += keyData[0];
			break;

		case 1:
			a += byteData[0];
			break;

		case 0:
			return CreateHashResult32(b, c);
		}
	}
	else
	{
		const BYTE* keyData = (const BYTE*)data;

		while (length > 12)
		{
			a += keyData[0];
			a += ((DWORD)keyData[1]) << 8;
			a += ((DWORD)keyData[2]) << 16;
			a += ((DWORD)keyData[3]) << 24;

			b += keyData[4];
			b += ((DWORD)keyData[5]) << 8;
			b += ((DWORD)keyData[6]) << 16;
			b += ((DWORD)keyData[7]) << 24;

			c += keyData[8];
			c += ((DWORD)keyData[9]) << 8;
			c += ((DWORD)keyData[10]) << 16;
			c += ((DWORD)keyData[11]) << 24;

			HashMix32(a, b, c);

			length -= 12;
			keyData += 12;
		}

		switch (length)
		{
		case 12: c += ((DWORD)keyData[11]) << 24;
		case 11: c += ((DWORD)keyData[10]) << 16;
		case 10: c += ((DWORD)keyData[9]) << 8;
		case 9: c += keyData[8];
		case 8: b += ((DWORD)keyData[7]) << 24;
		case 7: b += ((DWORD)keyData[6]) << 16;
		case 6: b += ((DWORD)keyData[5]) << 8;
		case 5: b += keyData[4];
		case 4: a += ((DWORD)keyData[3]) << 24;
		case 3: a += ((DWORD)keyData[2]) << 16;
		case 2: a += ((DWORD)keyData[1]) << 8;
		case 1: a += keyData[0];
			break;

		case 0:
			return CreateHashResult32(b, c);
		}
	}

	FinalHashMix32(a, b, c);

	return CreateHashResult32(b, c);
}

#if 0 // measured slower than Hash32.cpp

// By Bob Jenkins, 2012. bob_jenkins@burtleburtle.net. You may use this
// code any way you wish, private, educational, or commercial. It's free.
// See http://burtleburtle.net/bob/hash/spooky.html

static const size_t sc_numVars64 = 12;
static const size_t sc_blockSize64 = sc_numVars64 * 8;
static const size_t sc_bufSize64 = 2 * sc_blockSize64;
static const uint64_t sc_const64 = 0xc3a5c85c97cb3127ULL;
static const uint64_t sc_const64_2 = 0xb492b66fbe98f273ULL;
static const uint64_t sc_const64_3 = 0x9ae16a3b2f90404fULL;

inline static uint64_t Rot64(uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

inline static void Mix64(const uint64_t* data,
	uint64_t& s0, uint64_t& s1, uint64_t& s2, uint64_t& s3,
	uint64_t& s4, uint64_t& s5, uint64_t& s6, uint64_t& s7,
	uint64_t& s8, uint64_t& s9, uint64_t& s10, uint64_t& s11)
{
	s0 += data[0];   s2 ^= s10; s11 ^= s0;  s0 = Rot64(s0, 11);   s11 += s1;
	s1 += data[1];   s3 ^= s11; s0 ^= s1;   s1 = Rot64(s1, 32);   s0 += s2;
	s2 += data[2];   s4 ^= s0;  s1 ^= s2;   s2 = Rot64(s2, 43);   s1 += s3;
	s3 += data[3];   s5 ^= s1;  s2 ^= s3;   s3 = Rot64(s3, 31);   s2 += s4;
	s4 += data[4];   s6 ^= s2;  s3 ^= s4;   s4 = Rot64(s4, 17);   s3 += s5;
	s5 += data[5];   s7 ^= s3;  s4 ^= s5;   s5 = Rot64(s5, 28);   s4 += s6;
	s6 += data[6];   s8 ^= s4;  s5 ^= s6;   s6 = Rot64(s6, 39);   s5 += s7;
	s7 += data[7];   s9 ^= s5;  s6 ^= s7;   s7 = Rot64(s7, 57);   s6 += s8;
	s8 += data[8];   s10 ^= s6; s7 ^= s8;   s8 = Rot64(s8, 55);   s7 += s9;
	s9 += data[9];   s11 ^= s7; s8 ^= s9;   s9 = Rot64(s9, 54);   s8 += s10;
	s10 += data[10]; s0 ^= s8;  s9 ^= s10;  s10 = Rot64(s10, 22); s9 += s11;
	s11 += data[11]; s1 ^= s9;  s10 ^= s11; s11 = Rot64(s11, 46); s10 += s0;
}

inline static void EndPartial64(
	uint64_t& h0, uint64_t& h1, uint64_t& h2, uint64_t& h3,
	uint64_t& h4, uint64_t& h5, uint64_t& h6, uint64_t& h7,
	uint64_t& h8, uint64_t& h9, uint64_t& h10, uint64_t& h11)
{
	h11 += h1; h2 ^= h11; h1 = Rot64(h1, 44);
	h0 += h2;  h3 ^= h0;  h2 = Rot64(h2, 15);
	h1 += h3;  h4 ^= h1;  h3 = Rot64(h3, 34);
	h2 += h4;  h5 ^= h2;  h4 = Rot64(h4, 21);
	h3 += h5;  h6 ^= h3;  h5 = Rot64(h5, 38);
	h4 += h6;  h7 ^= h4;  h6 = Rot64(h6, 33);
	h5 += h7;  h8 ^= h5;  h7 = Rot64(h7, 10);
	h6 += h8;  h9 ^= h6;  h8 = Rot64(h8, 13);
	h7 += h9;  h10 ^= h7; h9 = Rot64(h9, 38);
	h8 += h10; h11 ^= h8; h10 = Rot64(h10, 53);
	h9 += h11; h0 ^= h9;  h11 = Rot64(h11, 42);
	h10 += h0; h1 ^= h10; h0 = Rot64(h0, 54);
}

inline static void End64(const uint64_t* data,
	uint64_t& h0, uint64_t& h1, uint64_t& h2, uint64_t& h3,
	uint64_t& h4, uint64_t& h5, uint64_t& h6, uint64_t& h7,
	uint64_t& h8, uint64_t& h9, uint64_t& h10, uint64_t& h11)
{
	h0 += data[0]; h1 += data[1]; h2 += data[2];   h3 += data[3];
	h4 += data[4]; h5 += data[5]; h6 += data[6];   h7 += data[7];
	h8 += data[8]; h9 += data[9]; h10 += data[10]; h11 += data[11];

	EndPartial64(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
	EndPartial64(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
	EndPartial64(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
}

inline static void ShortMix64(uint64_t& h0, uint64_t& h1, uint64_t& h2, uint64_t& h3)
{
	h2 = Rot64(h2, 50);  h2 += h3;  h0 ^= h2;
	h3 = Rot64(h3, 52);  h3 += h0;  h1 ^= h3;
	h0 = Rot64(h0, 30);  h0 += h1;  h2 ^= h0;
	h1 = Rot64(h1, 41);  h1 += h2;  h3 ^= h1;
	h2 = Rot64(h2, 54);  h2 += h3;  h0 ^= h2;
	h3 = Rot64(h3, 48);  h3 += h0;  h1 ^= h3;
	h0 = Rot64(h0, 38);  h0 += h1;  h2 ^= h0;
	h1 = Rot64(h1, 37);  h1 += h2;  h3 ^= h1;
	h2 = Rot64(h2, 62);  h2 += h3;  h0 ^= h2;
	h3 = Rot64(h3, 34);  h3 += h0;  h1 ^= h3;
	h0 = Rot64(h0, 5);   h0 += h1;  h2 ^= h0;
	h1 = Rot64(h1, 36);  h1 += h2;  h3 ^= h1;
}

inline static void ShortEnd64(uint64_t& h0, uint64_t& h1, uint64_t& h2, uint64_t& h3)
{
	h3 ^= h2;  h2 = Rot64(h2, 15);  h3 += h2;
	h0 ^= h3;  h3 = Rot64(h3, 52);  h0 += h3;
	h1 ^= h0;  h0 = Rot64(h0, 26);  h1 += h0;
	h2 ^= h1;  h1 = Rot64(h1, 51);  h2 += h1;
	h3 ^= h2;  h2 = Rot64(h2, 28);  h3 += h2;
	h0 ^= h3;  h3 = Rot64(h3, 9);   h0 += h3;
	h1 ^= h0;  h0 = Rot64(h0, 47);  h1 += h0;
	h2 ^= h1;  h1 = Rot64(h1, 54);  h2 += h1;
	h3 ^= h2;  h2 = Rot64(h2, 32);  h3 += h2;
	h0 ^= h3;  h3 = Rot64(h3, 25);  h0 += h3;
	h1 ^= h0;  h0 = Rot64(h0, 63);  h1 += h0;
}

static void HashBytesShort64(const void* data, size_t length, uint64_t* hash1, uint64_t* hash2)
{
	uint64_t buf[2 * sc_numVars64];

	union
	{
		const uint8_t* p8;
		uint32_t* p32;
		uint64_t* p64;
		size_t i;
	} u;

	u.p8 = (const uint8_t*)data;

	if (u.i & 0x7)
	{
		memcpy(buf, data, length);
		u.p64 = buf;
	}

	size_t remainder = length % 32;
	uint64_t a = *hash1;
	uint64_t b = *hash2;
	uint64_t c = sc_const64;
	uint64_t d = sc_const64;

	if (length > 15)
	{
		const uint64_t* end = u.p64 + (length / 32) * 4;

		// handle all complete sets of 32 bytes
		for (; u.p64 < end; u.p64 += 4)
		{
			c += u.p64[0];
			d += u.p64[1];
			ShortMix64(a, b, c, d);
			a += u.p64[2];
			b += u.p64[3];
		}

		//Handle the case of 16+ remaining bytes.
		if (remainder >= 16)
		{
			c += u.p64[0];
			d += u.p64[1];
			ShortMix64(a, b, c, d);
			u.p64 += 2;
			remainder -= 16;
		}
	}

	// Handle the last 0..15 bytes, and its length
	d += ((uint64_t)length) << 56;

	switch (remainder)
	{
	case 15:
		d += ((uint64_t)u.p8[14]) << 48;
		__fallthrough;

	case 14:
		d += ((uint64_t)u.p8[13]) << 40;
		__fallthrough;

	case 13:
		d += ((uint64_t)u.p8[12]) << 32;
		__fallthrough;

	case 12:
		d += u.p32[2];
		c += u.p64[0];
		break;

	case 11:
		d += ((uint64_t)u.p8[10]) << 16;
		__fallthrough;

	case 10:
		d += ((uint64_t)u.p8[9]) << 8;
		__fallthrough;

	case 9:
		d += (uint64_t)u.p8[8];
		__fallthrough;

	case 8:
		c += u.p64[0];
		break;

	case 7:
		c += ((uint64_t)u.p8[6]) << 48;
		__fallthrough;

	case 6:
		c += ((uint64_t)u.p8[5]) << 40;
		__fallthrough;

	case 5:
		c += ((uint64_t)u.p8[4]) << 32;
		__fallthrough;

	case 4:
		c += u.p32[0];
		break;

	case 3:
		c += ((uint64_t)u.p8[2]) << 16;
		__fallthrough;

	case 2:
		c += ((uint64_t)u.p8[1]) << 8;
		__fallthrough;

	case 1:
		c += (uint64_t)u.p8[0];
		break;

	case 0:
		c += sc_const64;
		d += sc_const64;
	}

	ShortEnd64(a, b, c, d);

	*hash1 = a;
	*hash2 = b;
}

ff::hash_t ff::HashBytes(const void* data, size_t length)
{
	uint64_t hash1 = sc_const64_2;
	uint64_t hash2 = sc_const64_3;

	if (length < sc_bufSize64)
	{
		HashBytesShort64(data, length, &hash1, &hash2);
		return hash1;
	}

	uint64_t h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11;
	uint64_t buf[sc_numVars64];

	union
	{
		const uint8_t* p8;
		uint64_t* p64;
		size_t i;
	} u;

	h0 = h3 = h6 = h9 = hash1;
	h1 = h4 = h7 = h10 = hash2;
	h2 = h5 = h8 = h11 = sc_const64;

	u.p8 = (const uint8_t*)data;
	uint64_t* end = u.p64 + (length / sc_blockSize64) * sc_numVars64;

	// handle all whole sc_blockSize64 blocks of bytes
	if ((u.i & 0x7) == 0)
	{
		while (u.p64 < end)
		{
			Mix64(u.p64, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
			u.p64 += sc_numVars64;
		}
	}
	else
	{
		while (u.p64 < end)
		{
			memcpy(buf, u.p64, sc_blockSize64);
			Mix64(buf, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);
			u.p64 += sc_numVars64;
		}
	}

	// handle the last partial block of sc_blockSize64 bytes
	uint8_t remainder = (uint8_t)(length - ((const uint8_t*)end - (const uint8_t*)data));
	memcpy(buf, end, remainder);
	memset(((uint8_t*)buf) + remainder, 0, sc_blockSize64 - remainder);
	((uint8_t*)buf)[sc_blockSize64 - 1] = remainder;

	// do some final mixing 
	End64(buf, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11);

	return hash1;
}

#endif
