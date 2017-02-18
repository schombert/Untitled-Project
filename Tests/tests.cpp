#include "globalhelpers.h"
#include "i18n.h"
#include "gtest/gtest.h"
#include "test_helpers.h"
#include "laws.h"
#include "structs.hpp"
#include "datamanagement.hpp"
#include "prov_control.h"
#include "ai_interest.h"
#include "living_data.h"
#include "traits.h"
#include "relations.h"
#include "fp.h"
#include "..\\uigeneration\\fileparsing_v2.h"
#include "nlp.h"

TEST(fp_tests, basic_tests) {
	EXPECT_EQ(std_fp::from_double(1.5)*std_fp::from_int(2), std_fp::from_int(3));
	EXPECT_EQ(std_fp::from_double(1.5)+std_fp::from_double(1.5), std_fp::from_int(3));
	EXPECT_EQ(std_fp::from_double(3) / std_fp::from_double(1.5), std_fp::from_int(2));
	EXPECT_EQ(std_fp::from_double(1.5) - std_fp::from_double(1.5), std_fp::from_int(0));
	EXPECT_EQ(std_fp::from_int(17).to_int(), 17);
	EXPECT_EQ(std_fp::from_double(1.25).to_double(), 1.25);
	EXPECT_EQ(std_fp::from_double(std_fp::from_double(1.25).to_double()), std_fp::from_double(1.25));
}

TEST(fp_tests, sign_tests) {
	EXPECT_EQ(std_fp::from_double(1.5)*std_fp::from_int(-2), std_fp::from_int(-3));
	EXPECT_EQ(std_fp::from_double(-1.5)*std_fp::from_int(2), std_fp::from_int(-3));
	EXPECT_EQ(std_fp::from_double(-1.5)*std_fp::from_int(-2), std_fp::from_int(3));
	EXPECT_EQ(std_fp::from_double(-3) / std_fp::from_double(1.5), std_fp::from_int(-2));
	EXPECT_EQ(std_fp::from_double(3) / std_fp::from_double(-1.5), std_fp::from_int(-2));
	EXPECT_EQ(std_fp::from_double(-3) / std_fp::from_double(-1.5), std_fp::from_int(2));
}

TEST(fp_tests, fmul) {
	EXPECT_EQ(std_fp::from_double(1.5).fmul(std_fp::from_int(2)), std_fp::from_int(3));
	EXPECT_EQ(std_fp::from_double(1.5).fmul(std_fp::from_int(-2)), std_fp::from_int(-3));
	EXPECT_EQ(std_fp::from_double(-1.5).fmul(std_fp::from_int(2)), std_fp::from_int(-3));
	EXPECT_EQ(std_fp::from_double(-1.5).fmul(std_fp::from_int(-2)), std_fp::from_int(3));
}


TEST(fp_tests, multiply_test) {
	EXPECT_EQ(std_fp::from_int(0x24696AEAB422)*std_fp::from_int(2),std_fp::from_int(0x48D2D5D56844));
}

TEST(fp_tests,divide_test) {
	EXPECT_EQ((std_fp::from_int(0x48D2D5D56844) / std_fp::from_int(2)).to_int(), 0x24696AEAB422);
	EXPECT_EQ(std_fp::from_int(0x48D2D5D56844) / std_fp::from_int(0x24696AEAB422), std_fp::from_int(2));
}

TEST(fp_tests, exact_divide_test) {
	EXPECT_EQ(std_fp::from_double(3).exact_div(std_fp::from_double(1.5)), std_fp::from_int(2));
	EXPECT_EQ(std_fp::from_double(-3).exact_div(std_fp::from_double(1.5)), std_fp::from_int(-2));
	EXPECT_EQ(std_fp::from_double(3).exact_div(std_fp::from_double(-1.5)), std_fp::from_int(-2));
	EXPECT_EQ(std_fp::from_double(-3).exact_div(std_fp::from_double(-1.5)), std_fp::from_int(2));
	EXPECT_EQ(std_fp::from_int(128).exact_div(std_fp::from_int(2)).to_int(), 64);
	EXPECT_EQ(std_fp::from_int(1028).exact_div(std_fp::from_int(2)).to_int(), 514);
	EXPECT_EQ(std_fp::from_int(1028).exact_div(std_fp::from_int(1028)).to_int(), 1);
	EXPECT_EQ((std_fp::from_int(0x48D2D5D56844).exact_div(std_fp::from_int(2))).to_int(), 0x24696AEAB422);
	EXPECT_EQ((std_fp::from_int(0x48D2D5D56844).exact_div(std_fp::from_int(4))).to_int(), 0x1234B5755A11);

}

TEST(fp_tests, sqrt_test) {
	EXPECT_DOUBLE_EQ(2.0, std_fp::from_int(4).sqrt().to_double());
	EXPECT_DOUBLE_EQ(4.0, std_fp::from_int(16).sqrt().to_double());
	EXPECT_DOUBLE_EQ(std_fp::from_double(1.414213562373095).to_double(), std_fp::from_int(2).sqrt().to_double());
	EXPECT_DOUBLE_EQ(fp<17>::from_double(1.414213562373095).to_double(), fp<17>::from_int(2).sqrt().to_double());
	EXPECT_DOUBLE_EQ(4.0, half_fp::from_int(16).sqrt().to_double());
}

TEST(fp_tests, sincos_test) {
	std_fp sval;
	std_fp cval;


	for (double i = -7.0; i < 7.0; i += 0.01) {
		std_fp ps = std_fp::from_double(i);
		ps.sincos(sval, cval);

		EXPECT_LT(sin(i) - 0.00015, sval.to_double());
		EXPECT_LT(cos(i) - 0.00015, cval.to_double());

		EXPECT_GT(sin(i) + 0.00015, sval.to_double());
		EXPECT_GT(cos(i) + 0.00015, cval.to_double());
	}
}

TEST(fp_tests, sincos_halftest) {
	half_fp sval;
	half_fp cval;


	for (double i = -7.0; i < 7.0; i += 0.01) {
		half_fp ps = half_fp::from_double(i);
		ps.sincos(sval, cval);

		EXPECT_LT(sin(i) - 0.0000015, sval.to_double());
		EXPECT_LT(cos(i) - 0.0000015, cval.to_double());

		EXPECT_GT(sin(i) + 0.0000015, sval.to_double());
		EXPECT_GT(cos(i) + 0.0000015, cval.to_double());
	}
}

