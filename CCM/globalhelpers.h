#pragma once
#pragma  warning(disable:4100)
#pragma  warning(disable:4189)
#pragma  warning(disable:4201)
#pragma  warning(disable:4505)

// #define CCL_NO_EXECPTIONS

#ifdef CCL_NO_EXECPTIONS
#pragma  warning(disable:4577)
#define BOOST_NO_EXCEPTIONS
#define BOOST_EXCEPTION_DISABLE
#define _HAS_EXCEPTIONS 0
#endif

#include <intrin.h>
#include <immintrin.h>
#include <math.h>

#pragma intrinsic(acos, cosh, pow, tanh, asin, fmod, sinh, atan, exp, log10, sqrt, atan2, log, sin, tan, cos)
#pragma intrinsic(_mul128, _InterlockedAnd, _InterlockedAnd64, _InterlockedCompareExchange, _InterlockedCompareExchange64, _InterlockedCompareExchangePointer, _InterlockedDecrement, _InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange, _InterlockedExchange64, _InterlockedExchangeAdd, _InterlockedExchangeAdd64, _InterlockedExchangePointer, _InterlockedIncrement, _InterlockedIncrement64, _InterlockedOr)
#pragma intrinsic(_InterlockedOr64, _InterlockedXor, _InterlockedXor64, _BitScanReverse64, _bittest64, _rdrand64_step)

#define GLEW_STATIC
#include "gl\glew.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_INLINE
#include <glm\glm.hpp>
#include <glm\gtc\matrix_inverse.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <SFML\Graphics.hpp>
#include <SFML\OpenGL.hpp>
#include <utility>
#include <list>
#include <deque>
#include <vector>
#include <array>
#include <string>
#include <Windows.h>


#pragma  warning(push)
#pragma  warning(disable:4701)
#include "sqlite3.h"
#pragma  warning(pop)

#include <tuple>
#include <functional>
#include <concrt.h>
#include <ppltasks.h>
#include <ppl.h>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <type_traits>
#include <algorithm>
#include <random>
#include <numeric>
#include "boost/date_time/gregorian/gregorian_types.hpp"
#include "SOIL.h"
#include "boost/container/flat_map.hpp"
#include "boost/container/flat_set.hpp"
#include "boost/container/small_vector.hpp"
#include "boost/random/mersenne_twister.hpp"
#include "boost/range/algorithm_ext/erase.hpp"

#define HAS_PEOPLE

using namespace boost::container;

#undef IN

#define IN(...) const __VA_ARGS__ & __restrict
#define INOUT(...) __VA_ARGS__ & __restrict
#define IN_P(...) __VA_ARGS__ * const __restrict

#define SGN(x) (((x) > 0) - ((x) < 0))

extern "C" __int64 __fastcall asm_div(__int64 lo, __int64 hi, __int64 d);

void LoadDDS(INOUT(sf::Texture) tex, IN_P(char) file);
void load_soil_image(INOUT(sf::Image) img, IN_P(char) file);
void MergeRects(INOUT(sf::IntRect) a, IN(sf::IntRect) b);
void RawGraphicsFinish();
void RawGraphicsSetUp(IN(sf::IntRect) viewport, IN(sf::View) view, int sizey);
sf::Color get_color_from_raw(IN_P(unsigned char) imgdata, int x, int y, int w, int ch) noexcept;

template<typename T>
class max_value : public std::integral_constant<T, std::numeric_limits<T>::max()> {};

template<>
class max_value<double> {
public:
	using value_type = double;
	using type = max_value<double>;
	static constexpr double value = DBL_MAX;
	constexpr operator double() const {
		return value;
	}
	constexpr double operator()() const {
		return value;
	}
};

template<>
class max_value<float> {
public:
	using value_type = float;
	using type = max_value<float>;
	static constexpr float value = FLT_MAX;
	constexpr operator float() const {
		return value;
	}
	constexpr float operator()() const {
		return value;
	}
};


template<typename T>
class min_value : public std::integral_constant<T, std::numeric_limits<T>::min()> {};

template<>
class min_value<double> {
public:
	using value_type = double;
	using type = max_value<double>;
	static constexpr double value = -DBL_MAX;
	constexpr operator double() const {
		return value;
	}
	constexpr double operator()() const {
		return value;
	}
};

template<>
class min_value<float> {
public:
	using value_type = float;
	using type = max_value<float>;
	static constexpr float value = -FLT_MAX;
	constexpr operator float() const {
		return value;
	}
	constexpr float operator()() const {
		return value;
	}
};

using prov_id = unsigned short;
using char_id = unsigned int;
using title_id = unsigned short;
using dyn_id = unsigned short;
using rel_id = unsigned short;
using cul_id = unsigned short;
using war_id = unsigned short;
using admin_id = unsigned short;
using pact_id = unsigned short;


using SM_TAG_TYPE = unsigned short;

#define STD_TAG_FUNCTIONS(type) constexpr type () noexcept : value(NONE) {}; \
	explicit constexpr type (decltype(value) v) noexcept : value(v) {}; \
	type & operator=(decltype(value) o) noexcept { value = o; return *this; }; \
	constexpr bool operator==(type o) const noexcept { return value == o.value; }; \
	constexpr bool operator!=(type o) const noexcept { return value != o.value; }; \
	constexpr bool operator<(type o) const noexcept { return value < o.value; };


struct prov_id_t {
public:
	static constexpr prov_id NONE = 0;
	prov_id value;
	STD_TAG_FUNCTIONS(prov_id_t)
};


struct char_id_t {
public:
	static constexpr char_id NONE = 0;
	char_id value;
	STD_TAG_FUNCTIONS(char_id_t)
};

struct title_id_t {
public:
	static constexpr title_id NONE = 0;
	title_id value;
	STD_TAG_FUNCTIONS(title_id_t)
};

struct dyn_id_t {
public:
	static constexpr dyn_id NONE = 0;
	dyn_id value;
	STD_TAG_FUNCTIONS(dyn_id_t)
};

struct rel_id_t {
public:
	static constexpr rel_id NONE = 0;
	rel_id value;
	STD_TAG_FUNCTIONS(rel_id_t)
};

struct cul_id_t {
public:
	static constexpr cul_id NONE = 0;
	cul_id value;
	STD_TAG_FUNCTIONS(cul_id_t)
};

struct war_id_t {
public:
	static constexpr war_id NONE = max_value<war_id>::value;
	war_id value;
	STD_TAG_FUNCTIONS(war_id_t)
};

struct admin_id_t {
public:
	static constexpr admin_id NONE = max_value<admin_id>::value;
	admin_id value;
	STD_TAG_FUNCTIONS(admin_id_t)
};

struct pact_id_t {
public:
	static constexpr pact_id NONE = max_value<pact_id>::value;
	pact_id value;
	STD_TAG_FUNCTIONS(pact_id_t)
};

template<typename T>
bool valid_ids(T id) {
	return id.value != T::NONE;
}

template<typename T, typename ... R>
bool valid_ids(T id, R ... rest) {
	return (id.value != T::NONE) & valid_ids(rest ...);
}


#define MAP_MODE_POL 0
#define MAP_MODE_VAS 1
#define MAP_MODE_ECON 2
#define MAP_MODE_WAR 3
#define MAP_MODE_WARSEL 4
#define MAP_MODE_CULTURE 5
#define MAP_MODE_RELIGION 6
#define MAP_MODE_DISTANCE 7

#define EMPIRE_TYPE 1
#define KINGDOM_TYPE 2
#define DUTCHY_TYPE 3
#define COUNTY_TYPE 4

#define EMPIRE_TYPE_S "1"
#define KINGDOM_TYPE_S "2"
#define DUTCHY_TYPE_S "3"
#define COUNTY_TYPE_S "4"

using namespace concurrency;

#define LoadDeferred(tex, file, globals) _LoadDeferred(tex, file, globals)


namespace global {
	extern reader_writer_lock ogll;
	extern reader_writer_lock index_lock;
}

template<typename T, typename U>
class type_or {
public:
	using type = std::true_type;
};
template<>
class type_or<std::false_type, std::false_type> {
public:
	using type = std::false_type;
};
template<typename T, typename U>
using type_or_t = typename type_or<T, U>::type;

template<typename T, typename U>
class type_and {
public:
	using type = std::false_type;
};
template<>
class type_and<std::true_type, std::true_type> {
public:
	using type = std::true_type;
};
template<typename T, typename U>
using type_and_t = typename type_and<T, U>::type;

template<bool b, typename A, typename B>
class type_if;

template<typename A, typename B>
class type_if<true, A, B> {
public:
	using type = A;
};
template<typename A, typename B>
class type_if<false, A, B> {
public:
	using type = B;
};

template<bool b, typename A, typename B>
using type_if_t = typename type_if<b, A, B>::type;

template<typename T>
struct sfinae_true : std::true_type {};

template<typename F, typename R, typename ... T>
std::integral_constant<size_t, sizeof...(T)> number_of_arguments(F& f, R(F::*mf)(T ...) const) {
	return std::integral_constant<size_t, sizeof...(T)>();
}

