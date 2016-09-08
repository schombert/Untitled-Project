// textviewer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "textviewer.h"
#include <string>
#include <vector>
#include "sqlite3.h"
#include "Shobjidl.h"
#include <array>

#define MAX_LOADSTRING 100

class stmtwrapper {
public:
	sqlite3_stmt* stmt;
	stmtwrapper() : stmt(nullptr) {};
	stmtwrapper(sqlite3* const db, const char* sql) : stmt(nullptr) {

		int res = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
		if (res != SQLITE_OK) {
			OutputDebugStringA((std::string(sql) + " SQL PREP ERR: ").c_str());
			OutputDebugStringA(sqlite3_errstr(res));
		}
	};
	stmtwrapper(const stmtwrapper&) = delete;
	stmtwrapper(stmtwrapper &&in) {
		stmt = in.stmt;
		in.stmt = nullptr;
	}
	~stmtwrapper() {
		if (stmt)
			sqlite3_finalize(stmt);
	};
	bool step() {
		int res = sqlite3_step(stmt);
		if (res == SQLITE_ROW)
			return true;

		if (res == SQLITE_OK || res == SQLITE_DONE)
			return false;

		OutputDebugStringA((std::string(sqlite3_sql(stmt)) + " SQL EXEC ERR: ").c_str());
		OutputDebugStringA(sqlite3_errstr(res));

		return false;

	}
	operator sqlite3_stmt*() const {
		return stmt;
	};
	stmtwrapper copy(sqlite3* const db) const {
		return stmtwrapper(db, sqlite3_sql(stmt));
	}
};

struct core_temp_shared_data
{
	unsigned int	uiLoad[256];
	unsigned int	uiTjMax[128];
	unsigned int	uiCoreCnt;
	unsigned int	uiCPUCnt;
	float			fTemp[256];
	float			fVID;
	float			fCPUSpeed;
	float			fFSBSpeed;
	float			fMultipier;
	char			sCPUName[100];
	unsigned char	ucFahrenheit;
	unsigned char	ucDeltaToTjMax;
};

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.


	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);


	const auto sfhandle = OpenFileMapping(FILE_MAP_READ, FALSE, L"CoreTempMappingObjectEx");
	if (sfhandle != NULL) {
		const core_temp_shared_data* dbuf = static_cast<core_temp_shared_data*>(MapViewOfFile(sfhandle, FILE_MAP_READ, 0, 0, 0));
		if (dbuf != NULL) {
			MEMORY_BASIC_INFORMATION info;
			size_t bytes = VirtualQuery(dbuf, &info, sizeof(info));

			std::string infostr = std::string("size of buffer: ") + std::to_string(bytes) + "\r\n";
			OutputDebugStringA(infostr.c_str());

			std::string cpustr = std::string("cpus: ") + std::to_string(dbuf->uiCPUCnt) + std::string(", cores: ") + std::to_string(dbuf->uiCoreCnt) + ", cpu0 load: " + std::to_string(dbuf->uiLoad[0]) + \
				", cpu0 tjmax: " + std::to_string(dbuf->uiTjMax[0]) + ", cpu0 fTemp: " + std::to_string(dbuf->fTemp[0]) + "\r\n";
			OutputDebugStringA(cpustr.c_str());

			// Sleep(5000);

			cpustr = std::string("cpus: ") + std::to_string(dbuf->uiCPUCnt) + std::string(", cores: ") + std::to_string(dbuf->uiCoreCnt) + ", cpu0 load: " + std::to_string(dbuf->uiLoad[0]) + \
				", cpu0 tjmax: " + std::to_string(dbuf->uiTjMax[0]) + ", cpu0 fTemp: " + std::to_string(dbuf->fTemp[0]) + "\r\n";
			OutputDebugStringA(cpustr.c_str());

			UnmapViewOfFile(dbuf);
		} else {
			OutputDebugStringA("could not map file\r\n");
		}
		
		CloseHandle(sfhandle);
	} else {
		OutputDebugStringA("could not open mapped file\r\n");
	}
	

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TEXTVIEWER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
    }

	CoUninitialize();

    return (int) msg.wParam;
}

#ifndef UNICODE  
typedef std::string tstring;
#else
typedef std::wstring tstring;
#endif


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

tstring window_text(HWND hwnd) {
	int tlen = GetWindowTextLength(hwnd) + 1;
	TCHAR* text = new TCHAR[tlen];
	GetWindowText(hwnd, text, tlen);
	tstring tmp(text);
	delete[] text;
	return tmp;
}

std::string ascii_window_text(HWND hwnd) {
	int tlen = GetWindowTextLengthA(hwnd) + 1;
	char* text = new char[tlen];
	GetWindowTextA(hwnd, text, tlen);
	std::string tmp(text);
	delete[] text;
	return tmp;
}