TEST(fp_tests, isin) {
	for (double i = 0; i < 2.0; i += 0.1) {
		half_fp ps = half_fp::from_double(i);
		half_fp result = ps.isin();

		//EXPECT_LT(sin(i) - 0.00003, result.to_double());
		//EXPECT_GT(sin(i) + 0.00003, result.to_double());
	}
}

TEST(fp_tests, atan) {
	for (double i = -7.0; i < 7.0; i += 0.01) {
		half_fp ps = half_fp::from_double(i);
		half_fp result = ps.atan();

		EXPECT_LT(atan(i) - 0.0000015, result.to_double());
		EXPECT_GT(atan(i) + 0.0000015, result.to_double());
	}
}

TEST(fp_tests, acos) {
	for (double i = -0.99; i < 1.0; i += 0.01) {
		half_fp ps = half_fp::from_double(i);
		half_fp result = ps.acos();

		EXPECT_LT(acos(i) - 0.0000015, result.to_double());
		EXPECT_GT(acos(i) + 0.0000015, result.to_double());
	}
}

TEST(fp_tests, asin) {
	for (double i = -0.99; i < 1.0; i += 0.01) {
		half_fp ps = half_fp::from_double(i);
		half_fp result = ps.asin();

		EXPECT_LT(asin(i) - 0.0000015, result.to_double());
		EXPECT_GT(asin(i) + 0.0000015, result.to_double());
	}
}

TEST(fp_tests, fast_sqrt_test) {
	EXPECT_DOUBLE_EQ(std_fp::from_int(4).fsqrt().to_double(), 1.9999847412109375);
	EXPECT_DOUBLE_EQ(std_fp::from_int(2).fsqrt().to_double(), std_fp::from_double(1.41421356237309504).to_double());
}

TEST(random_tests, bit_test) {
	unsigned __int64 accumulation = 0;
	for (int i = 0; i < 256; ++i) {
		accumulation |= global_store.get_value();
	}
	EXPECT_EQ(accumulation, 0xFFFFFFFFFFFFFFFF);
}


/*TEST(random_tests, distribution_test) {
	std_fp chances[] = {std_fp::from_float(0.5), std_fp::from_int(1), std_fp::from_float(1.5f)};
	unsigned int errors = 0;
	unsigned int counts[] = {0,0,0};
	for (size_t j = 0; j < 10000; ++j) {
		size_t res = get_from_distribution<3>(chances);
		if (res == SIZE_MAX) {
			++errors;
		} else {
			++counts[res];
		}
	}

	EXPECT_EQ(errors, 0);
	EXPECT_GE(counts[0] * 2, counts[1] - 100);
	EXPECT_LE(counts[0] * 2, counts[1] + 100);
	EXPECT_GE((counts[0] + counts[1]), counts[2] - 150);
	EXPECT_LE((counts[0] + counts[1]), counts[2] + 150);
}/**/

class int_clear_tc {
public:
	int value = 0;
	int_clear_tc() {};
	int_clear_tc(int v) : value(v) {};
	
	void construct(int n) {
		value = n;
	}
	void set_clear() {
		value = 0;
	}
	bool is_clear() {
		return value == 0;
	}
	bool operator==(int n) const {
		return value == n;
	}
};

TEST(containers, v_pool) {
	v_pool<int_clear_tc> pool;
	w_lock lk;
	const auto tg1 = pool.add(1, lk);
	const auto tg2 = pool.add(2, lk);
	const auto tg3 = pool.add(3, lk);
	const auto tg4 = pool.add(4, lk);
	const auto tg5 = pool.emplace(lk, 5);

	EXPECT_EQ(pool.pool.size(), 5);
	EXPECT_EQ(tg3, 2);
	EXPECT_EQ(tg5, 4);
	EXPECT_EQ(pool.get(tg3, lk), 3);
	EXPECT_EQ(pool.get(tg5, lk), 5);
	pool.free(tg2, lk);
	EXPECT_EQ(pool.first_free, tg2);
	EXPECT_EQ(pool.last_free, tg2);
	pool.free(tg4, lk);
	EXPECT_EQ(pool.last_free, tg4);

	const auto tg6 = pool.add(6, lk);
	const auto tg7 = pool.add(7, lk);

	EXPECT_EQ(pool.pool.size(), 5);
	EXPECT_EQ(pool.first_free, UINT_MAX);
	EXPECT_EQ(tg6, tg2);
	EXPECT_EQ(tg7, tg4);
	EXPECT_EQ(pool.get(tg4, lk), 7);
}

TEST(containers, v_vector) {
	v_vector_t<int, unsigned int> vv;
	vv.push_back(5);
	vv.push_back(3);
	vv.new_row();
	vv.new_row();
	vv.push_back(1);
	vv.push_back(0);
	vv.push_back(7);
	vv.new_row();
	vv.push_back(9);

	EXPECT_EQ(vv.row_size(), 4);
	EXPECT_EQ(vv.elements.size(), 6);
	EXPECT_EQ(vv.get(0, 1), 3);
	EXPECT_EQ(vv.get(0, 0), 5);
	EXPECT_EQ(vv.get(2, 0), 1);
	EXPECT_EQ(vv.get(2, 2), 7);
	EXPECT_EQ(vv.get(3, 0), 9);
	int sum = 0;
	for (auto p = vv.get_row(1); p.first != p.second; ++p.first)
		sum += *p.first;
	EXPECT_EQ(sum, 0);
	sum = 0;
	for (auto p = vv.get_row(2); p.first != p.second; ++p.first)
		sum += *p.first;
	EXPECT_EQ(sum, 8);

	auto p = vv.get_row(2);
	++p.first;
	vv.insert(p.first, 2);

	EXPECT_EQ(vv.get(2, 0), 1);
	EXPECT_EQ(vv.get(2, 1), 2);
	EXPECT_EQ(vv.get(2, 2), 0);
	EXPECT_EQ(vv.get(2, 3), 7);
	EXPECT_EQ(vv.get(3, 0), 9);

	p = vv.get_row(1);
	vv.insert(p.first, -1);

	sum = 0;
	for (auto p = vv.get_row(1); p.first != p.second; ++p.first)
		sum += *p.first;
	EXPECT_EQ(sum, -1);

	EXPECT_EQ(vv.get(1, 0), -1);
	EXPECT_EQ(vv.get(2, 0), 1);
	EXPECT_EQ(vv.get(2, 3), 7);
	EXPECT_EQ(vv.get(3, 0), 9);

	vv.add_to_row(0, 10);
	EXPECT_EQ(vv.get(0, 0), 10);
	EXPECT_EQ(vv.get(1, 0), -1);
	EXPECT_EQ(vv.get(0, 2), 3);

	vv.append_to_row(0, 12);
	EXPECT_EQ(vv.get(0, 3), 12);
	EXPECT_EQ(vv.get(1, 0), -1);
}

