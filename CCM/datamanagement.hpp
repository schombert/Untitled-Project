#pragma once
#include "globalhelpers.h"

#define _DEBUG_ON_

class stmtwrapper {
public:
	sqlite3_stmt* stmt;
	stmtwrapper() : stmt(nullptr) {};
	stmtwrapper(sqlite3* const db, const char* sql) : stmt(nullptr) {
#ifdef _DEBUG_ON_
		int res =
#endif
		sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
#ifdef _DEBUG_ON_
		if (res != SQLITE_OK) {
			OutputDebugStringA((std::string(sql) + " SQL PREP ERR: ").c_str());
			OutputDebugStringA(sqlite3_errstr(res));
		}
#endif
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
#ifdef _DEBUG_ON_
		if (res == SQLITE_OK || res == SQLITE_DONE)
			return false;
		OutputDebugStringA((std::string(sqlite3_sql(stmt)) + " SQL EXEC ERR: ").c_str());
		OutputDebugStringA(sqlite3_errstr(res));
#endif
		return false;

	}
	operator sqlite3_stmt*() const {
		return stmt;
	};
	stmtwrapper copy(sqlite3* const db) const {
		return stmtwrapper(db, sqlite3_sql(stmt));
	}
};

class bindings {
public:
	sqlite3_stmt* stmt;
	bindings(sqlite3_stmt* s) : stmt(s) {
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
	};
	
	bindings(const bindings& b) = delete;
	~bindings() {
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
	}
	template <typename T>
	void operator() (const int indx, T val) noexcept {
		if (val.value != T::NONE)
			sqlite3_bind_int64(stmt, indx, val.value);
		else
			sqlite3_bind_null(stmt, indx);
	}
	template <>
	void operator() (const int indx, const sqlite_int64 val) noexcept {
		sqlite3_bind_int64(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const size_t val) noexcept {
		sqlite3_bind_int64(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const int val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const short val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const char val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const unsigned int val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const unsigned short val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const unsigned char val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const double val) noexcept {
		sqlite3_bind_double(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const float val) noexcept {
		sqlite3_bind_double(stmt, indx, static_cast<double>(val));
	}
	void operator() (const int indx) noexcept {
		sqlite3_bind_null(stmt, indx);
	}
	template <>
	void operator()<sref>(const int indx, sref r) noexcept {
		if(r.to_int() != 0)
			sqlite3_bind_int64(stmt, indx, r.to_int());
		else
			sqlite3_bind_null(stmt, indx);
	}
	void operator() (const int indx, const void* data, size_t size) noexcept {
		sqlite3_bind_blob(stmt, indx, data, static_cast<int>(size), SQLITE_STATIC);
	}
	
};

inline void load_sql_value(IN_P(void) destination, size_t bytes, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	bytes = std::min(bytes, static_cast<size_t>(sqlite3_column_bytes(stmt, indx)));
	memcpy(destination, sqlite3_column_blob(stmt, indx), bytes);
}

template<typename T>
inline void load_sql_value(INOUT(T) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	if (sqlite3_column_type(stmt, indx) != SQLITE_NULL)
		destination.value = static_cast<decltype(destination.value)>(sqlite3_column_int64(stmt, indx));
	else
		destination.value = T::NONE;
}

template<typename V, typename A>
inline void load_sql_value(INOUT(std::vector<V, A>) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	const size_t elements = static_cast<size_t>(sqlite3_column_bytes(stmt, indx)) / sizeof(V);
	destination.resize(elements);
	memcpy(destination.data(), sqlite3_column_blob(stmt, indx), elements * sizeof(V));
}

template<>
inline void load_sql_value<std::string>(INOUT(std::string) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination.assign((char*)(sqlite3_column_text(stmt, indx)));
}

template<>
inline void load_sql_value<sref>(INOUT(sref) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination.load(sqlite3_column_int64(stmt, indx));
}

template<>
inline void load_sql_value<std::wstring>(INOUT(std::wstring) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination.assign((wchar_t*)(sqlite3_column_text16(stmt, indx)));
}

template<>
inline void load_sql_value<float>(INOUT(float) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<float>(sqlite3_column_double(stmt, indx));
}
template<>
inline void load_sql_value<double>(INOUT(double) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = sqlite3_column_double(stmt, indx);
}
template<>
inline void load_sql_value<int>(INOUT(int) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = sqlite3_column_int(stmt, indx);
}
template<>
inline void load_sql_value<unsigned int>(INOUT(unsigned int) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned int>(sqlite3_column_int64(stmt, indx));
}
template<>
inline void load_sql_value<long long>(INOUT(long long) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = sqlite3_column_int64(stmt, indx);
}
template<>
inline void load_sql_value<unsigned long long>(INOUT(unsigned long long) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned long long>(sqlite3_column_int64(stmt, indx));
}
template<>
inline void load_sql_value<short>(INOUT(short) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<short>(sqlite3_column_int(stmt, indx));
}
template<>
inline void load_sql_value<unsigned short>(INOUT(unsigned short) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned short>(sqlite3_column_int(stmt, indx));
}
template<>
inline void load_sql_value<char>(INOUT(char) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<char>(sqlite3_column_int(stmt, indx));
}
template<>
inline void load_sql_value<unsigned char>(INOUT(unsigned char) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned char>(sqlite3_column_int(stmt, indx));
}