template<typename F>
auto number_of_arguments(F f) {
	return number_of_arguments(f, &F::operator());
}

template<typename F, typename ... T>
auto apply_n(F&& f, std::integral_constant<size_t, 0>, T&& ... t) {
	return std::forward<F>(f)();
};

template<typename F, typename A, typename ... T>
auto apply_n(F&& f, std::integral_constant<size_t, 1>, A&& a, T&& ... t) {
	return std::forward<F>(f)(std::forward<A>(a));
};

template<typename F, typename A, typename B, typename ... T>
auto apply_n(F&& f, std::integral_constant<size_t, 2>, A&& a, B&& b, T&& ... t) {
	return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b));
};

template<typename F, typename A, typename B, typename C, typename ... T>
auto apply_n(F&& f, std::integral_constant<size_t, 3>, A&& a, B&& b, C&& c, T&& ... t) {
	return std::forward<F>(f)(std::forward<A>(a), std::forward<B>(b), std::forward<C>(c));
};

template<typename F, typename ... T>
auto auto_apply(F&& f, T&& ... t) {
	return apply_n(std::forward<F>(f), decltype(number_of_arguments(f))(), std::forward<T>(t) ...);
}

template<typename T>
T _TInterlockedCompareExchange(T* address, T newvalue, T test, std::integral_constant<size_t, 16>) {
	return InterlockedCompareExchange128(address, newvalue, test);
}

template<typename T>
T _TInterlockedCompareExchange(T* address, T newvalue, T test, std::integral_constant<size_t, 8>) {
	return InterlockedCompareExchange64(address, newvalue, test);
}

template<typename T>
T _TInterlockedCompareExchange(T* address, T newvalue, T test, std::integral_constant<size_t, 2>) {
	return InterlockedCompareExchange16((short*)address, newvalue, test);
}

template<typename T>
T _TInterlockedCompareExchange(T* address, T newvalue, T test, std::integral_constant<size_t, 4>) {
	return InterlockedCompareExchange(address, newvalue, test);
}

template<typename T>
T TInterlockedCompareExchange(T* address, T newvalue, T test) {
	return _TInterlockedCompareExchange(address, newvalue, test, std::integral_constant<size_t, sizeof(T)>());
}

template<typename T>
T _TInterlockedIncrement(T* address, T newvalue, T test, std::integral_constant<size_t, 16>) {
	return InterlockedIncrement128(address, newvalue, test);
}

template<typename T>
T _TInterlockedIncrement(T* address, std::integral_constant<size_t, 8>) {
	return InterlockedIncrement64(address);
}

template<typename T>
T _TInterlockedIncrement(T* address, std::integral_constant<size_t, 2>) {
	return InterlockedIncrement16((short*)address);
}

template<typename T>
T _TInterlockedIncrement(T* address, std::integral_constant<size_t, 4>) {
	return InterlockedIncrement(address);
}

template<typename T>
T TInterlockedIncrement(T* address) {
	return _TInterlockedIncrement(address, std::integral_constant<size_t, sizeof(T)>());
}

template<reader_writer_lock* lk>
class OGLLock_t {
public:
	sf::RenderTarget* __restrict rt;
	OGLLock_t(IN_P(sf::RenderTarget) t): rt(t) {
		rt->_setActive(true);
		lk->lock();
	}
	OGLLock_t(const OGLLock_t&) = delete;
	OGLLock_t(OGLLock_t &&source) : rt(std::move(source.rt)) { source.rt = nullptr; };
	~OGLLock_t() {
		if (rt) {
			rt->_setActive(false);
			lk->unlock();
		}
	}
	inline OGLLock_t& operator=(OGLLock_t &&source) {
		rt = source.rt;
		source.rt = nullptr;
	};
	inline void lock() noexcept { rt->_setActive(true); lk->lock();};
	inline void release() noexcept { rt->_setActive(false); lk->unlock(); };
	sf::RenderTarget* operator->() const {
		return rt;
	}
};

using OGLLock = OGLLock_t<&global::ogll>;

template<reader_writer_lock* lk>
class g_lock_t {
protected:
	g_lock_t() {};
public:
	g_lock_t(IN(g_lock_t)) {};
};

template<reader_writer_lock* lk>
class r_lock_t : public g_lock_t<lk> {
protected:
public:
	r_lock_t() {
		lk->lock_read();
	}
	r_lock_t(const r_lock_t& l) = delete;
	r_lock_t(r_lock_t&& in) = delete;
	const r_lock_t& operator =(r_lock_t&& in) = delete;

	~r_lock_t() {
		lk->unlock();
	}
	static void lock() {
		lk->lock_read();
	}
	static void unlock() {
		lk->unlock();
	}
};

template<reader_writer_lock* lk>
class fake_lock_t : public g_lock_t<lk> {
public:
	fake_lock_t() {};
	static void lock() {
	}
	static void unlock() {
	}
};

template<reader_writer_lock* lk>
class w_lock_t : public g_lock_t<lk> {
protected:
public:
	w_lock_t() {
		lk->lock();
	}
	w_lock_t(const w_lock_t& l) = delete;
	w_lock_t(w_lock_t&& in) = delete;
	const w_lock_t& operator =(w_lock_t&& in) = delete;

	~w_lock_t() {
		lk->unlock();
	}
	static void lock() {
		lk->lock();
	}
	static void unlock() {
		lk->unlock();
	}
};

using r_lock = r_lock_t<&global::index_lock>;
using w_lock = w_lock_t<&global::index_lock>;
using g_lock = g_lock_t<&global::index_lock>;
using fake_lock = fake_lock_t<&global::index_lock>;

template<typename T>
class comparison_from_lt {
public:
	template<typename O>
	bool operator==(type_if_t<std::is_scalar_v<O>, O, std::add_const_t<std::add_lvalue_reference_t<O>>> oth) const noexcept {
		return !(*((T*)this) < oth) && !(oth < *((T*)this));
	}
	template<typename O>
	bool operator!=(type_if_t<std::is_scalar_v<O>, O, std::add_const_t<std::add_lvalue_reference_t<O>>> oth) const noexcept {
		return !(*this == oth);
	}
	template<typename O>
	bool operator>(type_if_t<std::is_scalar_v<O>, O, std::add_const_t<std::add_lvalue_reference_t<O>>> oth) const noexcept {
		return oth < *((T*)this)
	}
	template<typename O>
	bool operator<=(type_if_t<std::is_scalar_v<O>, O, std::add_const_t<std::add_lvalue_reference_t<O>>> oth) const noexcept {
		return !(*((T*)this) > oth);
	}
	template<typename O>
	bool operator>=(type_if_t<std::is_scalar_v<O>, O, std::add_const_t<std::add_lvalue_reference_t<O>>> oth) const noexcept {
		return !(*((T*)this) < oth);
	}
};

template<typename T>
class SList
{
private:
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) SLIST_HEADER stack_head;

	struct __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) node
	{
		SLIST_ENTRY slist_entry;
		T obj;
	};

public:
	SList() {
		InitializeSListHead(&stack_head);
	}
	~SList() {
		clear();
	}
	template<typename ... P>
	bool push(P&& ... params) {
		node* __restrict pNode = (node*)_aligned_malloc(sizeof(node), MEMORY_ALLOCATION_ALIGNMENT);
		if (!pNode)
			return false;
		new (static_cast<void*>(&pNode->obj)) T(std::forward<P>(params)...);
		InterlockedPushEntrySList(&stack_head, &pNode->slist_entry);
		return true;
	}

	bool try_pop(INOUT(T) result) {
		node* __restrict pNode = (node*)InterlockedPopEntrySList(&stack_head);
		if (!pNode)
			return false;
		result = std::move(pNode->obj);
		pNode->obj.~T();
		_aligned_free(pNode);
		return true;
	}
	void clear() {
		node* __restrict pNode = (node*)InterlockedPopEntrySList(&stack_head);
		while (pNode) {
			pNode->obj.~T();
			_aligned_free(pNode);
			pNode = (node*)InterlockedPopEntrySList(&stack_head);
		}
	}
};

template<int mod, typename T>
class modulus_iterator : public std::iterator<std::random_access_iterator_tag, T> {
public:
	typedef std::random_access_iterator_tag iterator_category;
	typedef modulus_iterator<mod, T> _Myiter;
	typedef const T* _Tptr;
	typedef const T& reference;
	typedef const T* pointer;

	modulus_iterator()
		: _Ptr() {
	}
	modulus_iterator(_Tptr _Parg)
		: _Ptr(_Parg) {
	}

	template<typename IT>
	explicit modulus_iterator(IN(IT) in) : _Ptr(&(*in)) {};

