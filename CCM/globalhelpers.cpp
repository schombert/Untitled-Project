#include "globalhelpers.h"

using namespace std;

namespace global {
	reader_writer_lock ogll;
	reader_writer_lock index_lock;
}

#ifdef CCL_NO_EXECPTIONS
namespace boost {
	void throw_exception(stdext::exception const &) {
		abort();
	}
}
#endif

using namespace boost::gregorian;
const date date_offset(1400, Jan, 1);

#ifdef USE_DECLSPEC_THREAD
__declspec(thread) class _anon_g_wrap {
public:
	bool inited = false;
	random_store::generator_type g;
} _global_gen;
#endif


tls_handle_wrapper random_store::dwTlsIndex;


random_store::generator_type& random_store::get_generator() noexcept {
#ifdef USE_DECLSPEC_THREAD
	if (_global_gen.inited) {
		return _global_gen.g;
	} else {
		_global_gen.g.seed(std::random_device()());
		_global_gen.inited = true;
		return _global_gen.g;
	}
#else
	random_store::generator_type* g;
	if ((g = (generator_type*)TlsGetValue(dwTlsIndex)) == nullptr) {
		//int v = InterlockedIncrement(&cntr);
		//OutputDebugStringA((std::to_string(v) + " threads\r\n").c_str());
		TlsSetValue(dwTlsIndex, g = new generator_type(std::random_device()()));
	}
	return *g;
#endif
}


random_store global_store;


template<typename T>
inline bool veccmp(IN(std::vector<T>) v1, IN(std::vector<T>) v2) {
	if (v1.size() != v2.size())
		return false;
	for (unsigned int i = 0; i < v1.size(); ++i)
		if (!(v1[i] == v2[i]))
			return false;
	return true;
}


void RawGraphicsSetUp(IN(sf::IntRect) viewport, IN(sf::View) view, int sizey) {
	int top = sizey - (viewport.top + viewport.height);
	glViewport(viewport.left, top, viewport.width, viewport.height);

	// Set the projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(view.getTransform().getMatrix());

	// Go back to model-view mode
	glMatrixMode(GL_MODELVIEW);

	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
}

void RawGraphicsFinish() {
	glPopClientAttrib();
}


void MergeRects(INOUT(sf::IntRect) a, IN(sf::IntRect) b) {
	if (a.left < b.left) {
		a.width = max(a.left + a.width, b.left + b.width) - a.left;
	}
	else {
		a.width = max(a.left + a.width, b.left + b.width) - b.left;
		a.left = b.left;
	}

	if (a.top < b.top) {
		a.height = max(a.top + a.height, b.top + b.height) - a.top;
	}
	else {
		a.height = max(a.top + a.height, b.top + b.height) - b.top;
		a.top = b.top;
	}
}

sf::Color get_color_from_raw(IN_P(unsigned char) imgdata, int x, int y, int w, int ch) noexcept {
	return sf::Color(imgdata[(w*y + x)*ch], imgdata[(w*y + x)*ch + 1], imgdata[(w*y + x)*ch + 2]);
}

void load_soil_image(INOUT(sf::Image) img, IN_P(char) file) {
	int tw; int th; int ch;
	unsigned char* imgdata = SOIL_load_image(file, &tw, &th, &ch, SOIL_LOAD_AUTO);
	if (ch == 4) {
		img.create(tw, th, imgdata);
	} else {
		unsigned char* nimgdata = new unsigned char[tw*th * 4];
		for (int i = 0; i < tw*th; i++) {
			nimgdata[i * 4 + 0] = imgdata[i * ch + 0];
			nimgdata[i * 4 + 1] = imgdata[i * ch + 1];
			nimgdata[i * 4 + 2] = imgdata[i * ch + 2];
			nimgdata[i * 4 + 3] = 255;
		}
		img.create(tw, th, nimgdata);
		delete[] nimgdata;
	}
	SOIL_free_image_data(imgdata);
}

void LoadDDS(INOUT(sf::Texture) tex, IN_P(char) file) {
	int tw; int th; int ch;
	unsigned char* __restrict imgdata = SOIL_load_image(file, &tw, &th, &ch, 4);
	tex.create(tw, th);
	tex.update(imgdata);
	SOIL_free_image_data(imgdata);
}


void concurrent_aligned_free(IN_P(void) ptr) {
	IN_P(unsigned char) ptrcst = static_cast<unsigned char*>(ptr);
	concurrency::Free(ptrcst - (*(ptrcst - 1)));
}

size_t get_from_distribution_nt(const double * ar, const size_t n, double total) {
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

size_t get_from_distribution_n(const double * ar, const size_t n) {
	double total = std::max(ar[0], 0.0);
	for (size_t i = 1; i < n; ++i) {
		total += std::max(ar[i], 0.0);
	}
	return get_from_distribution_nt(ar, n, total);
}

tstring window_text(HWND hwnd) {
	int tlen = GetWindowTextLength(hwnd) + 1;
	TCHAR* text = new TCHAR[tlen];
	GetWindowText(hwnd, text, tlen);
	tstring tmp = text;
	delete[] text;
	return tmp;
}

std::string wstr_to_str(IN(std::wstring) in) {
	const wchar_t* const __restrict str = in.c_str();
	const int slen = static_cast<int>(in.length());
	const int len = WideCharToMultiByte(CP_UTF8, 0, str, slen, nullptr, 0, nullptr, nullptr);
	char* const __restrict buffer = (char*)_malloca(len);
	WideCharToMultiByte(CP_UTF8, 0, str, slen, buffer, len, nullptr, nullptr);
	std::string result(buffer, len);
	_freea((void*)buffer);
	return result;
}

std::wstring str_to_wstr(IN(std::string) in) {
	const char* const __restrict str = in.c_str();
	const int slen = static_cast<int>(in.length());
	const int len = MultiByteToWideChar(CP_UTF8, 0, str, slen, nullptr, 0);
	wchar_t* const __restrict buffer = (wchar_t*)_malloca(len * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str, slen, buffer, len);
	std::wstring result(buffer, len);
	_freea((void*)buffer);
	return result;
}