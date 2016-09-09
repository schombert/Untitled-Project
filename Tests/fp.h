#pragma once
#include "globalhelpers.h"


template <int n>
struct fp {
protected:
	explicit constexpr fp(__int64 i) : rawv(i) {}
public:
	__int64 rawv;
	fp() {}

	static constexpr fp from_int(__int64 i) noexcept { return fp(i << n); }
	static constexpr fp from_double(double d) noexcept { return fp(static_cast<__int64>(d * static_cast<double>(1I64 << n))); }
	static constexpr fp from_float(float d) noexcept { return fp(static_cast<__int64>(d * static_cast<float>(1I64 << n))); }
	static constexpr fp from_rawv(__int64 i) noexcept { return fp(i); }

	static constexpr fp minimum() noexcept { return fp(1); }
	static constexpr fp all_set() noexcept { return fp(mask<64>::value); }

	template<int m>
	static constexpr fp _from_fp(fp<m> in, std::true_type) noexcept { return fp(in.rawv >> (m - n)); }
	template<int m>
	static constexpr fp _from_fp(fp<m> in, std::false_type) noexcept { return fp(in.rawv << (n - m)); }
	template<int m>
	static constexpr fp from_fp(fp<m> in) noexcept { return _from_fp<m>(in, std::integral_constant<bool, (m > n)>()); }

	template<int m>
	friend struct fp;

	constexpr __int64 to_int() const noexcept {
		return rawv >> n;
	}
	constexpr unsigned __int64 to_uint() const noexcept {
		return (unsigned __int64)rawv >> n;
	}
	constexpr double to_double() const noexcept {
		return static_cast<double>(rawv) / static_cast<double>(1I64 << n);
	}
	constexpr float to_float() const noexcept {
		return static_cast<float>(rawv) / static_cast<float>(1I64 << n);
	}
	constexpr fp get_decimal() const noexcept {
		return fp(rawv & mask<n>::value);
	}
	constexpr __int64 to_rawv() const noexcept {
		return rawv;
	}

	constexpr fp floor() const noexcept {
		return fp(rawv & ~mask<n>::value);
	}
	fp& floor_eq() noexcept {
		rawv &= ~mask<n>::value;
		return *this;
	}
	constexpr fp abs() const noexcept {
		return fp(::abs(rawv));
	}

	constexpr fp operator+(fp<n> in) const noexcept {
		return fp(rawv + in.rawv);
	}
	constexpr fp operator-(fp<n> in) const noexcept {
		return fp(rawv - in.rawv);
	}
	constexpr fp operator-() const noexcept {
		return fp(-rawv);
	}
	fp operator*(fp<n> in) const noexcept {
		__int64 over;
		__int64 result = _mul128(in.rawv, rawv, &over);
		return fp(((unsigned __int64)result >> n) | (over << (64 - n)));
	}
	constexpr fp fmul(fp<n> in) const noexcept {
		return fp((in.rawv * rawv) >> n);
	}
	fp operator/(fp<n> in) const noexcept {
		return fp(asm_div(rawv << n, rawv >> (64 - n), in.rawv));
	}
	fp operator%(fp<n> in) const noexcept {
		return fp(rawv % in.rawv);
	}

	fp exact_div(fp<n> in) const noexcept {
		if (rawv == 0 || in.rawv == 0) return fp(0);

		const unsigned __int64 abn = ::abs(rawv);
		unsigned long nwidth = 0;
		_BitScanReverse64(&nwidth, abn);

		if (nwidth < 63 - n)
			return fp((rawv << n) / in.rawv);

		const unsigned __int64 abd = ::abs(in.rawv);
		unsigned long dwidth = 0;
		_BitScanReverse64(&dwidth, abd);

		__int64 pos = nwidth - dwidth;
		unsigned __int64 starting_value = abn >> pos;
		unsigned __int64 result_bits = 0;
		while (pos > 0) {
			result_bits <<= 1;
			if (starting_value >= abd) {
				result_bits |= 1I64;
				starting_value -= abd;
			}
			starting_value <<= 1;
			starting_value |= _bittest64((__int64*)&abn, --pos);
		}
		for (size_t cnt = 0; cnt < n + 1; ++cnt) {
			result_bits <<= 1;
			if (starting_value >= abd) {
				result_bits |= 1I64;
				starting_value -= abd;
			}
			starting_value <<= 1;
		}
		return fp((__int64)result_bits * ((rawv >= 0) == (in.rawv >= 0) ? 1 : -1));
	}