	reference operator*() const {
		_Analysis_assume_(this->_Ptr != _Tptr());
		return (*this->_Ptr);
	}
	pointer operator->() const {	// return pointer to class object
		return (::std::pointer_traits<pointer>::pointer_to(**this));
	}
	_Myiter& operator++() {
		_Ptr += mpd;
		return (*this);
	}
	_Myiter operator++(int) {	// postincrement
		_Myiter _Tmp(*this);
		_Ptr += mpd;
		return (_Tmp);
	}
	_Myiter& operator--() {	// predecrement
		_Ptr -= mod;
		return (*this);
	}
	_Myiter operator--(int) {	// postdecrement
		_Myiter _Tmp = *this;
		_Ptr -= mod;
		return (_Tmp);
	}
	_Myiter& operator+=(difference_type _Off) {	// increment by integer
		_Ptr += (_Off * mod);
		return (*this);
	}
	_Myiter operator+(difference_type _Off) const {	// return this + integer
		_Myiter _Tmp(*this);
		return (_Tmp += _Off);
	}

	_Myiter& operator-=(difference_type _Off) {	// decrement by integer
		_Ptr -= (_Off * mod);
		return (*this);
	}

	_Myiter operator-(difference_type _Off) const {	// return this - integer
		_Myiter _Tmp(*this);
		return (_Tmp -= _Off);
	}

	difference_type operator-(const _Myiter& _Right) const {
		return (_Ptr - _Right._Ptr) / mod;
	}

	reference operator[](difference_type _Off) const {	// subscript
		return *(_Ptr + _Off*mod);
	}

	bool operator==(const _Myiter& _Right) const {	// test for iterator equality
		return _Ptr == _Right._Ptr;
	}

	bool operator!=(const _Myiter& _Right) const {	// test for iterator inequality
		return !(*this == _Right);
	}

	bool operator<(const _Myiter& _Right) const {	// test if this < _Right
		return _Ptr < _Right._Ptr;
	}

	bool operator>(const _Myiter& _Right) const {	// test if this > _Right
		return _Right < *this;
	}

	bool operator<=(const _Myiter& _Right) const {	// test if this <= _Right
		return !(_Right < *this);
	}

	bool operator>=(const _Myiter& _Right) const {	// test if this >= _Right
		return !(*this < _Right);
	}

	_Tptr _Ptr;
};

template<int step, typename T = int>
class counting_iterator : public std::iterator<std::random_access_iterator_tag, T> {
public:
	typedef std::random_access_iterator_tag iterator_category;
	typedef counting_iterator<step, T> _Myiter;
	typedef T _Tptr;
	typedef T reference;
	typedef T* pointer;

	counting_iterator()
		: _Ptr() {
	}
	counting_iterator(_Tptr _Parg)
		: _Ptr(_Parg) {
	}

	reference operator*() const {
		_Analysis_assume_(this->_Ptr != _Tptr());
		return (this->_Ptr);
	}

	_Myiter& operator++() {
		_Ptr += step;
		return (*this);
	}
	_Myiter operator++(int) {	// postincrement
		_Myiter _Tmp(*this);
		_Ptr += step;
		return (_Tmp);
	}
	_Myiter& operator--() {	// predecrement
		_Ptr -= step;
		return (*this);
	}
	_Myiter operator--(int) {	// postdecrement
		_Myiter _Tmp = *this;
		_Ptr -= step;
		return (_Tmp);
	}
	_Myiter& operator+=(difference_type _Off) {	// increment by integer
		_Ptr += (_Off * step);
		return (*this);
	}
	_Myiter operator+(difference_type _Off) const {	// return this + integer
		_Myiter _Tmp(*this);
		return (_Tmp += _Off);
	}

	_Myiter& operator-=(difference_type _Off) {	// decrement by integer
		_Ptr -= (_Off * step);
		return (*this);
	}

	_Myiter operator-(difference_type _Off) const {	// return this - integer
		_Myiter _Tmp(*this);
		return (_Tmp -= _Off);
	}

	difference_type operator-(const _Myiter& _Right) const {
		return (_Ptr - _Right._Ptr) / step;
	}

	reference operator[](difference_type _Off) const {	// subscript
		return (_Ptr + _Off*step);
	}

	template<typename U>
	bool operator==(IN(U) _Right) const {	// test for iterator equality
		return _Ptr == _Right._Ptr;
	}

	template<typename U>
	bool operator!=(IN(U) _Right) const {	// test for iterator inequality
		return !(*this == _Right);
	}

	bool operator<(const _Myiter& _Right) const {	// test if this < _Right
		return _Ptr < _Right._Ptr;
	}

	bool operator>(const _Myiter& _Right) const {	// test if this > _Right
		return _Right < *this;
	}

	bool operator<=(const _Myiter& _Right) const {	// test if this <= _Right
		return !(_Right < *this);
	}

	bool operator>=(const _Myiter& _Right) const {	// test if this >= _Right
		return !(*this < _Right);
	}

	_Tptr _Ptr;
};

template<typename RESULT, typename INPUT, typename FUNCTION>
class generating_iterator {
public:
	typedef std::random_access_iterator_tag iterator_category;
	typedef generating_iterator<RESULT, INPUT, FUNCTION> _Myiter;
	typedef const INPUT* _Tptr;

	typedef RESULT value_type;
	typedef ptrdiff_t difference_type;
	typedef const RESULT pointer;
	typedef const RESULT reference;

	generating_iterator()
		: _Ptr(), function() {
	}
	generating_iterator(IN(FUNCTION) f, _Tptr _Parg)
		: _Ptr(_Parg), function(f) {
	}
	template<typename IT>
	generating_iterator(IN(FUNCTION) f, IN(IT) in)
		: function(f), _Ptr(&(*in)) {
	};
	



	RESULT operator*() const {
		_Analysis_assume_(this->_Ptr != _Tptr());
		return function(*this->_Ptr);
	}
	//pointer operator->() const {	// return pointer to class object
	//	return (::std::pointer_traits<pointer>::pointer_to(**this));
	//}
	_Myiter& operator++() {
		++_Ptr;
		return (*this);
	}
	_Myiter operator++(int) {	// postincrement
		_Myiter _Tmp(*this);
		++_Ptr;
		return (_Tmp);
	}
	_Myiter& operator--() {	// predecrement
		--_Ptr;
		return (*this);
	}
	_Myiter operator--(int) {	// postdecrement
		_Myiter _Tmp = *this;
		--_Ptr;
		return (_Tmp);
	}
	_Myiter& operator+=(ptrdiff_t _Off) {	// increment by integer
		_Ptr += (_Off);
		return (*this);
	}
	_Myiter operator+(ptrdiff_t _Off) const {	// return this + integer
		_Myiter _Tmp(*this);
		return (_Tmp += _Off);
	}

	_Myiter& operator-=(ptrdiff_t _Off) {	// decrement by integer
		_Ptr -= (_Off);
		return (*this);
	}

	_Myiter operator-(ptrdiff_t _Off) const {	// return this - integer
		_Myiter _Tmp(*this);
		return (_Tmp -= _Off);
	}

	ptrdiff_t operator-(const _Myiter& _Right) const {
		return (_Ptr - _Right._Ptr);
	}

	RESULT operator[](ptrdiff_t _Off) const {	// subscript
		return function(*(_Ptr + _Off));
	}

	template<typename T>
	bool operator==(IN(T) _Right) const {	// test for iterator equality
		return _Ptr == _Right._Ptr;
	}

	template<typename T>
	bool operator!=(IN(T) _Right) const {	// test for iterator inequality
		return !(*this == _Right);
	}

	bool operator<(const _Myiter& _Right) const {	// test if this < _Right
		return _Ptr < _Right._Ptr;
	}

	bool operator>(const _Myiter& _Right) const {	// test if this > _Right
		return _Right < *this;
	}

	bool operator<=(const _Myiter& _Right) const {	// test if this <= _Right
		return !(_Right < *this);
	}

	bool operator>=(const _Myiter& _Right) const {	// test if this >= _Right
		return !(*this < _Right);
	}

	FUNCTION function;
	const INPUT* _Ptr;
};

template<typename FUNC, typename BASE>
class transforming_iterator : public BASE {
public:
	transforming_iterator() : BASE() {}
	transforming_iterator(IN(BASE) i) : BASE(i) {}
	auto operator*() const {
		return FUNC::apply(BASE::operator*());
	}
};

class t_deref {
public:
	template<typename T>
	static auto apply(IN(T) i) {
		return *i;
	}
};

template<typename T>
using dereferencing_iterator = transforming_iterator<t_deref, T>;

template<typename FUNC, typename BASE>
class testing_transforming_iterator {
public:
	typedef std::forward_iterator_tag iterator_category;

	using _Myiter = testing_transforming_iterator<FUNC, BASE>;
	using value_type = decltype(FUNC::apply(*(BASE())));
	typedef ptrdiff_t difference_type;

	testing_transforming_iterator() : terminus(), _Ptr() {}

	testing_transforming_iterator(IN(BASE) ptr, IN(BASE) t)
		: terminus(t), _Ptr(ptr) {
		while (_Ptr != terminus && !FUNC::test(*_Ptr))
			++_Ptr;
	};

	auto operator*() {
		return FUNC::apply(*this->_Ptr);
	}

	const BASE terminus;
	BASE _Ptr;