std::string wstr_to_str(const std::wstring &in) {
	char buffer[256];
	const wchar_t* const __restrict str = in.c_str();
	const int slen = static_cast<int>(in.length());
	int len = WideCharToMultiByte(CP_UTF8, 0, str, slen, buffer, 256, nullptr, nullptr);

	if (len != 0)
		return std::string(buffer, len);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		len = WideCharToMultiByte(CP_UTF8, 0, str, slen, nullptr, 0, nullptr, nullptr);
		std::vector<char> dbuffer(len);
		WideCharToMultiByte(CP_UTF8, 0, str, slen, dbuffer.data(), len, nullptr, nullptr);
		return std::string(dbuffer.data(), len);
	}
	return std::string();
}

std::wstring str_to_wstr(const std::string &in) {
	wchar_t buffer[256];
	const char* const __restrict str = in.c_str();
	const int slen = static_cast<int>(in.length());

	int len = MultiByteToWideChar(CP_UTF8, 0, str, slen, buffer, 256);

	if (len != 0)
		return std::wstring(buffer, len);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		len = MultiByteToWideChar(CP_UTF8, 0, str, slen, nullptr, 0);

		std::vector<wchar_t> dbuffer(len);
		MultiByteToWideChar(CP_UTF8, 0, str, slen, dbuffer.data(), len);
		return std::wstring(dbuffer.data(), len);
	}
	return std::wstring();
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEXTVIEWER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

constexpr int num_adm_classes = 5;

