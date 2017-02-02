#pragma once
#include "globalhelpers.h"
#include <vector>

#define MEM_USE_64

#ifndef MEM_USE_64
#define SUB_BLOCKS 31
using mask_type = unsigned __int32;
#define SPECIAL_MASK 0x80000000
mask_type getmask(unsigned __int32 number);
#else
using mask_type = unsigned __int64;
#define SUB_BLOCKS 63
#define SPECIAL_MASK 0x8000000000000000
mask_type getmask(unsigned __int32 number);
#endif

using s_block = unsigned __int64;

struct ai_mem_block {
	mask_type allocationmask;
	s_block data[SUB_BLOCKS];
	ai_mem_block() : allocationmask(0) {};
	ai_mem_block(mask_type am) : allocationmask(am) {};
};

struct blockposition {
	unsigned char blocknumber;
	unsigned char blockoffset;
	unsigned short size;
	blockposition() : blocknumber(0), blockoffset(0), size(0) {};
	blockposition(unsigned char bn, unsigned char bo, unsigned short sz) : blocknumber(bn), blockoffset(bo), size(sz) {};
};


class ai_mem_pool {
public:
	std::vector < std::pair<ai_mem_block*, unsigned short> > memory;
	void init(const void* newmem, size_t size);
	void* compact();
	void cleanuppool();
	~ai_mem_pool();
	ai_mem_pool() {};
	ai_mem_pool(const ai_mem_pool& in) = delete;
};

blockposition ai_alloc(const size_t size, ai_mem_pool &mem);

template<typename T, typename ...PARAMS >
inline blockposition ai_alloc(ai_mem_pool &mem, PARAMS&& ... params) {
	//using basetype = std::remove_reference <std::remove_const< T >>;
	blockposition b = ai_alloc(sizeof(T), mem);
	if (b.size != 0) {
		new  (&(mem.memory[b.blocknumber].first->data[b.blockoffset])) T(std::forward<PARAMS>(params) ...);
	}
	return b;
}

blockposition ptrToPosition(void *ptr, const size_t size, const ai_mem_pool &mem);
void* posToPtr_v(const blockposition &b, const ai_mem_pool &mem);

template<typename T>
inline T* posToPtr(const blockposition &b, const ai_mem_pool &mem) {
	return (T*)posToPtr_v(b, mem);
}

void ai_free(blockposition& pos, ai_mem_pool &mem);

template<typename T>
inline void ai_free(blockposition& pos, ai_mem_pool &mem) {
	if (pos.size == 0 || pos.blocknumber >= mem.memory.size())
		return;
	posToPtr<T>(pos, mem)->~T();
	ai_free(pos, mem);
}

template<typename T>
inline blockposition ptrToPosition(T *ptr, const ai_mem_pool &mem) {
	return ptrToPosition(ptr, sizeof(T), mem);
}

template<typename T>
class bp_wrapper;
template<typename T>
class smart_bp;

template<typename T>
class _ptr_ready_wrapper  {
protected:
	blockposition bp;
	ai_mem_pool* m;
	_ptr_ready_wrapper(const blockposition& b, ai_mem_pool* mem) : bp(b), m(mem) {};
	_ptr_ready_wrapper(const _ptr_ready_wrapper<T>& wrp) = delete;
public:
	T& operator*() const {
		return *posToPtr<T>(bp, *m);
	}
	T* operator->() const {
		return posToPtr<T>(bp, *m);
	}
	operator T*() const {
		return posToPtr<T>(bp, *m);
	}
	_ptr_ready_wrapper(_ptr_ready_wrapper<T>&& wrp) : bp(wrp.bp), m(wrp.m) {
		wrp.m = nullptr;
	};
	friend class bp_wrapper<T>;
};