TEST(containers, indexes) {
	multiindex<unsigned short, int> mi;
	w_lock l;
	mi.insert(1, -2, l);
	mi.insert(10, 5, l);
	mi.insert(101, 7, l);
	mi.insert(101, 8, l);
	mi.insert(1, 10, l);

	EXPECT_EQ(mi.count(10, l), 1);
	EXPECT_EQ(mi.count(1, l), 2);
	EXPECT_EQ(mi.count(101, l), 2);
	EXPECT_EQ(mi.count(394, l), 0);

	int sum = 0;
	mi.for_each(101, l, [&sum](int v) { sum += v; });
	EXPECT_EQ(sum, 15);
	sum = 0;
	mi.for_each(1, l, [&sum](int v) { sum += v; });
	EXPECT_EQ(sum, 8);
	sum = 0;
	mi.for_all(l, [&sum](unsigned int i, int v) { sum += v; });
	EXPECT_EQ(sum, 28);

	mi.range_erase_if(1, l, [](IN(std::pair<unsigned short, int>) p) { return p.second < 0; });
	sum = 0;
	mi.for_each(1, l, [&sum](int v) { sum += v; });
	EXPECT_EQ(sum, 10);

	mi.eraseall(101, l);
	EXPECT_EQ(mi.count(101, l), 0);

	mi.insert(10, 1, l);
	mi.insert(10, 2, l);
	std::vector<int> v;
	mi.to_vector(10, v, l);
	EXPECT_EQ(v.size(), 3);

	sum = 0;
	for (int i : v)
		sum += i;
	EXPECT_EQ(sum, 8);

	auto rng = mi.range(10, l);
	EXPECT_EQ(std::distance(rng.first, rng.second), 3);
};

TEST(iterators, generating) {
	const auto f = [](unsigned int i) {return i*i; };
	using ittype = generating_iterator<int, unsigned int, decltype(f)>;
	const std::vector<unsigned int> base{0, 1, 2, 3, 4, 5, 6};
	const std::vector<int> result(ittype(f, base.begin()), ittype(f, base.end()));
	EXPECT_EQ(result[0], 0);
	EXPECT_EQ(result[1], 1);
	EXPECT_EQ(result[2], 4);
	EXPECT_EQ(result[3], 9);
	EXPECT_EQ(result[4], 16);
	EXPECT_EQ(result[5], 25);
	EXPECT_EQ(result[6], 36);
}

TEST(iterators, counting) {
	const std::vector<int> base(counting_iterator<1>(0), counting_iterator<1>(10));
	EXPECT_EQ(base.size(), 10);
	EXPECT_EQ(base[5], 5);
}

class t_double {
public:
	static int apply(int i) {
		return i * 2;
	}
};

TEST(iterators, transforming) {
	std::vector<int> base = {1, 2, 3, 4, 5};
	int sum = 0;
	int loops = 0;
	for (auto it = transforming_iterator<t_double, decltype(base.begin())>(base.begin()); it != base.end(); ++it) {
		++loops;
		sum += *it;
	}
	EXPECT_EQ(loops, 5);
	EXPECT_EQ(sum, 30);
}

class object_w_range {
public:
	std::vector<int> innerrange;
	object_w_range(int n) {
		for (int i = 0; i != n; ++i) {
			innerrange.push_back(i);
		}
	}
};

class test_subrange {
public:
	using reference = int&;
	using pointer = int*;
	using iterator_type = decltype(std::vector<int>().begin());

	static iterator_type begin(INOUT(object_w_range) o) {
		return o.innerrange.begin();
	}
	static iterator_type end(INOUT(object_w_range) o) {
		return o.innerrange.end();
	}
};

TEST(iterators, flattening) {
	std::vector<object_w_range> base;
	base.emplace_back(2);
	base.emplace_back(4);
	base.emplace_back(3);
	auto it = flattening_iterator<test_subrange, decltype(base.begin())>(base.begin());
	EXPECT_EQ(*it, 0);
	++it;
	EXPECT_EQ(*it, 1);
	++it;
	EXPECT_EQ(*it, 0);
	++it;
	EXPECT_EQ(*it, 1);
	++it;
	EXPECT_EQ(*it, 2);
	++it;
	EXPECT_EQ(*it, 3);
	++it;
	EXPECT_EQ(*it, 0);
	++it;
	EXPECT_EQ(*it, 1);
	++it;
	EXPECT_EQ(*it, 2);
	++it;
	EXPECT_EQ(it, base.end());

	int cnt = 0;
	for (auto it2 = flattening_iterator<test_subrange, decltype(base.begin())>(base.begin()); it2 != base.end(); ++it2)
		++cnt;
	EXPECT_EQ(cnt, 9);

}

TEST(i18n_test, i18n_test) {

	init_label_numbers();
	load_text_file(TEXT("D:\\documents\\VCPP\\CCL\\CCL\\text.txt"));
	clear_label_numbers();

	auto res = get_simple_string(TX_YES);
	EXPECT_STREQ(TEXT("Yes"), res.c_str());

	size_t p = 2;
	res = get_p_string(TX_N_SUF, &p, 1);
	EXPECT_STREQ(TEXT("nd"), res.c_str());

	p = 6;
	res = get_p_string(TX_MONTH, &p, 1);
	EXPECT_STREQ(TEXT("June"), res.c_str());

	size_t d[3] = {3, 6, 1994};
	res = get_p_string(TX_DATE, d, 3);
	EXPECT_STREQ(TEXT("June 3rd, 1994"), res.c_str());

	res = get_p_string(TX_S_DATE, d, 3);
	EXPECT_STREQ(TEXT("Jun 3, 1994"), res.c_str());

	h_link_ident h;
	p = 5;
	res = get_p_string(TX_TEST, &p, 1);
	EXPECT_STREQ(TEXT("Tplural"), res.c_str());
	
	p = 1;
	res = get_p_string(TX_TEST, &p, 1);
	EXPECT_STREQ(TEXT("Tsingular"), res.c_str());
	
	EXPECT_STREQ(TEXT("Call to arms"), get_simple_string(TX_L_CTOARMS).c_str());
	EXPECT_STREQ(TEXT("Invitation"), get_simple_string(TX_L_INVITATION).c_str());
}