class adm_dispatch_it_class {
public:
	template <int cnt>
	static int nth_element() {
		return cnt+1;
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
class generate_array_it<gen_class , sz, -1, Types ...> {
public:
	static std::array<decltype(gen_class::nth_element<0>()), sz::value> doit(Types&&... t) {
		return{std::forward<Types>(t)...};
	}
};


template <typename gen_class, int sz >
std::array<decltype(gen_class::nth_element<0>()), sz> generate_array() {
	return generate_array_it<gen_class, std::integral_constant<int, sz>, sz - 1>::doit();
}

static const auto g_array = generate_array<adm_dispatch_it_class, num_adm_classes>();

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
#define ID_EDITCHILD 100
#define ID_EDITNUM 101
#define ID_DISPLAYTEXY 102

std::string string_source;

template <int n>
struct mask {
	const static __int64 value = (1I64 << (n - 1)) | mask<n - 1>::value;
};

template<>
struct mask<0> {
	const static __int64 value = 0;
};

template<typename T>
class string_ref_t {
protected:
	T _get(const T* const buffer) const noexcept {
		if (offset < buffer->length())
			return buffer->substr(offset, length);
		else
			return T("out of range: ") + std::to_string(offset);
	}
	unsigned int offset;
	unsigned int length;
public:
	string_ref_t(const T& txt, const T* const buffer) {
		length = static_cast<unsigned int>(txt.size());
		size_t off = buffer->find(txt);
		if (off == T::npos) {
			offset = static_cast<unsigned int>(buffer->size());
			*buffer += txt;
		} else {
			offset = static_cast<unsigned int>(off);
		}
	}
	string_ref_t() : offset(0), length(0) {}

	__int64 to_int() const noexcept {
		return ((__int64)length << 32) | (__int64)offset;
	}
	void load(__int64 data) noexcept {
		offset = static_cast<unsigned int>(data & mask<32>::value);
		length = static_cast<unsigned int>(data >> 32);
	}
};


void load_string_source(sqlite3* db) {
	stmtwrapper getplayer(db, "SELECT mastertext FROM globals");
	getplayer.step();
	string_source.assign((char*)sqlite3_column_blob(getplayer, 0), sqlite3_column_bytes(getplayer, 0));
}

template<std::string* buffer>
class string_ref : public string_ref_t<std::string> {
public:
	string_ref(const std::string &txt) : string_ref_t<std::string>(txt, buffer) {}
	string_ref() {}

	std::string get() const noexcept { return _get(buffer); }
};

using sref = string_ref<&string_source>;
#define GWL_HINSTANCE (-6)



HRESULT open_db(sqlite3* &db) {
	// CoCreate the File Open Dialog object.
	IFileOpenDialog *pFileOpen = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr)) {
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFilePath;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				// Display the file name to the user.
				if (SUCCEEDED(hr)) {

					sqlite3_open16(pszFilePath, &db);

					CoTaskMemFree(pszFilePath);
				} else {
					OutputDebugStringA("COM ERR: pItem->GetDisplayName\r\n");
				}

				pItem->Release();
			} else {
				OutputDebugStringA("COM ERR: pFileOpen->GetResult\r\n");
			}
		} else {
			OutputDebugStringA("COM ERR: pFileOpen->Show\r\n");
		}
		pFileOpen->Release();
	} else {
		OutputDebugStringA("COM ERR: CoCreateInstance\r\n");
		if (hr == E_NOINTERFACE) {
			OutputDebugStringA("COM ERR: E_NOINTERFACE\r\n");
		} else if (hr == REGDB_E_CLASSNOTREG) {
			OutputDebugStringA("COM ERR: REGDB_E_CLASSNOTREG\r\n");
		} else if (hr == CLASS_E_NOAGGREGATION) {
			OutputDebugStringA("COM ERR: REGDB_E_CLASSNOTREG\r\n");
		} else if (hr == E_POINTER) {
			OutputDebugStringA("COM ERR: REGDB_E_CLASSNOTREG\r\n");
		} else {
			std::string pv = std::string("hr value: ") + std::to_string(hr) + "\r\n";
			OutputDebugStringA(pv.c_str());
		}
	}
	return hr;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;
	static HWND hwndNumber;
	static HWND textdisplay;
	static bool block_update = false;

    switch (message)
    {
	case WM_CREATE:
	{
		hwndEdit = CreateWindowEx(
			0, L"EDIT",   // predefined class 
			NULL,         // no window title 
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_READONLY |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,   // set size in WM_SIZE message 
			hWnd,         // parent window 
			(HMENU)ID_EDITCHILD,   // edit control ID 
			(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
			NULL);

		hwndNumber = CreateWindowEx(
			0, L"EDIT",   // predefined class 
			NULL,         // no window title 
			WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_LEFT,
			0, 0, 0, 0,   // set size in WM_SIZE message 
			hWnd,         // parent window 
			(HMENU)ID_EDITNUM,   // edit control ID 
			(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
			NULL);

		textdisplay = CreateWindowEx(
			0, L"EDIT",   // predefined class 
			NULL,         // no window title 
			WS_CHILD | WS_VISIBLE |
			ES_LEFT,
			0, 0, 0, 0,   // set size in WM_SIZE message 
			hWnd,         // parent window 
			(HMENU)ID_DISPLAYTEXY,   // edit control ID 
			(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
			NULL);

		SendMessage(hwndEdit, EM_SETLIMITTEXT, 0, 0);

		sqlite3* db;
		if (SUCCEEDED(open_db(db))) {
			load_string_source(db);
			sqlite3_close(db);
		}

		const auto tsource = str_to_tstr(string_source);
		SetWindowText(hwndEdit, tsource.c_str());
	}
	break;

	case WM_SIZE:
		// Make the edit control the size of the window's client area. 
		{
			const auto width = LOWORD(lParam);
			const auto height = HIWORD(lParam);
			MoveWindow(hwndEdit,
				0, 0,                  // starting x- and y-coordinates 
				width,        // width of client area 
				height - 40,        // height of client area 
				TRUE);                 // repaint window
			MoveWindow(hwndNumber,
				0, height - 40,                  // starting x- and y-coordinates 
				width,        // width of client area 
				20,        // height of client area 
				TRUE);                 // repaint window 
			MoveWindow(textdisplay,
				0, height - 20,                  // starting x- and y-coordinates 
				width,        // width of client area 
				20,        // height of client area 
				TRUE);                 // repaint window 
		}
		break;
	case WM_COMMAND:
	{
		const auto id = LOWORD(wParam);
		const auto command = HIWORD(wParam);
		switch (id) {
		case ID_EDITNUM:
			if (command == EN_CHANGE) {
				if (block_update) {
					block_update = false;
					return DefWindowProc(hWnd, message, wParam, lParam);
				}
				const auto newvalue = window_text(hwndNumber);
				block_update = true;
				try {
					const long long v = stoll(newvalue);
					sref sr;
					sr.load(v);
					const auto section = str_to_tstr(sr.get());
					SetWindowText(textdisplay, section.c_str());
				} catch (...) {
					SetWindowText(textdisplay, L"No value entered");
				}
				return 0;
			} else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		case ID_DISPLAYTEXY:
			if (command == EN_CHANGE) {
				if (block_update) {
					block_update = false;
					return DefWindowProc(hWnd, message, wParam, lParam);
				}

				const auto newvalue = tstr_to_str(window_text(textdisplay));
				block_update = true;
				
				size_t off = string_source.find(newvalue);
				if (off == std::string::npos) {
					SetWindowText(hwndNumber, L"NA");
				} else {
					const auto num = std::to_wstring((newvalue.size() << 32) | off);
					SetWindowText(hwndNumber, num.c_str());
				}
			} else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}