	_Myiter& operator++() {
		++_Ptr;
		while (_Ptr != terminus && !FUNC::test(*_Ptr))
			++_Ptr;
		return (*this);
	}
	_Myiter operator++(int) {	// postincrement
		_Myiter _Tmp(*this);
		++_Ptr;
		while (_Ptr != terminus && !FUNC::test(*_Ptr))
			++_Ptr;
		return (_Tmp);
	}

	_Myiter& operator+=(ptrdiff_t _Off) {	// increment by integer
		_Ptr += (_Off);
		while (_Ptr != terminus && !FUNC::test(*_Ptr))
			++_Ptr;
		return (*this);
	}
	_Myiter operator+(ptrdiff_t _Off) const {	// return this + integer
		_Myiter _Tmp(*this);
		return (_Tmp += _Off);
	}

	ptrdiff_t operator-(const _Myiter& _Right) const {
		return (_Ptr - _Right._Ptr);
	}

	bool operator==(IN(BASE) _Right) const {	// test for iterator equality
		return _Ptr == _Right;
	}

	bool operator!=(IN(BASE) _Right) const {	// test for iterator inequality
		return !(*this == _Right);
	}

	bool operator<(IN(BASE) _Right) const {	// test if this < _Right
		return _Ptr < _Right;
	}

	bool operator>(IN(BASE) _Right) const {	// test if this > _Right
		return _Right < *this;
	}

	bool operator<=(IN(BASE) _Right) const {	// test if this <= _Right
		return !(_Right < *this);
	}

	bool operator>=(IN(BASE) _Right) const {	// test if this >= _Right
		return !(*this < _Right);
	}

};

template<typename SUBRANGE, typename BASE>
class flattening_iterator : public BASE {
public:
	typedef flattening_iterator<SUBRANGE, BASE> _Myiter;

	
	using sub_iterator = typename SUBRANGE::iterator_type;

	sub_iterator current_sub;

	using reference = typename SUBRANGE::reference;
	using pointer = typename SUBRANGE::pointer;

	flattening_iterator()
		: BASE(), current_sub() {
	}
	flattening_iterator(IN(flattening_iterator) b)
		: BASE(static_cast<BASE>(b)), current_sub(b.current_sub) {
	}
	flattening_iterator(IN(BASE) b)
		: BASE(b), current_sub() {
	}
	


	auto operator*() {
		if(current_sub == sub_iterator())
			current_sub = SUBRANGE::begin(BASE::operator*());
		return *current_sub;
	}
	auto get_parent() const {
		return BASE::operator*();
	}
	auto operator->() const {
		return current_sub.operator->();
	}
	_Myiter& operator++() {
		if (current_sub == sub_iterator())
			current_sub = SUBRANGE::begin(BASE::operator*());
		++current_sub;
		if (current_sub == SUBRANGE::end(BASE::operator*())) {
			BASE::operator++();
			current_sub = sub_iterator();
		}
		return (*this);
	}
	_Myiter operator++(int) {	// postincrement
		_Myiter _Tmp(*this);
		++_Tmp;
		return (_Tmp);
	}
	_Myiter& operator--() {	// predecrement
		--current_sub;
		return (*this);
	}
	_Myiter operator--(int) {	// postdecrement
		_Myiter _Tmp(*this);
		--_Tmp;
		return (_Tmp);
	}
	_Myiter& operator+=(typename BASE::difference_type _Off) {	// increment by integer
		if (current_sub == sub_iterator())
			current_sub = SUBRANGE::begin(BASE::operator*());
		current_sub += _Off;
		if (current_sub >= SUBRANGE::end(BASE::operator*())) {
			BASE::operator++();
			current_sub = sub_iterator();
		}
		return (*this);
	}
	_Myiter operator+(typename BASE::difference_type _Off) const {	// return this + integer
		_Myiter _Tmp(*this);
		return (_Tmp += _Off);
	}

	_Myiter& operator-=(typename BASE::difference_type _Off) {	// decrement by integer
		current_sub -= _Off;
		return (*this);
	}

	_Myiter operator-(typename BASE::difference_type _Off) const {	// return this - integer
		_Myiter _Tmp(*this);
		return (_Tmp -= _Off);
	}

	template<typename T>
	bool operator==(IN(T) _Right) const {	// test for iterator equality
		return _Ptr == _Right._Ptr;
	}

	template<typename T>
	bool operator!=(IN(T) _Right) const {	// test for iterator inequality
		return _Ptr != _Right._Ptr;
	}

	bool operator<(IN(_Myiter) _Right) const {	// test if this < _Right
		return BASE::operator<(_Right) || (*this == _Right && current_sub < _Right.current_sub);
	}

	bool operator>(IN(_Myiter) _Right) const {	// test if this > _Right
		return _Right < *this;
	}

	bool operator<=(IN(_Myiter) _Right) const {	// test if this <= _Right
		return !(_Right < *this);
	}

	bool operator>=(IN(_Myiter) _Right) const {	// test if this >= _Right
		return !(*this < _Right);
	}
};

template<int mod, typename T>
modulus_iterator<mod, T> modulus_end(int offset, IN(std::vector<T>) base) noexcept {
	return modulus_iterator<mod, T>(base.begin() + std::max(offset, (((static_cast<int>(base.size()) - 1) - offset) / mod) * mod + mod + offset));
};

template<int mod, typename T>
modulus_iterator<mod, T> modulus_begin(int offset, IN(std::vector<T>) base) noexcept {
	return modulus_iterator<mod, T>(base.begin() + offset);
};


template<typename K, typename V>
class multiindex {
protected:
	flat_multimap<K, V> map;
	//static V tosecond(IN(std::pair<K, V>) p) { return p.second; };
public:
	using rl = g_lock;
	using wl = w_lock;

	multiindex() {};

	size_t count(K k, IN(rl)) const noexcept {
		return map.count(k);
	}

	void insert(const K k, const V v, IN(wl)) noexcept {
		map.emplace(k, v);
	}

	void clear(IN(wl)) noexcept {
		map.clear();
	}

	template <typename ... ARGS>
	void emplace(const K k, IN(wl), ARGS&& ... args) noexcept {
		map.emplace(k, V(std::forward<ARGS>(args) ...));
	}

	template<typename T>
	void for_each(const K k, IN(rl), IN(T) func) const noexcept {
		for (auto pr = map.equal_range(k); pr.first != pr.second; ++pr.first) {
			func(pr.first->second);
		}
	}
	template<typename T>
	void for_all(IN(rl), IN(T) func) const noexcept {
		const auto end = map.end();
		for (auto it = map.begin(); it != end; ++it) {
			func(it->first, it->second);
		}
	}
	template<typename T>
	bool for_each_breakable(const K k, IN(rl), IN(T) func) const noexcept {
		for (auto pr = map.equal_range(k); pr.first != pr.second; ++pr.first) {
			if (func(pr.first->second))
				return true;
		}
		return false;
	}
	template<typename T>
	bool for_all_breakable(IN(rl), IN(T) func) const noexcept {
		const auto end = map.end();
		for (auto it = map.begin(); it != end; ++it) {
			if (func(it->first, it->second))
				return true;
		}
		return false;
	}
	template<typename T>
	bool for_each_breakable(const K k, IN(wl), IN(T) func) noexcept {
		for (auto pr = map.equal_range(k); pr.first != pr.second; ++pr.first) {
			if (func(pr.first->second))
				return true;
		}
		return false;
	}
	template<typename T>
	bool for_all_breakable(IN(wl), IN(T) func) noexcept {
		const auto end = map.end();
		for (auto it = map.begin(); it != end; ++it) {
			if (func(it->first, it->second))
				return true;
		}
		return false;
	}
	template <typename F>
	void range_erase_if(const K k, IN(wl), IN(F) func) noexcept {
		const auto iend = map.upper_bound(k);
		map.erase(
			std::remove_if(map.lower_bound(k), iend, std::cref(func)),
			iend);
	}

	void erase(const K k, const V v, IN(wl) l) noexcept {
		range_erase_if(k, l, [&v](IN(std::pair<K, V>) p) {return p.second == v; });
	}
	void eraseall(const K k, IN(wl)) noexcept {
		map.erase(k);
	}
	template <typename F>
	void erase_if(IN(wl), IN(F) func) noexcept {
		boost::range::remove_erase_if(map, std::cref(func));
	}

	template <typename VEC>
	void to_vector(K index, INOUT(VEC) vec, IN(rl)) const noexcept {
		auto pr = map.equal_range(index);
		vec.reserve(vec.size() + std::distance(pr.first, pr.second));
		for (; pr.first != pr.second; ++pr.first) {
			vec.push_back(pr.first->second);
		}
	}

	template <typename VEC>
	void replace_range(K index, IN(VEC) vec, IN(wl)) noexcept {
		auto pr = map.equal_range(index);
		size_t vindx = 0;
		const auto vsize = vec.size();
		for (; pr.first != pr.second && vindx != vsize; ++pr.first) {
			pr.first->second = vec[vindx];
			++vindx;
		}
		if (pr.first != pr.second) {
			map.erase(pr.first, pr.second);
		} else if (vindx != vsize) {
			std::vector<std::pair<K, V>> newvec;
			newvec.reserve(vsize - vindx);
			for (; vindx != vsize; ++vindx) {
				newvec.emplace_back(index, vec[vindx]);
			}
			map.insert_sk(pr.second, newvec.begin(), newvec.end());
		}
	}