	void s_sqrt() noexcept {
		if (rawv <= 0) return;

		if ((n & 0x01) != 0)
			rawv <<= 1;

		unsigned __int64 root = 0;
		unsigned __int64 rem = 0;
		for (size_t i = 0; i < 32 + n / 2; ++i) {
			root <<= 1;
			rem = ((rem << 2) + ((unsigned __int64)rawv >> 62));
			rawv <<= 2;
			++root;
			if (root <= rem) {
				rem -= root;
				++root;
			} else {
				--root;
			}
		}

		root >>= 1;
		rawv = root;
	}

	fp sqrt() const noexcept {
		fp<n> t(*this);
		t.s_sqrt();
		return t;
	}

	fp fsqrt() const noexcept {
		unsigned long exponent = 0;
		_BitScanReverse64(&exponent, rawv);

		fp<62> base = fp<62>::from_rawv(rawv << (61 - exponent));
		exponent = exponent - (n - 1);

		fp<62> estimate = base * fp<62>::from_double(0.590178532) + fp<62>::from_double(0.417319242);

		estimate = (base / estimate + estimate) >> 1;
		estimate = (base / estimate + estimate) >> 1;
		estimate = (base / estimate + estimate) >> 1;

		if ((exponent & 0x01) != 0) {
			estimate *= fp<62>::from_double(0.707106781186547);
			++exponent;
		}
		if (exponent < 0)
			exponent /= 2;
		else
			exponent = (exponent + 1) / 2;

		return fp(estimate.rawv >> ((62 - n) - exponent));
	}

	fp isin() const {
		static const fp<60 - n> ipi = fp<60 - n>::from_double(1.0 / 3.1415926535897932384);
		static const fp<60> s1 = fp<60>::from_double(3.1415926535897932384);
		static const fp<60> s3 = fp<60>::from_double(5.1677127800499700292);
		static const fp<60> s5 = fp<60>::from_double(2.5501640398773454438);
		static const fp<60> s7 = fp<60>::from_double(0.5992645293207920768);

		_int64 over;
		__int64 result = _mul128(ipi.rawv, rawv, &over);

		fp<60> y(mask<60>::value & result);
		fp<60> z = y * y;
		fp<60> s = s1 - z * (s3 - z * (s5 - z * s7));
		return fp<n>::from_fp<60>(y * s);
	}

