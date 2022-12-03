#pragma once

#include "CoreMinimal.h"
#include <array>

struct FNode
{
	using Node = std::array<uint32, 8>;
	explicit FNode()
	{
		for (auto& i : Data) { i = INDEX_NONE; }
	}
	FNode(uint32 data)
	{
		for (auto& i : Data) { i = data; }
	}

	Node Data;
	uint32 RefCount = 1;

	const uint32& operator[](uint32 index)const { return Data[index]; };
	uint32& operator[](uint32 index) { return Data[index]; };
	operator std::array<uint32, 8>()const { return Data; };
	bool operator==(const FNode& FNode)const { return (FNode.Data == this->Data); };
	void operator=(const FNode& FNode)
	{
		Data = FNode.Data;
		RefCount = 1;
	}
};


FORCEINLINE uint32_t getblock32(const uint32_t* p, int i)
{
	return p[i];
}

#define ROTL32(x,y)	_rotl(x,y)

FORCEINLINE uint32_t fmix32(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

FORCEINLINE void MurmurHash3_x86_32(const void* key, int len,
	uint32_t seed, void* out)
{
	const uint8_t* data = (const uint8_t*)key;
	const int nblocks = len / 4;

	uint32_t h1 = seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	//----------
	// body

	const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

	for (int i = -nblocks; i; i++)
	{
		uint32_t k1 = getblock32(blocks, i);

		k1 *= c1;
		k1 = ROTL32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = ROTL32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	//----------
	// tail

	const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

	uint32_t k1 = 0;

	switch (len & 3)
	{
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len;

	h1 = fmix32(h1);

	*(uint32_t*)out = h1;
}


FORCEINLINE uint32 HashFunction(const void* key, int len, uint32 seed)
{
	uint32 result;
	MurmurHash3_x86_32(key, len, seed, &result);
	return result;
}

FORCEINLINE uint32 GetTypeHash(const FNode::Node& Node)
{
	return HashFunction(&(Node[0]), sizeof(uint32_t) * 8, 0);
}