	void update(const K k, const V v1, const V v2, IN(wl)) noexcept {
		for (auto pr = map.equal_range(k); pr.first != pr.second; ++pr.first) {
			if (pr.first->second == v1) {
				pr.first->second = v2;
				return;
			}
		}
		map.emplace(k, v2);
	}

	auto range(K index, IN(wl)) noexcept {
		return map.equal_range(index);
	}
	auto range(K index, IN(rl)) const noexcept {
		return map.equal_range(index);
	}
};

template<typename K, typename V>
class single_index_t {
protected:
	flat_map<K, V> map;
public:
	using rl = g_lock;
	using wl = w_lock;

	single_index_t() {};

	auto insert(K k, V v, IN(wl)) noexcept {
		return map.insert(std::pair<K, V>(k, v));
	}
	void clear(IN(wl)) noexcept {
		map.clear();
	}
	auto find(K k, IN(rl)) const noexcept {
		return map.find(k);
	}
	auto find(K k, IN(wl)) noexcept {
		return map.find(k);
	}
	auto end(IN(rl)) const noexcept {
		return map.end();
	}
	V& get(K k, IN(wl)) noexcept {
		return map[k];
	}
	bool contains(K k, IN(rl)) const noexcept {
		return map.count(k) != 0;
	}
	const V& get(K k, IN(rl)) const noexcept {
		return map.at(k);
	}
	template <typename ... ARGS>
	auto emplace(const K k, IN(wl), ARGS&& ... args) noexcept {
		return map.emplace(k, V(std::forward<ARGS>(args) ...));
	}
	template<typename T>
	void for_all(IN(rl), IN(T) func) const noexcept {
		const auto end = map.end();
		for (auto it = map.begin(); it != end; ++it) {
			func(it->first, it->second);
		}
	}
	template<typename T>
	void for_all(IN(wl), IN(T) func) noexcept {
		const auto end = map.end();
		for (auto it = map.begin(); it != end; ++it) {
			func(it->first, it->second);
		}
	}
	template<typename T>
	bool for_all_breakable(IN(rl), IN(T) func) const noexcept {
		const auto end = map.end();
		for (auto it = map.begin(); it != end; ++it) {
			if (func(it->first, it->second))
				return true;
		}
		return false;
	}
	void erase(K k, IN(wl)) noexcept {
		map.erase(k);
	}
	template<typename T>
	void erase_if(IN(wl), IN(T) func) noexcept {
		boost::range::remove_erase_if(map, std::cref(func));
	}
	void update(K k, V v, IN(wl)) noexcept {
		map[k] = v;
	}
};

template<typename T>
using single_index = single_index_t<size_t, T>;

class interested_vector {
private:
	concurrent_vector<char_id> vec;
public:
	void add(const char_id c) {
		for (auto& __restrict id : vec) {
			if (InterlockedCompareExchange(&id,c,0) == 0)
				return;
		}
		vec.push_back(c);
	}
	void exchange(const char_id oldval, const char_id newal) {
		for (auto& __restrict id : vec) {
			InterlockedCompareExchange(&id, newal, oldval);
		}
	}
	void remove(const char_id c) {
		for (auto& __restrict id : vec) {
			InterlockedCompareExchange(&id, 0, c);
		}
	}
	bool in_set(const char_id c) const {
		for (const auto id : vec) {
			if (id == c)
				return true;
		}
		return false;
	}
};

template<typename OBJ, typename TAG>
class v_pool_t {
public:
	std::vector<OBJ> pool;

	using tag_type = TAG;
	static constexpr auto tag_max = max_value<TAG>::value;

	TAG first_free = tag_max;
	TAG last_free = tag_max;

	using rl = g_lock;
	using wl = w_lock;

	void clear(IN(wl) l) noexcept {
		pool.clear();
		first_free = tag_max;
		last_free = tag_max;
	}
	
	void expand_capacity(size_t newsize, IN(wl) l) noexcept {
		auto oldsize = pool.size();
		if (newsize > oldsize) {
			pool.resize(newsize);
			for (; oldsize != newsize; ++oldsize) {
				free(static_cast<TAG>(oldsize), l);
			}
		}
	}
	
	void reserve(TAG t, IN(wl) l) noexcept {
		if (t == first_free) {
			first_free = last_free;
			for (TAG i = t + 1; i < last_free; ++i) {
				if (pool[i].is_clear()) {
					first_free = i;
					break;
				}
			}
		}

		if (t == last_free) {
			if (last_free == first_free) {
				first_free = last_free = tag_max;
				return;
			}
			last_free = first_free;
			for (TAG i = last_free - 1; i > first_free; --i) {
				if (pool[i].is_clear()) {
					last_free = i;
					break;
				}
			}
		}
	}

	template<typename OIN>
	TAG add(OIN&& o, IN(wl) l) noexcept {
		if (first_free == tag_max) {
			pool.push_back(std::forward<OIN>(o));
			return static_cast<TAG>(pool.size() - 1);
		} else {
			const auto location = first_free;
			pool[location] = std::forward<OIN>(o);
			if (first_free == last_free) {
				first_free = last_free = tag_max;
				return location;
			} else {
				for (TAG i = first_free + 1; i < last_free; ++i) {
					if (pool[i].is_clear()) {
						first_free = i;
						return location;
					}
				}
				first_free = last_free;
				return location;
			}
		}
	}
	template <typename ... ARGS>
	TAG emplace(IN(wl) l, ARGS&& ... args) noexcept {
		if (first_free == tag_max) {
			pool.emplace_back(std::forward<ARGS>(args) ...);
			return static_cast<TAG>(pool.size() - 1);
		} else {
			const auto location = first_free;
			pool[location].construct(std::forward<ARGS>(args) ...);
			if (first_free == last_free) {
				first_free = last_free = tag_max;
				return location;
			} else {
				for (TAG i = first_free + 1; i < last_free; ++i) {
					if (pool[i].is_clear()) {
						first_free = i;
						return location;
					}
				}
				first_free = last_free;
				return location;
			}
		} 
	}
	void free(TAG indx, IN(wl) l) noexcept {
		pool[indx].set_clear();
		if (indx < first_free)
			first_free = indx;
		if (indx > last_free || last_free == tag_max)
			last_free = indx;
	}
	void free(INOUT(OBJ) p, IN(wl) l) noexcept {
		const TAG indx = static_cast<TAG>(std::distance(&(*pool.begin()), &p));
		p.set_clear();
		if (indx < first_free)
			first_free = indx;
		if (indx > last_free || last_free == tag_max)
			last_free = indx;
	}
	OBJ& get(TAG indx, IN(rl) l) noexcept {
		return pool[indx];
	}
	const OBJ& get(TAG indx, IN(rl) l) const noexcept {
		return pool[indx];
	}
	TAG get_index(IN(OBJ) p, IN(rl) l) const noexcept {
		return static_cast<TAG>(std::distance(&(*pool.begin()), &p));
	}
	template <typename FUNC>
	void for_each(IN(rl) l, IN(FUNC) f) noexcept {
		size_t sz = pool.size();
		for (size_t i = 0; i != sz; ++i) {
			if (!pool[i].is_clear()) {
				f(pool[i]);
			}
		}
	}

	template <typename FUNC>
	void for_each_breakable(IN(rl) l, IN(FUNC) f) noexcept {
		size_t sz = pool.size();
		for (size_t i = 0; i != sz; ++i) {
			if (!pool[i].is_clear()) {
				if (f(pool[i]))
					break;
			}
		}
	}
};

template<typename OBJ>
using v_pool = v_pool_t<OBJ, unsigned int>;

template<typename T, typename I>
class v_vector_t {
public:
	std::vector<T> elements;
	std::vector<I> index;

	v_vector_t() {
		index.push_back(0);
	}

	void clear() noexcept {
		index.clear();
		elements.clear();
		index.push_back(0);
	}

	void insert(IN(typename std::vector<T>::iterator) it, T elem) noexcept {
		size_t pos = std::distance(elements.begin(), it);
		elements.insert(it, elem);
		size_t i = 0;
		const auto isize = index.size();
		for (; index[i] < pos && i != isize; ++i)
			;
		if (i != isize && index[i] == pos)
			++i;
		for (; i != isize; ++i)
			++index[i];
	}

	void new_row() noexcept {
		index.push_back(static_cast<I>(elements.size()));
	}

	void expand_rows(size_t sz) noexcept {
		if(index.size() < sz+1)
			index.resize(sz+1, static_cast<I>(elements.size()));
	}

	size_t row_size() const noexcept {
		return index.size();
	}

	std::pair<typename std::vector<T>::iterator, typename std::vector<T>::iterator> get_row(size_t i) noexcept {
		std::pair<typename std::vector<T>::iterator, typename std::vector<T>::iterator> p;
		p.first = elements.begin() + index[i];
		if (i + 1 < index.size()) {
			p.second = elements.begin() + index[i + 1];
		} else {
			p.second = elements.end();
		}
		return p;
	}