	void sincos(INOUT(fp<n>) s, INOUT(fp<n>) c) const noexcept {
		static const fp<n> pisix = fp<n>::from_double(3.1415926535897932384 / 6.0);
		static const fp<n> sixpi = fp<n>::from_double(6.0 / 3.1415926535897932384);
		static const fp<n> sin_30 = fp<n>::from_float(0.5f);
		static const fp<n> cos_30 = fp<n>::from_int(3).sqrt() / fp<n>::from_int(2);
		static const fp<n> st1 = fp<n>::from_double(1.0 / 20.0);
		static const fp<n> st2 = fp<n>::from_double(1.0 / 6.0);
		static const fp<n> ct1 = fp<n>::from_double(1.0 / 30.0);
		static const fp<n> ct2 = fp<n>::from_double(1.0 / 12.0);
		static const fp<n> one = fp<n>::from_int(1);

		//const fp<n> nv = (*this * sixpi + fp<n>::from_float(0.5f)).floor();
		fp<n> nv(*this);
		nv *= sixpi;
		nv += fp<n>::from_float(0.5f);
		nv.floor_eq();

		int inv = nv.to_int() % 12;

		nv *= pisix;
		fp<n> tmp(*this);
		tmp -= nv;
		//const fp<n> tmp = *this - (nv * pisix);

		if (inv < 0)
			inv += 12;
		fp<n> z(tmp);
		z *= tmp;
		//const fp<n> s1 = ((z * (st1) - one) * (z * (st2)) + one) * (tmp);
		//const fp<n> c1 = ((((z * (ct1) + one) * (z * (ct2)) - one) * (z)) >> 1) + one;
		fp<n> s1(z);
		fp<n> c1(z);
		(((((s1 *= st1) -= one) *= z) *= st2) += one) *= tmp;
		(((((((c1 *= ct1) += one) *= z) *= ct2) -= one) *= z) >>= 1) += one;

		switch (inv) {
		case 0: s = s1; c = c1; return;
		case 1: ((s = cos_30) *= s1) += (sin_30 * c1); ((c = -sin_30) *= s1) += (cos_30 * c1); return;
		case 2: ((s = sin_30) *= s1) += (cos_30 * c1); ((c = -cos_30) *= s1) += (sin_30 * c1); return;
		case 3: s = c1; c = -s1; return;
		case 4: ((s = -sin_30) *= s1) += (cos_30 * c1); ((c = -cos_30) *= s1) -= (sin_30 * c1); return;
		case 5: ((s = -cos_30) *= s1) += (sin_30 * c1); ((c = -sin_30) *= s1) -= (cos_30 * c1); return;
		case 6: s = -s1; c = -c1; return;
		case 7: ((s = -cos_30) *= s1) -= (sin_30 * c1); ((c = sin_30) *= s1) -= (cos_30 * c1); return;
		case 8: ((s = -sin_30) *= s1) -= (cos_30 * c1); ((c = cos_30) *= s1) -= (sin_30 * c1); return;
		case 9: s = -c1; c = s1; return;
		case 10: ((s = sin_30) *= s1) -= (cos_30 * c1); ((c = cos_30) *= s1) += (sin_30 * c1); return;
		case 11: ((s = cos_30) *= s1) -= (sin_30 * c1); ((c = sin_30) *= s1) += (cos_30 * c1); return;
		default: return;
		}
	}

	void s_atan() noexcept {
		static const fp<n> lhpi = fp<n>::from_double(3.1415926535897932384 / 2.0);
		static const fp<n> b = fp<n>::from_double(3.1415926535897932384 / 6.0);
		static const fp<n> k = fp<n>::from_double(0.57735026918962576450914878050196);
		static const fp<n> b0 = fp<n>::from_double(3.1415926535897932384 / 12.0);
		static const fp<n> k0 = fp<n>::from_double(0.26794919243112270647255365849413);

		static const fp<n> A = fp<n>::from_double(0.999999020228907);
		static const fp<n> B = fp<n>::from_double(0.257977658811405);
		static const fp<n> C = fp<n>::from_double(0.59120450521312);

		bool comp = false;
		bool hiseg = false;

		bool sign = rawv < 0;
		if (sign) ch_sign();

		if (*this > fp<n>::from_int(1)) {
			comp = true;
			inv();
		}
		if (*this > k0) {
			hiseg = true;
			fp<n> xd(*this);
			++(xd *= k);
			(*this -= k) /= xd;
			//+x = (x - k) / (fp<n>::from_int(1) + k * x);
		}

		fp<n> nx2(*this);
		nx2 *= *this;
		fp<n> dx2(nx2);

		//fp<n> ang = x * (A + B * x2) / (fp<n>::from_int(1) + C * x2);
		++(dx2 *= C);
		(((nx2 *= B) += A) *= *this) /= dx2;

		if (hiseg)
			nx2 += b;
		if (comp)
			(nx2 -= lhpi).ch_sign();
		if (sign)
			nx2.ch_sign();
		*this = nx2;
	}