TEST(parse_v2, basic) {
	std::vector<parse_option> results;
	EXPECT_TRUE(parse_v2(L"fparse2_test.txt", results));

	EXPECT_EQ(3, results.size());
	EXPECT_STREQ("entry_1", results[0].node.first.get().c_str());
	EXPECT_EQ(6, results[0].node.second.size());
	EXPECT_STREQ("content 1", results[0].node.second[0].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[0].node.second.size());
	EXPECT_STREQ("content 2", results[0].node.second[1].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[1].node.second.size());
	EXPECT_STREQ("sub_1", results[0].node.second[2].node.first.get().c_str());
	EXPECT_EQ(2, results[0].node.second[2].node.second.size());
	EXPECT_STREQ("sv1", results[0].node.second[2].node.second[0].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[2].node.second[0].node.second.size());
	EXPECT_STREQ("sv3", results[0].node.second[2].node.second[1].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[2].node.second[1].node.second.size());
	EXPECT_STREQ("sub_2", results[0].node.second[3].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[3].node.second.size());
	EXPECT_STREQ("sub_3", results[0].node.second[4].node.first.get().c_str());
	EXPECT_EQ(1, results[0].node.second[4].node.second.size());
	EXPECT_STREQ("sv2", results[0].node.second[4].node.second[0].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[4].node.second[0].node.second.size());
	EXPECT_STREQ("content3", results[0].node.second[5].node.first.get().c_str());
	EXPECT_EQ(0, results[0].node.second[5].node.second.size());
	EXPECT_STREQ("entry_2", results[1].node.first.get().c_str());
	EXPECT_EQ(1, results[1].node.second.size());
	EXPECT_STREQ("content 4", results[1].node.second[0].node.first.get().c_str());
	EXPECT_EQ(0, results[1].node.second[0].node.second.size());
	EXPECT_STREQ("top content", results[2].node.first.get().c_str());
	EXPECT_EQ(0, results[2].node.second.size());
}

struct struct_aggl {
	char_id_t chid;
	admin_id_t admid;
	title_id_t tid;
};

TEST(data_integrity_test, basic_test) {
	EXPECT_TRUE(load_test_data());
	EXPECT_GT(detail::provinces.size(), 100);
	EXPECT_GT(detail::people.size(), 100);
	EXPECT_GT(detail::titles.size(), 100);
	EXPECT_GT(admin_pool.pool.size(), 100);
	EXPECT_GT(global::control_pool.pool.size(), 100);
	EXPECT_GT(global::dj_pool.pool.size(), 100);

	EXPECT_EQ(sizeof(admin_id_t), sizeof(admin_id));
	EXPECT_EQ(sizeof(struct_aggl), sizeof(admin_id) + sizeof(char_id) + sizeof(title_id));
	//EXPECT_EQ(no_value<admin_id_t>.value, admin_id_t::NONE);
	//EXPECT_EQ(no_value<admin_id_t>, admin_id_t(admin_id_t::NONE));
	//EXPECT_EQ(no_value<admin_id_t>, admin_id_t());
}

TEST(data_integrity_test, living_executives) {
	EXPECT_TRUE(load_test_data());
	char_id_t dead_executive;
	admin_pool.for_each(fake_lock(), [&dead_executive](IN(administration) adm) {
		if (get_object(adm.executive).died != 0) {
			dead_executive = adm.executive;
		}
	});
	EXPECT_EQ(dead_executive, char_id_t());
}

TEST(data_integrity_test, basic_ownership) {
	EXPECT_TRUE(load_test_data());
	const auto france_title = title_from_name("France");
	EXPECT_TRUE(valid_ids(france_title));

	bool only_one_france_admin = true;
	admin_id_t fadmin;

	r_lock l;
	admin_pool.for_each(l, [&only_one_france_admin, &fadmin, &l, france_title](IN(administration) adm) {
		if (adm.associated_title == france_title) {
			if (!valid_ids(fadmin)) {
				fadmin = admin_id_t(admin_pool.get_index(adm, l));
			} else {
				only_one_france_admin = false;
			}
		}
	});

	EXPECT_TRUE(valid_ids(fadmin));
	EXPECT_TRUE(only_one_france_admin);

	const auto paris = province_from_name("Paris");
	EXPECT_TRUE(valid_ids(paris));

	EXPECT_TRUE(is_controlled_by_a(paris, fadmin, l));
	EXPECT_TRUE(is_controlled_under_t(paris, france_title, l));
	EXPECT_TRUE(is_dj(paris, france_title, l));

	std::vector<unsigned int> owned;
	global::provtowner.to_vector(paris, owned, l);
	EXPECT_NE(owned.size(), 0);


	const auto k_fr = head_of_state(fadmin, l);
	EXPECT_TRUE(valid_ids(k_fr));

	//const auto ud = get_udata(k_fr, l);
	//const auto td = get_tdata(k_fr, l);

	//EXPECT_NE(ud, nullptr);
	//EXPECT_NE(td, nullptr);

	// EXPECT_EQ(td->executive_of, fadmin);

	const auto aust = province_from_name("Austisland");
	const auto vest = province_from_name("Vestisland");
	EXPECT_TRUE(valid_ids(aust, vest));

	bool touches_vest = false;
	bool touches_other_titled = false;
	for (auto pr = global::province_connections.get_row(aust.value); pr.first != pr.second; ++pr.first) {
		if (vest == prov_id_t(*pr.first)) {
			touches_vest = true;
		} else if (P_HAS_TITLE(*pr.first)) {
			touches_other_titled = true;
		}
	}

	EXPECT_TRUE(touches_vest);
	EXPECT_FALSE(touches_other_titled);

	touches_other_titled = false;
	bool touches_aust = false;
	for (auto pr = global::province_connections.get_row(vest.value); pr.first != pr.second; ++pr.first) {
		if (aust == prov_id_t(*pr.first)) {
			touches_aust = true;
		} else if (P_HAS_TITLE(*pr.first)) {
			touches_other_titled = true;
		}
	}

	EXPECT_TRUE(touches_aust);
	EXPECT_FALSE(touches_other_titled);
}



