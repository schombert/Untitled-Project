#include "globalhelpers.h"
#include "mapdisplay.h"
#include <Windows.h>

double test_trig_fp() {
	__int64 itimestamp = 0;
	__int64 endstamp = 0;

	OutputDebugStringA("---distance speed---");
	OutputDebugStringA("\r\n");

	double distanceres = 0;
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);
		
		const glm::vec2 origin(0, 0);
		for (int i = 0; i != 7000; i += 10) {
			for (int j = 0; j != 4000; ++j) {
				distanceres += distance_from_texture(origin, glm::vec2(i, j));
			}
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("floating point: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);
		half_fp hp_distanceres = half_fp::from_int(0);
		const glm::tvec2<half_fp> origin(half_fp::from_int(0), half_fp::from_int(0));
		for (half_fp i = half_fp::from_int(0); i != half_fp::from_int(7000); i += half_fp::from_int(10)) {
			for (half_fp j = half_fp::from_int(0); j != half_fp::from_int(4000); ++j) {
				hp_distanceres += hp_distance_from_texture(origin, glm::tvec2<half_fp>(i, j));
			}
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("fixed point   : ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");


	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);
		for (int i = 0; i != 7000; i += 10) {
			for (int j = 0; j != 4000; ++j) {
				distanceres -= (7000 - i) * (7000 - i) + (4000 - j) * (4000 - j);
			}
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("taxi cab      : ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");
	return distanceres;
}


double test_simple_math() {
	__int64 itimestamp = 0;
	__int64 endstamp = 0;

	OutputDebugStringA("-----simple math----");
	OutputDebugStringA("\r\n");

	double distanceres = 0;
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);

		const glm::vec2 origin(0, 0);
		for (int i = 0; i != 7000; i += 10) {
			for (int j = 1; j != 4000; ++j) {
				distanceres += static_cast<double>(i) / static_cast<double>(j);
			}
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("floating point: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);
		half_fp hp_distanceres = half_fp::from_int(0);
		const glm::tvec2<half_fp> origin(half_fp::from_int(0), half_fp::from_int(0));
		for (half_fp i = half_fp::from_int(0); i != half_fp::from_int(7000); i += half_fp::from_int(10)) {
			for (half_fp j = half_fp::from_int(1); j != half_fp::from_int(4000); ++j) {
				half_fp a(i);
				a /= j;
				hp_distanceres += a;
			}
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("fixed point   : ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	__int64 idis = 0;
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);
		
		const glm::vec2 origin(0, 0);
		for (int i = 0; i != 7000; i += 10) {
			for (int j = 1; j != 4000; ++j) {
				idis += i / j;
			}
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("integers      : ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	return distanceres + idis;
}


double test_sqrt() {
	__int64 itimestamp = 0;
	__int64 endstamp = 0;

	OutputDebugStringA("---test error---");
	OutputDebugStringA("\r\n");

	double distanceres = 0;
	const double increment = 1.0 / static_cast<double>(1i64 << 16);
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);

		
		for (double i = 0.0; i != 1.0; i += increment) {
			distanceres += sqrt(1 - i*i) * sqrt(1 - i*i);
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("by sqrtx2: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);


		for (double i = 0.0; i != 1.0; i += increment) {
			distanceres += exp2(log2(1 - i*i) * log2(1 - i*i));
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("by lgth: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	return distanceres;
}



double test_addition() {
	__int64 itimestamp = 0;
	__int64 endstamp = 0;

	OutputDebugStringA("---test addition---");
	OutputDebugStringA("\r\n");

	double distanceres = 0;
	const double increment = 1.0 / static_cast<double>(1i64 << 16);
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);


		for (double i = 0.0; i != 1.0; i += increment) {
			distanceres += acos(i);
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("by acos: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	double distanceres2 = 1;
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);

		for (double i = 0.0; i != 1.0; i += increment) {
			distanceres2 = distanceres2*i - (sqrt(1 - i*i) * sqrt(1 - distanceres2*distanceres2));
		}

		distanceres2 = acos(distanceres2);
		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("by sqrt: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	return distanceres + distanceres2;
}

double tosecond(IN(std::pair<int, double>) p) {
	return p.second;
}

double test_tovector() {
	flat_multimap<int, double> map;
	map.emplace(0, 0.5);
	map.emplace(1, 0.5);
	map.emplace(1, 1.5);
	map.emplace(1, 2.5);
	map.emplace(1, 3.5);
	map.emplace(1, 4.5);
	map.emplace(2, 0.5);

	__int64 itimestamp = 0;
	__int64 endstamp = 0;

	OutputDebugStringA("---test to vector---\r\n");

	double result = 0;

	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);


		for (size_t i = 0; i != 10000; ++i) {
			std::vector<double> vec;

			auto pr = map.equal_range(1);
			vec.reserve(vec.size() + std::distance(pr.first, pr.second));
			for (; pr.first != pr.second; ++pr.first) {
				vec.push_back(pr.first->second);
			}

			result += vec[2];
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("by push: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	{
		QueryPerformanceCounter((LARGE_INTEGER*)&itimestamp);


		for (size_t i = 0; i != 10000; ++i) {
			std::vector<double> vec;

			const auto pr = map.equal_range(1);
			vec.insert(vec.cend(),
				generating_iterator<double, std::pair<int, double>, decltype(&tosecond)>(&tosecond, pr.first),
				generating_iterator<double, std::pair<int, double>, decltype(&tosecond)>(&tosecond, pr.second));

			result += vec[2];
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&endstamp);
	}

	OutputDebugStringA("by insert: ");
	OutputDebugStringA(std::to_string(endstamp - itimestamp).c_str());
	OutputDebugStringA("\r\n");

	return result;
}

int CALLBACK ptest_WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	int nCmdShow
	) {

	test_tovector();

	//test_sqrt();
	//test_addition();
	//test_trig_fp();
	//test_simple_math();
	return 0;
}