	fp atan() const noexcept {
		fp<n> t = *this;
		t.s_atan();
		return t;
	}

	void s_acos() noexcept {
		static const fp<n> lhpi = fp<n>::from_double(3.1415926535897932384 / 2.0);
		if (*this >= fp<n>::from_int(1)) {
			*this = fp<n>::from_int(0);
			return;
		}
		if (*this <= fp<n>::from_int(-1)) {
			*this = fp<n>::from_int(1);
			return;
		}
		fp<n> den(*this);
		(den *= *this) -= fp<n>::from_int(1);
		den.ch_sign();
		den.s_sqrt();
		*this /= den;
		s_atan();
		*this -= lhpi;
		ch_sign();

		//return hpi - (*this / (fp<n>::from_int(1) - ((*this) * (*this))).sqrt()).atan();
	}

	fp acos() const noexcept {
		fp<n> t(*this);
		t.s_acos();
		return t;
	}

	fp asin() const noexcept {
		static const fp<n> hpi = fp<n>::from_double(3.1415926535897932384 / 2.0);

		if (*this >= fp<n>::from_int(1)) return hpi;
		if (*this <= fp<n>::from_int(-1)) return -hpi;

		return (*this / (fp<n>::from_int(1) - ((*this) * (*this))).sqrt()).atan();
	}

	fp& operator++() noexcept {
		rawv += 1i64 << n;
		return *this;
	}
	fp& ch_sign() noexcept {
		rawv = -rawv;
		return *this;
	}
	fp& operator--() noexcept {
		rawv -= 1i64 << n;
		return *this;
	}
	constexpr fp operator >> (int s) const noexcept {
		return fp(rawv >> s);
	}
	constexpr fp operator<<(int s) const noexcept {
		return fp(rawv << s);
	}
	fp& operator>>=(int s) noexcept {
		rawv >>= s;
		return *this;
	}
	fp& operator<<=(int s) noexcept {
		rawv <<= s;
		return *this;
	}
	fp& operator+=(fp<n> in) noexcept {
		rawv += in.rawv;
		return *this;
	}
	fp& operator-=(fp<n> in) noexcept {
		rawv -= in.rawv;
		return *this;
	}
	fp& operator*=(fp<n> in) noexcept {
		__int64 over;
		__int64 result = _mul128(in.rawv, rawv, &over);
		rawv = ((unsigned __int64)result >> n) | (over << (64 - n));
		return *this;
	}
	fp& operator/=(fp<n> in) noexcept {
		rawv = asm_div(rawv << n, rawv >> (64 - n), in.rawv);
		return *this;
	}
	fp& _inv(std::true_type) noexcept {
		rawv = (1i64 << (2 * n)) / rawv;
		return *this;
	}
	fp& _inv(std::false_type) noexcept {
		rawv = asm_div(0, 1i64 << ((2 * n) - 64), rawv);
		return *this;
	}
	fp& inv() noexcept {
		return _inv(std::integral_constant<bool, (2 * n) < 64>());
	}
	constexpr bool operator==(fp<n> in) const noexcept {
		return rawv == in.rawv;
	}
	constexpr bool operator!=(fp<n> in) const noexcept {
		return rawv != in.rawv;
	}
	constexpr bool operator<(fp<n> in) const noexcept {
		return rawv < in.rawv;
	}
	constexpr bool operator>(fp<n> in) const noexcept {
		return rawv > in.rawv;
	}
	constexpr bool operator>=(fp<n> in) const noexcept {
		return rawv >= in.rawv;
	}
	constexpr bool operator<=(fp<n> in) const noexcept {
		return rawv <= in.rawv;
	}
};

template<int n>
fp<n> sqrt(fp<n> v) {
	return v.sqrt();
}

using std_fp = fp<16>;
using half_fp = fp<32>;