TEST(relations, interest) {
	EXPECT_TRUE(load_test_data());

	const auto ger = admin_from_name("Germany");
	const auto pmr = admin_from_name("Pomerania");
	const auto kmk = admin_from_name("Kimak");
	EXPECT_TRUE(valid_ids(ger, pmr, kmk));

	r_lock l;
	const auto k_ger = head_of_state(ger, l);
	const auto k_pmr = head_of_state(pmr, l);
	const auto d_kmk = head_of_state(kmk, l);

	EXPECT_TRUE(valid_ids(k_ger, k_pmr, d_kmk));

	cvector<char_id_t> current_neighbors;
	global::get_nearby_independant(k_ger, current_neighbors, l);
	EXPECT_GT(current_neighbors.size(), 0);

	current_neighbors.clear();
	global::get_nearby_independant(k_pmr, current_neighbors, l);
	EXPECT_GT(current_neighbors.size(), 0);

	bool inlist = false;
	for (auto cid : current_neighbors) {
		if (k_ger == cid)
			inlist = true;
	}
	EXPECT_TRUE(inlist);

	inlist = false;
	for (auto pr = interested_in.equal_range(char_id_t(k_ger)); pr.first != pr.second; ++pr.first) {
		if (k_pmr == pr.first->second)
			inlist = true;
	}
	EXPECT_TRUE(inlist);
	inlist = false;
	for (auto pr = interested_in.equal_range(char_id_t(k_pmr)); pr.first != pr.second; ++pr.first) {
		if (k_ger == pr.first->second)
			inlist = true;
	}
	EXPECT_TRUE(inlist);

	EXPECT_EQ(interest_status_of(k_ger, k_pmr, l), interest_relation_contents::TAG_TARGET);
	EXPECT_EQ(interest_status_of(k_pmr, k_ger, l), interest_relation_contents::TAG_THREAT);
	EXPECT_EQ(interest_status_of(k_pmr, d_kmk, l), interest_relation_contents::TAG_NONE);
	EXPECT_EQ(interest_status_of(d_kmk, k_pmr, l), interest_relation_contents::TAG_NONE);
}

TEST(relations, feeling) {
	EXPECT_TRUE(load_test_data());

	EXPECT_TRUE(global::living.size() != 0);
	char_id_t p1 = *global::living.begin();
	char_id_t p2;
	
	w_lock l;

	for (auto it = global::living.begin() + 1; it != global::living.end(); ++it) {
		if (similarity_score(p1, *it, l) == 0) {
			p2 = *it;
			break;
		}
	}

	EXPECT_TRUE(valid_ids(p2));

	
	EXPECT_EQ(get_feeling(p1, p2, l), 0);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 0);

	adjust_relation(p1, p2, 0, l);
	adjust_relation(p1, p2, 0, l);
	adjust_relation_symmetric(p2, p1, 0, l);
	adjust_relation_symmetric(p1, p2, 0, l);

	EXPECT_EQ(get_feeling(p1, p2, l), 0);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 0);

	adjust_relation(p1, p2, 5, l);
	adjust_relation(p1, p2, 5, l);

	EXPECT_EQ(get_feeling(p1, p2, l), 1);
	EXPECT_EQ(get_feeling(p2, p1, l), 0);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 0);

	adjust_relation(p2, p1, 5, l);
	adjust_relation(p2, p1, 5, l);

	EXPECT_EQ(get_feeling(p1, p2, l), 1);
	EXPECT_EQ(get_feeling(p2, p1, l), 1);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 1);

	remove_all_relations(p1, l);

	EXPECT_EQ(get_feeling(p1, p2, l), 0);
	EXPECT_EQ(get_feeling(p2, p1, l), 0);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 0);


	float initial_p1_opinion = opinion(p1, p2, l);

	adjust_relation_symmetric(p2, p1, 5, l);
	adjust_relation_symmetric(p1, p2, 5, l);

	EXPECT_EQ(get_feeling(p1, p2, l), 1);
	EXPECT_EQ(get_feeling(p2, p1, l), 1);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 1);

	EXPECT_GT(opinion(p1, p2, l), initial_p1_opinion);

	remove_all_relations(p1, l);

	adjust_relation(p1, p2, -5, l);
	adjust_relation(p1, p2, -5, l);

	EXPECT_EQ(get_feeling(p1, p2, l), -1);
	EXPECT_EQ(get_feeling(p2, p1, l), 0);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), 0);

	adjust_relation_symmetric(p2, p1, -5, l);
	adjust_relation_symmetric(p1, p2, -5, l);

	EXPECT_EQ(get_feeling(p1, p2, l), -1);
	EXPECT_EQ(get_feeling(p2, p1, l), -1);
	EXPECT_EQ(get_mutual_feeling(p1, p2, l), -1);

	EXPECT_LT(opinion(p1, p2, l), initial_p1_opinion);
}