	std::pair<typename std::vector<T>::const_iterator, typename std::vector<T>::const_iterator> get_row(size_t i) const noexcept {
		std::pair<typename std::vector<T>::const_iterator, typename std::vector<T>::const_iterator> p;
		p.first = elements.cbegin() + index[i];
		if (i + 1 < index.size()) {
			p.second = elements.cbegin() + index[i + 1];
		} else {
			p.second = elements.cend();
		}
		return p;
	}

	T& get(size_t x, size_t y) noexcept {
		auto it = elements.begin() + index[x] + y;
		return *it;
	}

	const T& get(size_t x, size_t y) const noexcept {
		const auto it = elements.cbegin() + index[x] + y;
		return *it;
	}

	void push_back(IN(T) elem) noexcept {
		elements.push_back(elem);
	}

	void add_to_row(size_t i, IN(T) elem) noexcept {
		if (i >= index.size()) {
			index.resize(i + 1, static_cast<I>(elements.size()));
		}
		elements.insert(elements.begin() + index[i], elem);
		size_t sz = index.size();
		for (++i; i < sz; ++i) {
			++index[i];
		}
	}

	void append_to_row(size_t i, IN(T) elem) noexcept {
		size_t sz = index.size();
		if (i + 1 < sz)
			elements.insert(elements.begin() + index[i + 1], elem);
		else
			elements.push_back(elem);
		for (++i; i < sz; ++i) {
			++index[i];
		}
	}
};

template <typename T>
using v_vector = v_vector_t<T, unsigned int>;

template<typename T>
class ordered_pair : public std::pair<T, T> {
public:
	ordered_pair(const T a, const T b) : pair<T, T>(std::min(a, b), std::max(a, b)) {};
	T get_other(const T i) {
		if (first == i)
			return second;
		return first;
	}
};

using edge = ordered_pair<prov_id>;

template<typename T>
struct std::hash<ordered_pair<T>> {
	std::size_t operator()(IN(ordered_pair<T>) s) const
	{
		return std::hash<T>()(s.first) ^ std::hash<T>()(s.second);
	}
};

template<typename T>
struct std::hash<std::pair<T,T>> {
	std::size_t operator()(IN(std::pair<T,T>) s) const {
		return std::hash<T>()(s.first) ^ std::hash<T>()(s.second);
	}
};


template <int v>
struct ItoType {
	enum {value = v };
};

class NullType {
public:
	NullType() {};
	template<typename T> explicit NullType(T&&) {};
};


class peronal_attributes {
private:
	unsigned __int64 data;
public:
	void load(unsigned __int64 d) noexcept {
		data = d;
		unsigned __int64 mask = 0x03;
		for (size_t i = 0; i != 32; ++i) {
			if (((data & mask) == mask))
				data &= ~mask;
			mask <<= 2;
		}
	}
	unsigned __int64 to_int() const noexcept {
		return data;
	}
	bool has_positive_N(unsigned int n) const noexcept {
		return (data & ((unsigned __int64)0x01) << (n * 2)) != 0;
	}
	bool has_negative_N(unsigned int n) const noexcept {
		return (data & ((unsigned __int64)0x02) << (n * 2)) != 0;
	}
	template<int n>
	bool has_positive_N() const noexcept {
		return (data & ((unsigned __int64)0x01) << (n * 2)) != 0;
	}
	template<int n>
	bool has_negative_N() const noexcept {
		return (data & ((unsigned __int64)0x02) << (n * 2)) != 0;
	}
	unsigned char trait_value(unsigned int n) const noexcept {
		return (data >> (n * 2)) & 0x03;
	}
	template<int n>
	unsigned char trait_value() const noexcept {
		return (data >> (n * 2)) & 0x03;
	}
	template<int max>
	int compare(IN(peronal_attributes) other) const noexcept {
		unsigned __int64 ord = data ^ other.data;
		unsigned __int64 cpy = data;

		int sum = 0;
		for (size_t n = 0; n != max; ++n) {
			if ((ord & 0x03) == 0x03) {
				--sum;
			} else if ((ord & 0x03) == 0 && (cpy & 0x03) != 0) {
				++sum;
			}
			cpy >>= 2;
			ord >>= 2;
		}
		return sum;
	}
	void set_positive_N(unsigned int n) {
		const unsigned __int64 mask = ~(((unsigned __int64)0x03) << n * 2);
		const unsigned __int64 value = ((unsigned __int64)0x01) << n * 2;
		data &= mask;
		data |= value;
	}
	void set_negative_N(unsigned int n) {
		const unsigned __int64 mask = ~(((unsigned __int64)0x03) << n * 2);
		const unsigned __int64 value = ((unsigned __int64)0x02) << n * 2;
		data &= mask;
		data |= value;
	}
	void clear_N(unsigned int n) {
		const unsigned __int64 mask = ~(((unsigned __int64)0x03) << n*2);
		data &= mask;
	}
};

template <int n, int factor_numerator, int factor_denomenator>
void normalize_array(double array[n]) {
	const static double factor = static_cast<double>(factor_numerator) / static_cast<double>(factor_denomenator);
	double denom = 0.0;
	for (size_t i = 0; i != n; ++i) {
		denom += array[i] * array[i];
	}
	denom = factor / denom;
	for (size_t i = 0; i != n; ++i) {
		array[i] *= denom;
	}
}

template<int n>
class sum_of_arrays {
public:
	double* arrays[n];

	template<typename... ARGS>
	sum_of_arrays(ARGS ... args) noexcept {
		sum_of_arrays_innr<0>(args ...);
	}
	template<int cnt>
	void sum_of_arrays_innr(double* param) noexcept {
		arrays[cnt] = param;
	}
	template<int cnt, typename... ARGS>
	void sum_of_arrays_innr(double* param, ARGS ... args) noexcept {
		arrays[cnt] = param;
		sum_of_arrays_innr<cnt + 1>(args ...);
	}

	double operator[] (size_t j) const noexcept {
		double sum = 0.0;
		for (size_t i = 0; i != n; ++i) {
			sum += arrays[i][j];
		}
		return sum;
	}

	double sum() const noexcept {
		double sum = 0.0;
		for (size_t i = 0; i != n; ++i) {
			for (size_t j = 0; j != n; ++ j)
				sum += arrays[i][j];
		}
		return sum;
	}
};

template <typename T, typename Q>
inline void _vector_erase(INOUT(std::vector<T, Q>) v, const T val, std::true_type) noexcept {
	decltype(v.begin()) it;
	const auto end = v.end();
	if ((it = std::find(v.begin(), end, val)) != end) {
		*it = std::move(v.back());
		v.pop_back();
	}
}

template <typename T, typename Q>
inline void _vector_erase(INOUT(std::vector<T, Q>) v, IN(T) val, std::false_type) noexcept {
	decltype(v.begin()) it;
	const auto end = v.end();
	if ((it = std::find(v.begin(), end, val)) != end) {
		*it = std::move(v.back());
		v.pop_back();
	}
}


template <typename T, typename Q>
inline void vector_erase_all(INOUT(T) v, const Q val) noexcept {
	for (size_t i = v.size() - 1; i != SIZE_MAX; --i) {
		if (v[i] == val) {
			v[i] = std::move(v.back());
			v.pop_back();
		}
	}
}

#define vector_erase(vector, value) _vector_erase(vector, value, std::is_scalar<decltype(value)>::type())


template <typename T, typename F>
inline void vector_erase_if(INOUT(T) vec, IN(F) func) noexcept {
	for (size_t indx = vec.size() - 1; indx != SIZE_MAX; --indx) {
		if (!func(vec[indx])) {
			
		} else {
			vec[indx] = std::move(vec.back());
			vec.pop_back();
		}
	}
}

using namespace boost::gregorian;
extern const date date_offset;

template<size_t alignment>
__declspec(restrict) void* concurrent_aligned_malloc(size_t size) {
	static constexpr auto align_mask = alignment - 1;

	unsigned char* __restrict ptr = static_cast<unsigned char*>(concurrency::Alloc(size + alignment));
	if (!ptr)
		return nullptr;

	unsigned char* __restrict aligned_ptr = ptr + (alignment - ((intptr_t)(ptr + alignment) & align_mask));
	*(aligned_ptr - 1) = static_cast<unsigned char>(aligned_ptr - ptr);

	return aligned_ptr;
}

void concurrent_aligned_free(IN_P(void) ptr);

template <typename T, typename... ARGS>
__declspec(restrict) T* concurrent_allocate(ARGS&&... args) noexcept {
	T* const  __restrict p = (T*)(concurrency::Alloc(sizeof(T)));
	::new ((void*)p) T(std::forward<ARGS>(args)...);
	return p;
}

template <typename T>
class concurrent_delete {
public:
	void operator() (IN_P(T) ptr) noexcept {
		ptr->~T();
		concurrency::Free(ptr);
	}
};

template <typename T>
using concurrent_uniq = std::unique_ptr<T, concurrent_delete<T>>;

