#include "globalhelpers.h"
#include "ai_mem.h"
#include <immintrin.h>
#include <cstdlib>

#ifndef MEM_USE_64
mask_type getmask(unsigned __int32 number) {
	static mask_type lookup[32] = {
		0x00000000,
		0x00000001,
		0x00000003,
		0x00000007,
		0x0000000F,
		0x0000001F,
		0x0000003F,
		0x0000007F,
		0x000000FF,
		0x000001FF,
		0x000003FF,
		0x000007FF,
		0x00000FFF,
		0x00001FFF,
		0x00003FFF,
		0x00007FFF,
		0x0000FFFF,
		0x0001FFFF,
		0x0003FFFF,
		0x0007FFFF,
		0x000FFFFF,
		0x001FFFFF,
		0x003FFFFF,
		0x007FFFFF,
		0x00FFFFFF,
		0x01FFFFFF,
		0x03FFFFFF,
		0x07FFFFFF,
		0xFFFFFFFF,
		0x1FFFFFFF,
		0x3FFFFFFF,
		0x7FFFFFFF
	};
	return lookup[number];
}
#else
mask_type getmask(unsigned __int32 number) {
	static mask_type lookup[64] = {
		0x0000000000000000,
		0x0000000000000001,
		0x0000000000000003,
		0x0000000000000007,
		0x000000000000000F,
		0x000000000000001F,
		0x000000000000003F,
		0x000000000000007F,
		0x00000000000000FF,
		0x00000000000001FF,
		0x00000000000003FF,
		0x00000000000007FF,
		0x0000000000000FFF,
		0x0000000000001FFF,
		0x0000000000003FFF,
		0x0000000000007FFF,
		0x000000000000FFFF,
		0x000000000001FFFF,
		0x000000000003FFFF,
		0x000000000007FFFF,
		0x00000000000FFFFF,
		0x00000000001FFFFF,
		0x00000000003FFFFF,
		0x00000000007FFFFF,
		0x0000000000FFFFFF,
		0x0000000001FFFFFF,
		0x0000000003FFFFFF,
		0x0000000007FFFFFF,
		0x00000000FFFFFFFF,
		0x000000001FFFFFFF,
		0x000000003FFFFFFF,
		0x000000007FFFFFFF,
		0x00000000FFFFFFFF,
		0x00000001FFFFFFFF,
		0x00000003FFFFFFFF,
		0x00000007FFFFFFFF,
		0x0000000FFFFFFFFF,
		0x0000001FFFFFFFFF,
		0x0000003FFFFFFFFF,
		0x0000007FFFFFFFFF,
		0x000000FFFFFFFFFF,
		0x000001FFFFFFFFFF,
		0x000003FFFFFFFFFF,
		0x000007FFFFFFFFFF,
		0x00000FFFFFFFFFFF,
		0x00001FFFFFFFFFFF,
		0x00003FFFFFFFFFFF,
		0x00007FFFFFFFFFFF,
		0x0000FFFFFFFFFFFF,
		0x0001FFFFFFFFFFFF,
		0x0003FFFFFFFFFFFF,
		0x0007FFFFFFFFFFFF,
		0x000FFFFFFFFFFFFF,
		0x001FFFFFFFFFFFFF,
		0x003FFFFFFFFFFFFF,
		0x007FFFFFFFFFFFFF,
		0x00FFFFFFFFFFFFFF,
		0x01FFFFFFFFFFFFFF,
		0x03FFFFFFFFFFFFFF,
		0x07FFFFFFFFFFFFFF,
		0xFFFFFFFFFFFFFFFF,
		0x1FFFFFFFFFFFFFFF,
		0x3FFFFFFFFFFFFFFF,
		0x7FFFFFFFFFFFFFFF
	};
	return lookup[number];
}
#endif

void ai_mem_pool::cleanuppool() {
	for (auto & pr : memory) {
		if (pr.second != 0)
			free(pr.first);
	}
	memory.resize(0);
}

void ai_mem_pool::init(const void* newmem, size_t size) {
	cleanuppool();
	if (size == 0)
		return;

	size_t totalblocks = size / sizeof(ai_mem_block);
	ai_mem_block* cpy = (ai_mem_block*)malloc(size);
	if (cpy) {
		memcpy(cpy, newmem, size);
		for (size_t indx = 0; indx < totalblocks; ++indx) {
			memory.emplace_back(cpy + indx, 0);
		}
		if (memory.size() > 0)
			memory[0].second = static_cast<unsigned int>(totalblocks);
	} else {

	}
}

void* ai_mem_pool::compact() {
	bool needscompact = memory.size() > 0 ? memory[0].second != memory.size() : false;
	
	if (!needscompact) {
		if (memory.size() > 0)
			return memory[0].first;
		else
			return nullptr;
	}

	//ai_mem_block* cpy = (ai_mem_block*)malloc(memory.size()*sizeof(ai_mem_block));
	ai_mem_block* cpy = (ai_mem_block*)realloc(memory[0].first, memory.size()*sizeof(ai_mem_block));
	if (cpy) {
		unsigned int offset = memory[0].second;
		memory[0].second = 0;
		while (offset < memory.size()) {
			memcpy(cpy + offset, memory[offset].first, memory[offset].second * sizeof(ai_mem_block));
			offset += memory[offset].second;
		}
		for (auto & pr : memory) {
			if (pr.second != 0)
				free(pr.first);
			pr.second = 0;
		}
		memory[0].first = cpy;
		memory[0].second = static_cast<unsigned short>(memory.size());
		for (offset = 1; offset < memory.size(); ++offset) {
			memory[offset].first = memory[0].first + offset;
		}
	} else {
		memory.clear();
	}

	return cpy;
}