TEST(nlp, basic_functions) {
	matrix_type test_matrix((value_type*)_alloca(sizeof(value_type) * 24), 4, 6);
	test_matrix <<	2, 0, 0, 1, 10, 5,
					0, 1, 3, 0, 10, 5,
					0, 0, 1.5, 0, 10, 5,
					0, 0, 0, 1, 10, 5;
	rvector_type test_vector((value_type*)_alloca(sizeof(value_type) * 4), 4);
	rvector_type test_result((value_type*)_alloca(sizeof(value_type) * 4), 4);
	test_vector << 1, 2, 3, 4;
	test_result.noalias() = rvector_type::Zero(4);

	matrix_type test_matrix_LU_store((value_type*)_alloca(sizeof(value_type) * 16), 4, 4);
	test_matrix_LU_store.noalias() = test_matrix.leftCols(4);
	Eigen::PartialPivLU<Eigen::Ref<matrix_type>> LU_decomp(test_matrix_LU_store);

	EXPECT_TRUE(test_matrix.leftCols(4).isUpperTriangular());

	// tv * tm-1 = tr
	// tv = tr * tm
	vector_times_ut_inverse(test_vector, test_matrix, test_result);
	//tv(0) = 2 * tr(0) = 0.5
	EXPECT_DOUBLE_EQ(0.5, test_result(0));
	//tv(1) = 0 * tr(0) + 1 * tr(1) = 2
	EXPECT_DOUBLE_EQ(2.0, test_result(1));
	//tv(2) = 0 * tr(0) + 3 * tr(2) + 1.5 * tr(3) = (3 - 6) / 1.5 = -2
	EXPECT_DOUBLE_EQ(-2.0, test_result(2));
	//tv(3) = 1 * tr(0) + 0 * tr(1) + 0 * tr(2) + 1 * tr(3) = (4 - 0.5) = 3.5
	EXPECT_DOUBLE_EQ(3.5, test_result(3));

	test_result.noalias() = rvector_type::Zero(4);
	vector_times_LU_inverse(test_vector, LU_decomp, test_result);
	EXPECT_DOUBLE_EQ(0.5, test_result(0));
	EXPECT_DOUBLE_EQ(2.0, test_result(1));
	EXPECT_DOUBLE_EQ(-2.0, test_result(2));
	EXPECT_DOUBLE_EQ(3.5, test_result(3));

	vector_type test_vector_b((value_type*)_alloca(sizeof(value_type) * 4), 4);
	vector_type test_result_b((value_type*)_alloca(sizeof(value_type) * 4), 4);
	test_vector_b << 2, 4, 6, 8;
	test_result_b.noalias() = vector_type::Zero(4);

	ut_inverse_times_vector(test_matrix, test_vector_b, test_result_b);

	//tv(3) = 1 * tr(3) = 8.0
	EXPECT_DOUBLE_EQ(8.0, test_result_b(3));
	//tv(2) = 1.5 * tr(2) + 0 * tr(3) = 6 / 1.5 = 4
	EXPECT_DOUBLE_EQ(4.0, test_result_b(2));
	//tv(1) = 1 * tr(1) + 3 * tr(2) + 0 * tr(3) = (4 - 12) = -8
	EXPECT_DOUBLE_EQ(-8.0, test_result_b(1));
	//tv(0) = 2 * tr(0) + 0 * tr(1) + 0 * tr(2) + 1 * tr(3) = (2 - 8) / 2 = -3.0
	EXPECT_DOUBLE_EQ(-3.0, test_result_b(0));

	test_result_b.noalias() = vector_type::Zero(4);
	LU_inverse_times_vector(LU_decomp, test_vector_b, test_result_b);
	EXPECT_DOUBLE_EQ(8.0, test_result_b(3));
	EXPECT_DOUBLE_EQ(4.0, test_result_b(2));
	EXPECT_DOUBLE_EQ(-8.0, test_result_b(1));
	EXPECT_DOUBLE_EQ(-3.0, test_result_b(0));



	rvector_type test_gradient((value_type*)_alloca(sizeof(value_type) * 6), 6);
	rvector_type test_reduced_n_gradient((value_type*)_alloca(sizeof(value_type) * 2), 2);
	test_gradient << 1, 2, 3, 4, 5, 6;

	test_reduced_n_gradient.noalias() = rvector_type::Zero(2);
	EXPECT_DOUBLE_EQ(0.0, test_reduced_n_gradient(0));
	EXPECT_DOUBLE_EQ(0.0, test_reduced_n_gradient(1));

	caclulate_reduced_n_gradient_ut(4, 2, test_matrix, test_gradient, test_reduced_n_gradient);
	EXPECT_DOUBLE_EQ(-35.0, test_reduced_n_gradient(0));
	EXPECT_DOUBLE_EQ(-14.0, test_reduced_n_gradient(1));

	test_reduced_n_gradient.noalias() = rvector_type::Zero(2);
	caclulate_reduced_n_gradient_LU(4, 2, test_matrix, LU_decomp, test_gradient, test_reduced_n_gradient);
	EXPECT_DOUBLE_EQ(-35.0, test_reduced_n_gradient(0));
	EXPECT_DOUBLE_EQ(-14.0, test_reduced_n_gradient(1));

	test_reduced_n_gradient *= -1.0;

	vector_type test_direction((value_type*)_alloca(sizeof(value_type) * 6), 6);
	std::vector<var_mapping> variable_mapping = {var_mapping{1.0, 1}, var_mapping{2.0, 2}, var_mapping{0.0, 5}, var_mapping{10.0, 3}, var_mapping{1.5, 4}, var_mapping{2.5, 0}};

	calcuate_n_direction_mokhtar(4, 2, variable_mapping, test_reduced_n_gradient, test_direction);
	EXPECT_DOUBLE_EQ(-35.0 * 1.5, test_direction(4));
	EXPECT_DOUBLE_EQ(0.0, test_direction(5));

	calcuate_n_direction_steepest(4, 2, variable_mapping, test_reduced_n_gradient, test_direction);
	EXPECT_DOUBLE_EQ(-35.0, test_direction(4));
	EXPECT_DOUBLE_EQ(0.0, test_direction(5));

	calcuate_b_direction_ut(4, 2, test_matrix, test_direction);
	EXPECT_DOUBLE_EQ(0.0, test_direction(0));
	EXPECT_DOUBLE_EQ(-350.0, test_direction(1));
	EXPECT_DOUBLE_EQ(350.0 / 1.5, test_direction(2));
	EXPECT_DOUBLE_EQ(350.0, test_direction(3));

	test_direction.head(4) = vector_type::Zero(4);
	calcuate_b_direction_LU(4, 2, test_matrix, LU_decomp, test_direction);
	EXPECT_DOUBLE_EQ(0.0, test_direction(0));
	EXPECT_DOUBLE_EQ(-350.0, test_direction(1));
	EXPECT_DOUBLE_EQ(350.0 / 1.5, test_direction(2));
	EXPECT_DOUBLE_EQ(350.0, test_direction(3));

	const auto ml = max_lambda(variable_mapping, test_direction);
	EXPECT_DOUBLE_EQ(1.0 / 350.0, ml.first);
	EXPECT_DOUBLE_EQ(0, ml.second);
}