template <typename T, typename... ARGS>
concurrent_uniq<T> concurrent_unique(ARGS&&... args) noexcept {
	T* const  __restrict p = (T*)(concurrency::Alloc(sizeof(T)));
	::new ((void*)p) T(std::forward<ARGS>(args)...);
	return concurrent_uniq<T>(p, concurrent_delete<T>());
}

template <typename T, typename BASE, typename... ARGS>
concurrent_uniq<BASE> concurrent_unique_cast(ARGS&&... args) noexcept {
	T* const  __restrict p = (T*)(concurrency::Alloc(sizeof(T)));
	::new ((void*)p) T(std::forward<ARGS>(args)...);
	return concurrent_uniq<BASE>(p, concurrent_delete<BASE>());
}

template <typename T>
class concurrent_allocator {
public:
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef const T* const_pointer;
	typedef const T& const_reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef std::false_type propagate_on_container_move_assignment;
	typedef std::true_type is_always_equal;

	template<typename U>
	struct rebind
	{	
		typedef concurrent_allocator<U> other;
	};

	pointer address(reference val)const  noexcept {
		return &val;
	}
	const_pointer address(const_reference val) const noexcept {
		return &val;
	}

	concurrent_allocator() noexcept {}
	concurrent_allocator(const concurrent_allocator<T>&) noexcept {}
	template<typename U>
	concurrent_allocator(const concurrent_allocator<U>&) noexcept {
	}
	template<typename U>
	concurrent_allocator<T>& operator=(const concurrent_allocator<U>&) noexcept {
		return *this;
	}

	void deallocate(pointer ptr, size_type count) noexcept  {
		concurrency::Free(ptr);
	}

	__declspec(restrict) pointer allocate(size_type count) noexcept {
		return (pointer)concurrency::Alloc(sizeof(T)*count);
	}

	__declspec(restrict) pointer allocate(size_type count, const void *) noexcept {
		return (pointer)concurrency::Alloc(sizeof(T)*count);
	}

	template<typename U, typename... ARGS>
	void construct(U *ptr, ARGS&&... args) {
		::new ((void *)ptr) U(std::forward<ARGS>(args)...);
	}

	template<typename U>
	void destroy(U *ptr) {
		ptr->~U();
	}
	size_t max_size() const noexcept {	// estimate maximum array size
		return ((size_t)(-1) / sizeof(T));
	}

	bool operator==(IN(concurrent_allocator<T>) o) const {
		return true;
	}
	bool operator!=(IN(concurrent_allocator<T>) o) const {
		return false;
	}
};

template<typename T>
using cvector = std::vector<T, concurrent_allocator<T>>;
template<typename T>
using cflat_set = flat_set<T, std::less<T>, concurrent_allocator<T>>;
template<typename T, typename V>
using cflat_map = flat_map<T, V, std::less<T>, concurrent_allocator<std::pair<T, V>>>;

template <int n>
struct mask {
	const static __int64 value = (1I64 << (n-1)) | mask<n - 1>::value;
};

template<typename L>
struct allocator_t {
	template<typename K>
	using type = concurrent_allocator<K>;
	template<typename K>
	using boost_type = concurrent_allocator<K>;
};

template<>
struct allocator_t<w_lock> {
	template<typename K>
	using type = std::allocator<K>;
	template<typename K>
	using boost_type = boost::container::new_allocator<K>;
};


template<>
struct mask<0> {
	const static __int64 value = 0;
};

template<typename O, typename I>
O dumb_cast(IN(I) v) {
	O r;
	memcpy((void*)(&r), (void*)(&v), std::min(sizeof(I),sizeof(O)));
	return r;
}

#define MAX_THREADS 32

class tls_handle_wrapper {
protected:
	DWORD dwTlsIndex;
public:
	tls_handle_wrapper() : dwTlsIndex(TlsAlloc()) {
	}
	tls_handle_wrapper(const tls_handle_wrapper&) = delete;
	tls_handle_wrapper(tls_handle_wrapper&& o) : dwTlsIndex(o.dwTlsIndex) { o.dwTlsIndex = TLS_OUT_OF_INDEXES; };
	tls_handle_wrapper& operator=(const tls_handle_wrapper&) = delete;
	tls_handle_wrapper& operator=(tls_handle_wrapper&& o) { dwTlsIndex = o.dwTlsIndex; o.dwTlsIndex = TLS_OUT_OF_INDEXES; };

	~tls_handle_wrapper() {
		if(dwTlsIndex != TLS_OUT_OF_INDEXES)
			TlsFree(dwTlsIndex);
	}
	operator DWORD() const noexcept {
		return dwTlsIndex;
	}
};

class random_store {
public:
	using generator_type = boost::random::mt11213b;

	static tls_handle_wrapper dwTlsIndex;

	static generator_type& get_generator() noexcept;

	static unsigned __int64 get_value() noexcept {
		INOUT(auto) rg = get_generator();
		return (static_cast<unsigned __int64>((rg)()) << 32) ^ static_cast<unsigned __int64>((rg)());
	}

	static unsigned __int64 get_int() noexcept {
		return get_value();
	}

	static auto get_fast_int() noexcept {
		return (get_generator())();
	}

	static double get_double() noexcept {
		return std::generate_canonical<double, std::numeric_limits<double>::digits>(get_generator());
	}
	static double get_fast_double() noexcept {
		return std::generate_canonical<double, std::numeric_limits<float>::digits>(get_generator());
	}
	static float get_float() noexcept {
		return std::generate_canonical<float, std::numeric_limits<float>::digits>(get_generator());
	}
};

extern random_store global_store;

inline random_store& get_random_store() noexcept {
	return global_store;
}

template <int n, int j, int max>
void get_from_distribution_vec(unsigned int results[n], IN(std::uniform_real_distribution<double>) d, IN(sum_of_arrays<j>) distribution, INOUT(random_store::generator_type) g) {
	double random_in[n];
	for (size_t i = 0; i != n; ++i) {
		random_in[i] = d(g);
	}

	std::sort(std::begin(random_in), std::end(random_in));
	size_t position = 0;
	double current = 0.0;
	for (size_t i = 0; i != max && position != n; ++i) {
		current += distribution[i];
		while (random_in[position] <= current && position != n) {
			results[position] = static_cast<unsigned int>(i);
			position++;
		}
	}

	while (position != n) {
		results[position] = max - 1;
		position++;
	}
}

size_t get_from_distribution_nt(const double* ar, const size_t n, double total);
size_t get_from_distribution_n(const double* ar, const size_t n);

template<int n>
size_t get_from_distribution_t(const double* ar, double total) {
	total *= global_store.get_double();
	for (size_t j = 0; j < n; ++j) {
		if (ar[j] > 0.0) {
			if (total <= ar[j])
				return j;
			total -= ar[j];
		}
	}
	return SIZE_MAX;
}

template<int n>
size_t get_from_distribution(const double* ar) {
	double total = std::max(ar[0], 0.0);
	for (size_t i = 1; i < n; ++i) {
		total += std::max(ar[i], 0.0);
	}
	return get_from_distribution_t<n>(ar, total);
}

template <typename T, typename U>
bool weak_equals(IN(T) a, IN(U) b) {
	return !a.owner_before(b) && !b.owner_before(a);
}

template <typename MAP, typename K, typename V>
void emplace_multi_unique(INOUT(MAP) m, K k, V v) {
	for (auto pr = m.equal_range(k); pr.first != pr.second; ++pr.first) {
		if (pr.first->second == v)
			return;
	}
	m.emplace(k, v);
}

template <typename MAP, typename K, typename V>
void emplace_vv_unique(INOUT(MAP) m, K k, V v) {
	if (k >= m.index.size()) {
		m.add_to_row(k, v);
		return;
	}
	for (auto pr = m.get_row(k); pr.first != pr.second; ++pr.first) {
		if (*(pr.first) == v)
			return;
	}
	m.add_to_row(k, v);
}


template<typename T, T* buffer>
struct string_ref_t : public comparison_from_lt<string_ref_t<T, buffer>> {
protected:
	unsigned int offset;
	unsigned int length;
public:
	static const std::true_type no_add;

	explicit string_ref_t(IN(T) txt) noexcept {
		length = static_cast<unsigned int>(txt.size());
		size_t off = buffer->find(txt);
		if (off == T::npos) {
			offset = static_cast<unsigned int>(buffer->size());
			*buffer += txt;
		} else {
			offset = static_cast<unsigned int>(off);
		}
	}
	explicit string_ref_t(IN(T) txt, decltype(no_add)) noexcept {
		size_t off = buffer->find(txt);
		if (off != T::npos) {
			length = static_cast<unsigned int>(txt.size());
			offset = static_cast<unsigned int>(off);
		} else {
			length = 0;
			offset = 0;
		}
	}
	constexpr string_ref_t() noexcept : offset(0) , length(0) {}
	explicit constexpr string_ref_t(__int64 data) : offset(static_cast<unsigned int>(data & mask<32>::value)), length(static_cast<unsigned int>(data >> 32)) {}