template<typename T>
class bp_wrapper {
private:
	blockposition bp;
public:
	bp_wrapper() {};
	bp_wrapper(const blockposition &b) : bp(b) {};
	bp_wrapper& operator=(const blockposition &b) {
		bp = b;
		return *this;
	}
	_ptr_ready_wrapper<T> operator() (ai_mem_pool &mem) {
		return _ptr_ready_wrapper<T>(bp, &mem);
	}
	operator blockposition&() {
		return bp;
	}
};

template<typename T>
class _ptr_ready_wrapper_s;

template<typename T>
class smart_bp {
protected:
	class _data {
	protected:
		unsigned short refcount;
		T data;
		template<typename ... PARAMS>
		_data(PARAMS&& ... params) : refcount(1), data(std::forward<PARAMS>(params) ...) {}
		friend class _ptr_ready_wrapper_s<T>;
	};
	blockposition bp;
	bool released = false;
	smart_bp() {};
public:
	template<typename ... PARAMS>
	static smart_bp make_shared_bp(ai_mem_pool &mem, PARAMS ... params) {
		bp = ai_alloc<_data>(mem, std::forward<PARAMS>(params) ...);
	}

	smart_bp(const smart_bp<T> &orig, ai_mem_pool &mem) : bp(orig.bp) {
		++posToPtr<dataclass>(bp, mem)->refcount;
	};

	smart_bp(const smart_bp<T> &&orig) : bp(orig.bp) {
		orig.bp.size = 0;
		orig.released = true;
	};

	smart_bp& operator=(smart_bp<T> &&orig) {
		bp = orig.bp;
		orig.bp.size = 0;
		orig.released = true;
		return *this;
	}

	_ptr_ready_wrapper_s<T> operator() (ai_mem_pool &mem);

	void release(ai_mem_pool &mem) {
		if (posToPtr<dataclass>(bp, mem)->refcount == 1) {
			ai_free<T>(bp, mem);
		}
		else {
			--posToPtr<dataclass>(bp, mem)->refcount;
		}
		bp.size = 0;
		released = true;
	}

	~smart_bp() {
		assert(released);
	}

	friend class _ptr_ready_wrapper_s < T >;
};

template<typename T>
class _ptr_ready_wrapper_s  {
	typename smart_bp<T>::_data dataclass;
protected:
	blockposition& bp;
	ai_mem_pool* m;
	_ptr_ready_wrapper_s(blockposition& b, ai_mem_pool* mem) : bp(b), m(mem) {
		++posToPtr<dataclass>(bp, *mem)->refcount;
	};
public:
	_ptr_ready_wrapper_s(const _ptr_ready_wrapper_s<T>& wrp) : m(in.m), bp(in.bp) {
		++posToPtr<dataclass>(bp, *mem)->refcount;
	}
	_ptr_ready_wrapper_s(_ptr_ready_wrapper_s<T> &&in) : m(in.m), bp(in.bp) {
		in.bp.size = 0;
	}
	_ptr_ready_wrapper_s<T>& operator=(_ptr_ready_wrapper_s<T> &&in) {
		m = in.m;
		bp = in.bp;
		in.bp.size = 0;
	}
	_ptr_ready_wrapper_s<T>& operator=(const _ptr_ready_wrapper_s<T> &in) {
		m = in.m;
		bp = in.bp;
		++posToPtr<dataclass>(bp, *m)->refcount;
	}
	T& operator*() const {
		return posToPtr<dataclass>(bp, *m)->data;
	}
	T* operator->() const {
		return *(&posToPtr<dataclass>(bp, *m)->data);
	}
	operator T*() const {
		return *(&posToPtr<dataclass>(bp, *m)->data);
	}
	~_ptr_ready_wrapper_s() {
		if (posToPtr<dataclass>(bp, *m)->refcount == 1) {
			ai_free<T>(bp, *m);
		}
		else {
			--posToPtr<dataclass>(bp, *m)->refcount;
		}
	}
	friend class smart_bp<T>;
};

template<typename T>
_ptr_ready_wrapper_s<T> smart_bp<T>::operator() (ai_mem_pool &mem) {
	return _ptr_ready_wrapper_s<T>(bp, &mem);
}