TEST(nlp, line_search) {
	linear_test_function a([](value_type x) { return -x / (x*x + 2.0f); }, [](value_type x) { return (2.0f*x*x / pow(2.0f+x*x, 2.0f)) - (1.0f/(2.0f+x*x)); });
	linear_test_function b([](value_type x) { return pow(x + 0.004f, 5.0f) - 2.0f * pow(x + 0.004f, 4.0f); },
		[](value_type x) { return value_type(log(exp(5.0f*pow(x + 0.004f, 4.0f))/exp(8.0f*pow(x + 0.004f, 3.0f))));});
	linear_test_function c([](value_type x) { return 0.5f*pow(x, 2.0f) - 1.01f * pow(x, 4.0f) + pow(x, 6.0f)/6.0f + 1.0f / (1.0f + x); }, [](value_type x) { return x - 4.04f*pow(x,3.0f) + pow(x, 5.0f)- 1.0f / pow(1.0f + x, 2.0f); });
	linear_test_function d([](value_type x) { return sin(x)+x*x-1.5f*x; }, [](value_type x) { return cos(x)+2.0f*x-1.5f; });
	linear_test_function e([](value_type x) { return - pow(x+1.0f,2.0f); }, [](value_type x) { return -2.0f * (x + 1.0f); });

	auto result = backtrack_linear_steepest_descent(c, 2.0f);
	ASSERT_NEAR(1.94727, result.first, 0.001);
	result = hager_zhang_linear_steepest_descent(c, 2.0f);
	ASSERT_NEAR(1.94727, result.first, 0.001);
	result = interpolation_linear_steepest_descent(c, 2.0f);
	ASSERT_NEAR(1.94727, result.first, 0.001);
	result = derivative_interpolation_linear_steepest_descent(c, 2.0f);
	ASSERT_NEAR(1.94727, result.first, 0.001);

	result = backtrack_linear_steepest_descent(a, 2.0f);
	ASSERT_NEAR(sqrt(2.0f), result.first, 0.001);
	result = hager_zhang_linear_steepest_descent(a, 2.0f);
	ASSERT_NEAR(sqrt(2.0f), result.first, 0.001);
	result = interpolation_linear_steepest_descent(a, 2.0f);
	ASSERT_NEAR(sqrt(2.0f), result.first, 0.001);
	result = derivative_interpolation_linear_steepest_descent(a, 2.0f); // hits safety limit w/ float
	ASSERT_NEAR(sqrt(2.0f), result.first, 0.001);

	result = derivative_interpolation_linear_steepest_descent(d, 2.0f);
	ASSERT_NEAR(0.267826, result.first, 0.001);
	result = backtrack_linear_steepest_descent(d, 2.0f);
	ASSERT_NEAR(0.267826, result.first, 0.001);
	result = interpolation_linear_steepest_descent(d, 2.0f);
	ASSERT_NEAR(0.267826, result.first, 0.001);
	result = hager_zhang_linear_steepest_descent(d, 2.0f);
	ASSERT_NEAR(0.267826, result.first, 0.001);

	result = derivative_interpolation_linear_steepest_descent(e, 2.0f);
	ASSERT_NEAR(2.0, result.first, 0.001);
	result = backtrack_linear_steepest_descent(e, 2.0f);
	ASSERT_NEAR(2.0, result.first, 0.001);
	result = interpolation_linear_steepest_descent(e, 2.0f);
	ASSERT_NEAR(2.0, result.first, 0.001);
	result = hager_zhang_linear_steepest_descent(e, 2.0f);
	ASSERT_NEAR(2.0, result.first, 0.001);

	result = derivative_interpolation_linear_steepest_descent(b, 2.0f);
	ASSERT_NEAR(1.596, result.first, 0.001);
	result = hager_zhang_linear_steepest_descent(b, 2.0f);
	ASSERT_NEAR(1.596, result.first, 0.001);
	result = backtrack_linear_steepest_descent(b, 2.0f);
	ASSERT_NEAR(1.596, result.first, 0.001);
	result = interpolation_linear_steepest_descent(b, 2.0f);
	ASSERT_NEAR(1.596, result.first, 0.001);

	result = derivitive_minimization_steepest_descent(a, 2.0f);
	ASSERT_NEAR(sqrt(2.0f), result.first, 0.001);
	result = derivitive_minimization_steepest_descent(b, 2.0f);
	ASSERT_NEAR(1.596, result.first, 0.001);
	result = derivitive_minimization_steepest_descent(c, 2.0f);
	ASSERT_NEAR(1.94727, result.first, 0.001);
	result = derivitive_minimization_steepest_descent(d, 2.0f);
	ASSERT_NEAR(0.267826, result.first, 0.001);
	result = derivitive_minimization_steepest_descent(e, 2.0f);
	ASSERT_NEAR(2.0, result.first, 0.001);
}

TEST(nlp, sum_of_functions_class) {
	sum_of_functions tf;
	tf.add_function([](const value_type* at) { return at[0] * at[1]; },
		[](const value_type* at, const value_type* direction) {return direction[0] * at[1] + direction[1] * at[0];}, 
		{0, 2});
	tf.add_function([](const value_type* at) { return at[0] * at[0]; },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * at[0]; },
		{1});
	tf.add_function([](const value_type* at) { return at[0] + at[1]; },
		[](const value_type* at, const value_type* direction) {return direction[0] + direction[1]; },
		{1, 3});
	// f(x) = x0*x2 + x1^2 + x1 + x3;
	// G(f) = (x2, 2*x1 + 1, x0, 1)

	vector_type td((value_type*)_alloca(sizeof(value_type) * 4), 4);
	rvector_type tg((value_type*)_alloca(sizeof(value_type) * 4), 4);

	std::vector<var_mapping> v = {
		var_mapping{random_store::get_double() * 10.0, 3},
		var_mapping{random_store::get_double() * 10.0, 2},
		var_mapping{random_store::get_double() * 10.0, 0},
		var_mapping{random_store::get_double() * 10.0, 1}};

	EXPECT_FLOAT_EQ(v[0].current_value*v[2].current_value+v[1].current_value*v[1].current_value + v[1].current_value + v[3].current_value, tf.evaluate_at(v));
	
	tf.gradient_at(v, tg);
	EXPECT_FLOAT_EQ(v[2].current_value, tg(3));
	EXPECT_FLOAT_EQ(2.0 * v[1].current_value + 1.0, tg(2));
	EXPECT_FLOAT_EQ(v[0].current_value, tg(0));
	EXPECT_FLOAT_EQ(1.0, tg(1));

	td(0) = random_store::get_double() * 10.0;
	td(1) = random_store::get_double() * 10.0;
	td(2) = random_store::get_double() * 10.0;
	td(3) = random_store::get_double() * 10.0;

	value_type λ = random_store::get_double();

	EXPECT_FLOAT_EQ((v[0].current_value + td(3) * λ)*(v[2].current_value + td(0)*λ) + (v[1].current_value + td(2)*λ)*(v[1].current_value + td(2)*λ) + (v[1].current_value + td(2)*λ) + (v[3].current_value + td(1)*λ), tf.evaluate_at(v, λ,td));
	EXPECT_FLOAT_EQ(
		(v[2].current_value + λ * td(0)) * td(3) + 
		(2.0 * (v[1].current_value + td(2) * λ) + 1.0) * td(2) + 
		(v[0].current_value + td(3)*λ) * td(0) + 
		1.0 * td(1),
		tf.gradient_at(v, λ, td));
	const auto pr = tf.evaluate_at_with_derivative(v, λ, td);
	EXPECT_FLOAT_EQ(tf.evaluate_at(v, λ, td), pr.first);
	EXPECT_FLOAT_EQ(tf.gradient_at(v, λ, td), pr.second);
}