	T get() const noexcept {
		return buffer->substr(offset, length);
	}
	constexpr __int64 to_int() const noexcept {
		return ((__int64)length << 32) | (__int64)offset;
	}
	void load(__int64 data) noexcept {
		offset = static_cast<unsigned int>(data & mask<32>::value);
		length = static_cast<unsigned int>(data >> 32);
	}
	static __int64 find_value(IN(T) txt) noexcept {
		size_t off = buffer->find(txt);
		if (off == T::npos) {
			return 0;
		} else {
			unsigned int o;
			unsigned int l;

			l = static_cast<unsigned int>(txt.size());
			o = static_cast<unsigned int>(off);

			return ((__int64)l << 32) | (__int64)o;
		}
	}
	bool operator< (IN(string_ref_t<T, buffer>) other) const noexcept {
		return to_int() < other.to_int();
	}
	bool operator==(IN(string_ref_t<T, buffer>) other) const noexcept {
		return to_int() == other.to_int();
	}
};

template<typename T, T* buffer>
const std::true_type string_ref_t<T, buffer>::no_add = std::true_type();


class update_record {
public:
	bool needs_update;
	const std::function<void()> func;
	update_record(IN(std::function<void()>) f) : needs_update(false), func(f) {};
};

template<typename T>
inline size_t to_param(T i) {
	return static_cast<size_t>(i.value);
}
template<>
inline size_t to_param(double i) {
	return dumb_cast<size_t>(i);
}
template<>
inline size_t to_param(float i) {
	double d = static_cast<double>(i);
	return dumb_cast<size_t>(d);
}
template<>
inline size_t to_param(size_t i) {
	return i;
}
template<>
inline size_t to_param(int i) {
	return static_cast<size_t>(i);
}
template<>
inline size_t to_param(short i) {
	return static_cast<size_t>(i);
}
template<>
inline size_t to_param(unsigned short i) {
	return static_cast<size_t>(i);
}
template<>
inline size_t to_param(unsigned char i) {
	return static_cast<size_t>(i);
}
template<>
inline size_t to_param(unsigned int i) {
	return static_cast<size_t>(i);
}

template<int size, typename T>
class nolock_stack {
public:
	T storage[size];
	LONG freeposition;

	void push(T value) {
		LONG newpos = InterlockedIncrement(&freeposition);
		storage[(newpos - 1) % size] = value;
	}
	T peek() {
		LONG tfree = freeposition;
		if (tfree != 0) {
			return storage[(tfree - 1) % size];
		}
		return T();
	}
	T pop() {
		LONG tfree = freeposition;
		if (tfree != 0) {
			T oldval = storage[(tfree-1) % size];
			LONG oldpos = InterlockedDecrement(&freeposition);
			TInterlockedCompareExchange(&storage[(tfree - 1) % size], T(), oldval);
			return oldval;
		}
		return T();
	}
};

template <typename gen_class, typename sz, int n, typename ... Types>
class generate_array_it {
public:
	static std::array<decltype(gen_class::nth_element<0>()), sz::value> doit(Types&&... t) {
		return generate_array_it<gen_class, sz, n - 1, decltype(gen_class::nth_element<0>()), Types ...>::doit(gen_class::nth_element<n>(), std::forward<Types>(t)...);
	}
};

template <typename gen_class, typename sz, typename ... Types>
class generate_array_it<gen_class, sz, -1, Types ...> {
public:
	static std::array<decltype(gen_class::nth_element<0>()), sz::value> doit(Types&&... t) {
		return{std::forward<Types>(t)...};
	}
};


template <typename gen_class, int sz >
std::array<decltype(gen_class::nth_element<0>()), sz> generate_array() {
	return generate_array_it<gen_class, std::integral_constant<int, sz>, sz - 1>::doit();
}

template<typename MEMBER_TYPE, typename EXTRA_DATA, typename EXEC>
class actionable_list_detail {
public:
	template<typename T>
	struct __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) generic_node {
		SLIST_ENTRY slist_entry;
		EXTRA_DATA dat;
		T obj;

		void execute() noexcept {
			EXEC::apply(dat, obj);
		}
	};

	template<typename derived, typename ... P>
	static __declspec(restrict) generic_node<derived>* get_new(IN(EXTRA_DATA) dat, P&& ... params) noexcept {
		generic_node<derived>* __restrict pNode = (generic_node<derived>*)concurrent_aligned_malloc<MEMORY_ALLOCATION_ALIGNMENT>(sizeof(generic_node<derived>));
		new (static_cast<void*>(&pNode->dat)) EXTRA_DATA(dat);
		new (static_cast<void*>(&pNode->obj)) derived(std::forward<P>(params)...);
		return pNode;
	}
};

template<typename MEMBER_TYPE, typename EXEC>
class actionable_list_detail<MEMBER_TYPE, std::false_type, EXEC> {
public:
	template<typename T>
	struct __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) generic_node {
		SLIST_ENTRY slist_entry;
		T obj;

		void execute() noexcept {
			EXEC::apply(obj);
		}
	};

	template<typename derived, typename ... P>
	static __declspec(restrict) generic_node<derived>* get_new(P&& ... params) noexcept {
		generic_node<derived>* __restrict pNode = (generic_node<derived>*)concurrent_aligned_malloc<MEMORY_ALLOCATION_ALIGNMENT>(sizeof(generic_node<derived>));
		new (static_cast<void*>(&pNode->obj)) derived(std::forward<P>(params)...);
		return pNode;
	}
};



template<typename MEMBER_TYPE, typename EXTRA_DATA, typename EXEC>
class actionable_list_class_t {
private:
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) SLIST_HEADER stack_head;

	template<typename T>
	using generic_node = typename actionable_list_detail<MEMBER_TYPE, EXTRA_DATA, EXEC>::generic_node<T>;

public:
	actionable_list_class_t() {
		InitializeSListHead(&stack_head);
	}
	~actionable_list_class_t() {
		clear();
	}
	template<typename derived, typename ... P>
	void add_new(P&& ... params) noexcept {
		generic_node<derived>* __restrict pNode = actionable_list_detail<MEMBER_TYPE, EXTRA_DATA, EXEC>::get_new<derived>(std::forward<P>(params)...);
		InterlockedPushEntrySList(&stack_head, &pNode->slist_entry);
	}
	void clear() noexcept {
		generic_node<MEMBER_TYPE>* __restrict pNode = (generic_node<MEMBER_TYPE>*)InterlockedPopEntrySList(&stack_head);
		while (pNode) {
			((MEMBER_TYPE*)(&pNode->obj))->~MEMBER_TYPE();
			concurrent_aligned_free(pNode);
			pNode = (generic_node<MEMBER_TYPE>*)InterlockedPopEntrySList(&stack_head);
		}
	}
	void execute_entries() noexcept {
		generic_node<MEMBER_TYPE>* __restrict pNode = (generic_node<MEMBER_TYPE>*)InterlockedPopEntrySList(&stack_head);
		while (pNode) {
			pNode->execute();
			((MEMBER_TYPE*)(&pNode->obj))->~MEMBER_TYPE();
			concurrent_aligned_free(pNode);
			pNode = (generic_node<MEMBER_TYPE>*)InterlockedPopEntrySList(&stack_head);
		}
	}
};

template<typename T, size_t sz>
void shuffle(IN_P(T) dest) noexcept {
	class _shuffle {
	public:
		template<size_t n>
		static void apply(IN_P(T) dest) noexcept {
			std::swap(dest[0], dest[random_store::get_fast_int() % sz]);
			apply<n - 1>(dest + 1);
		}
		template<>
		static void apply<1>(IN_P(T) dest) noexcept {

		}
	};
	_shuffle::apply(dest);
}

template<typename T, size_t sz>
void shuffle(const IN_P(T) source, IN_P(T) dest) noexcept {
	memcpy(dest, source, sz * sizeof(T));
	shuffle<sz>(dest);
}

template<typename T>
void shuffle(T* dest, size_t sz) noexcept {
	while (sz > 1) {
		std::swap(dest[0], dest[random_store::get_fast_int() % sz]);
		++dest;
		--sz;
	}
}

template<typename T>
void shuffle(const IN_P(T) source, T* dest, size_t sz) noexcept {
	memcpy(dest, source, sz * sizeof(T));
	shuffle(dest, sz);
}

template<typename MEMBER_TYPE, typename EXEC>
using actionable_list_class = actionable_list_class_t<MEMBER_TYPE, std::false_type, EXEC>;

#ifndef UNICODE  
typedef std::string tstring;
#else
typedef std::wstring tstring;
#endif

tstring window_text(HWND hwnd);

std::string wstr_to_str(IN(std::wstring) in);
std::wstring str_to_wstr(IN(std::string) in);

#ifndef UNICODE  
#define tstr_to_str(x) x
#define str_to_tstr(x) x
#define tstr_to_wstr str_to_wstr
#define wstr_to_tstr wstr_to_str
#else
#define tstr_to_str wstr_to_str
#define str_to_tstr str_to_wstr
#define tstr_to_wstr(x) x
#define wstr_to_tstr(x) x
#endif

#define UPDATE_FREQ 5
#define MAX_PROVS 1438