ai_mem_pool::~ai_mem_pool() {
	cleanuppool();
}

mask_type extractN(mask_type source, unsigned __int32 offset, unsigned __int32 count) {
	return (source >> offset) & getmask(count);
}

blockposition ai_alloc(const size_t size, ai_mem_pool &mem) {
	const size_t blocks = (size + sizeof(s_block) - 1) / sizeof(s_block);
	if (blocks <= SUB_BLOCKS) {
		//mask_type mask = ___blsmsk(0x1 << (blocks - 1));
		mask_type mask = getmask(static_cast<unsigned int>(blocks));
		for (size_t indx = 0; indx < mem.memory.size(); ++indx) {
			if ((mem.memory[indx].first->allocationmask & SPECIAL_MASK) != 0) {
				indx += ((mem.memory[indx].first->allocationmask & ~SPECIAL_MASK) - 1);
			}
			else {
				for (unsigned int off = 0; off <= SUB_BLOCKS - (blocks); ++off) {
					//if (_bextr_u32(mem[indx].alprovincemask, off, blocks) == 0) {
					if (((mem.memory[indx].first->allocationmask >> off) & mask) == 0) {
						mem.memory[indx].first->allocationmask |= (mask << off);
						return blockposition(static_cast<unsigned char>(indx), static_cast<unsigned char>(off), static_cast<unsigned short>(blocks));
					}
				}
			}
		}	
		//not enough space
		mem.memory.emplace_back((ai_mem_block*)calloc(1, sizeof(ai_mem_block)), 1);
		mem.memory.back().first->allocationmask = mask;
		return blockposition(static_cast<unsigned char>(mem.memory.size() - 1), 0, static_cast<unsigned short>(blocks));
	}
	else {
		const size_t fullblocks = (blocks + 1 + SUB_BLOCKS) / (SUB_BLOCKS + 1);
		bool found = false;
		for (size_t indx = 0; !found && indx < mem.memory.size() - (fullblocks - 1); ++indx) {
			if ((mem.memory[indx].first->allocationmask & SPECIAL_MASK) != 0) {
				indx += ((mem.memory[indx].first->allocationmask & ~SPECIAL_MASK) - 1);
			}
			else if (mem.memory[indx].first->allocationmask == 0) {
				if (mem.memory[indx].second >= fullblocks) {
					mem.memory[indx].first->allocationmask = SPECIAL_MASK | fullblocks;
					return blockposition(static_cast<unsigned char>(indx), 0, static_cast<unsigned short>(blocks));
				}
			}
		}
		//not enough space;
		
		ai_mem_block* newblk;
		mem.memory.emplace_back(newblk = (ai_mem_block*)calloc(fullblocks, sizeof(ai_mem_block)), static_cast<unsigned short>(fullblocks));
		mem.memory.back().first->allocationmask = SPECIAL_MASK | fullblocks;
		const unsigned char blockpos = static_cast<unsigned char>(mem.memory.size() - 1);
		for (size_t inn = 0; inn < fullblocks - 1; inn++) {
			mem.memory.emplace_back(newblk+1+inn, 0);
		}
		return blockposition(static_cast<unsigned char>(blockpos), 0, static_cast<unsigned short>(blocks));
	}

	return blockposition();
}



void ai_free(blockposition& pos, ai_mem_pool &mem) {
	if (pos.size == 0 || pos.blocknumber >= mem.memory.size())
		return;
	if (pos.size <= SUB_BLOCKS) {
		//mask_type mask = ___blsmsk(0x1 << (blocks - 1));
		mask_type mask = getmask(pos.size);
		mem.memory[pos.blocknumber].first->allocationmask &= ~(mask << pos.blockoffset);
	}
	else {
		const size_t fullblocks = (pos.size + 1 + SUB_BLOCKS) / (SUB_BLOCKS + 1);
		for (unsigned int indx = 0; indx < fullblocks && pos.blocknumber + indx < mem.memory.size(); ++indx) {
			mem.memory[pos.blocknumber + indx].first->allocationmask = 0;
		}
	}

	pos.size = 0;


	for (__int64 bk = mem.memory.size() - 1; bk >= 0; --bk) {
		if (mem.memory[bk].second != 0) {
			if (mem.memory[bk].first->allocationmask != 0) {
				mem.memory.resize(bk + mem.memory[bk].second);
				return;
			}
			 else {
				free(mem.memory[bk].first);
				 mem.memory[bk].first = nullptr;
			}
		}
	}
	mem.memory.clear();
	
}

blockposition ptrToPosition(void *ptr, const size_t size, const ai_mem_pool &mem) {
	if (mem.memory.size() == 0)
		return blockposition();
	size_t blocknumber = 0;
	for (unsigned int i = 0; i < mem.memory.size(); ++i) {
		if (ptr > mem.memory[i].first && ptr < mem.memory[i].first + 1)
			blocknumber = i;
	}
	size_t blockpos = (size_t)((BYTE*)ptr - (BYTE*)(mem.memory[blocknumber].first->data)) / sizeof(s_block);
	return blockposition(static_cast<unsigned char>(blocknumber), static_cast<unsigned char>(blockpos), static_cast<unsigned short>(size));
}


void* posToPtr_v(const blockposition &b, const ai_mem_pool &mem) {
	if (b.size == 0 || b.blocknumber >= mem.memory.size())
		return nullptr;
	return (void*)&(mem.memory[b.blocknumber].first->data[b.blockoffset]);
}