TEST(nlp, steepest_descent) {
	sum_of_functions tf;
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{0});
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{1});
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{2});
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{3});

	//_control87(_EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM);

	std::vector<var_mapping> v = {
		var_mapping{2.0, 0},
		var_mapping{0.0, 2},
		var_mapping{8.0, 1},
		var_mapping{0.0, 3}};

	matrix_type coeff((value_type*)_alloca(sizeof(value_type) * 8), 2, 4);
	coeff <<	1, 0, 1, 0,
				0, 1, 0, 2;
	flat_multimap<unsigned short, unsigned short> ranks;
	ranks.insert(std::pair<unsigned short, unsigned short>(0, 0));
	ranks.insert(std::pair<unsigned short, unsigned short>(0, 1));
	ranks.insert(std::pair<unsigned short, unsigned short>(1, 2));
	ranks.insert(std::pair<unsigned short, unsigned short>(1, 3));


	sof_hz_steepest_descent(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_m_hz_steepest_descent(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);


	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_dm_steepest_descent(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value); 
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_m_dm_steepest_descent(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_bt_steepest_descent(tf, v, coeff, ranks); // failed -- too many iterations

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_m_bt_steepest_descent(tf, v, coeff, ranks); // failed -- too many iterations

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_int_steepest_descent(tf, v, coeff, ranks);  // failed -- too many iterations

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_m_int_steepest_descent(tf, v, coeff, ranks); // failed -- too many iterations

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_dint_steepest_descent(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);

	v[0].current_value = 2.0;
	v[1].current_value = 0.0;
	v[2].current_value = 8.0;
	v[3].current_value = 0.0;

	sof_m_dint_steepest_descent(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);
	
}


TEST(nlp, steepest_descent_b) {
	sum_of_functions tf;
	tf.add_function([](const value_type* at) {
			return 2.0 * at[0] * at[0] + 2.0 * at[1] * at[1] - 2.0 * at[0] * at[1] - 4.0 * at[0] - 6.0 * at[1]; }, 
		[](const value_type* at, const value_type* direction) {
			return direction[0] * (4.0 * at[0] - 2.0 * at[1] - 4.0) + direction[1] * (4.0 * at[1] - 2.0 * at[0] - 6.0); },
		{0, 1, 2, 3});

	//_control87(_EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM);

	std::vector<var_mapping> v = {
		var_mapping{0.0, 0},
		var_mapping{0.0, 1},
		var_mapping{2.0, 2},
		var_mapping{5.0, 3}};

	matrix_type coeff((value_type*)_alloca(sizeof(value_type) * 8), 2, 4);
	coeff << 1, 1, 1, 0,
			 1, 5, 0, 1;

	
	flat_multimap<unsigned short, unsigned short> ranks;
	setup_rank_map(coeff, v, ranks);

	sof_hz_steepest_descent(tf, v, coeff, ranks);

	ASSERT_NEAR(35.0/31.0, v[0].current_value, 0.0001);
	ASSERT_NEAR(24.0/31.0, v[1].current_value, 0.0001);
	ASSERT_NEAR(3.0/31.0, v[2].current_value, 0.0001);
	ASSERT_NEAR(0.0, v[3].current_value, 0.0001);

	const auto eval = tf.evaluate_at(v);

	v[0].current_value = 0.0;
	v[1].current_value = 0.0;
	v[2].current_value = 2.0;
	v[3].current_value = 5.0;

	sof_m_hz_steepest_descent(tf, v, coeff, ranks);

	ASSERT_NEAR(35.0 / 31.0, v[0].current_value, 0.0001);
	ASSERT_NEAR(24.0 / 31.0, v[1].current_value, 0.0001);
	ASSERT_NEAR(3.0 / 31.0, v[2].current_value, 0.0001);
	ASSERT_NEAR(0.0, v[3].current_value, 0.0001);

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_dm_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_m_dm_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_bt_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_m_bt_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_int_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_m_int_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_dint_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));

	v[0].current_value = 0.0; v[1].current_value = 0.0; v[2].current_value = 2.0; v[3].current_value = 5.0;
	sof_m_dint_steepest_descent(tf, v, coeff, ranks);
	ASSERT_GE(eval + 0.0001, tf.evaluate_at(v));
}

TEST(nlp, conjugate_gradient) {
	sum_of_functions tf;
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{0});
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{1});
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{2});
	tf.add_function([](const value_type* at) { return (at[0] - value_type(2.0)) * (at[0] - value_type(2.0)); },
		[](const value_type* at, const value_type* direction) {return direction[0] * value_type(2.0) * (at[0] - value_type(2.0)); },
		{3});

	std::vector<var_mapping> v = {
		var_mapping{2.0, 0},
		var_mapping{0.0, 2},
		var_mapping{8.0, 1},
		var_mapping{0.0, 3}};

	matrix_type coeff((value_type*)_alloca(sizeof(value_type) * 8), 2, 4);
	coeff << 1, 0, 1, 0,
			 0, 1, 0, 2;
	flat_multimap<unsigned short, unsigned short> ranks;
	setup_rank_map(coeff, v, ranks);

	sof_hz_conjugate_gradient(tf, v, coeff, ranks);

	EXPECT_FLOAT_EQ(1.0, v[0].current_value);
	EXPECT_FLOAT_EQ(1.0, v[1].current_value);
	EXPECT_FLOAT_EQ(2.4, v[2].current_value);
	EXPECT_FLOAT_EQ(2.8, v[3].current_value);
}