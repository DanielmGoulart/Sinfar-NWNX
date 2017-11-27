#pragma once

#include "core.h"
#include <deque>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>

namespace nwnx { 

namespace nwscript {
	class CScriptDbResultData;
}

namespace mysql {

void my_mysql_stmt_bind_set_text(MYSQL_BIND& bind, const char* text);
void my_mysql_stmt_prepare(MYSQL_STMT* stmt, const char* sql);

typedef long unsigned int* MYSQL_LENGTHS;
class CMySQLRes
{
	friend class nwnx::nwscript::CScriptDbResultData;
protected:
	mutable MYSQL_RES* mysql_result;
private:
	mutable size_t num_col;
	mutable MYSQL_ROW current_row;
	mutable MYSQL_LENGTHS lengths;
	void swap(const CMySQLRes& from);
public:
	CMySQLRes(MYSQL_RES* mysql_result);
	CMySQLRes();
	CMySQLRes(const CMySQLRes& copy_from);
	CMySQLRes& operator=(const CMySQLRes& assign_from);
	MYSQL_ROW fetch_row();
	MYSQL_LENGTHS get_lengths();
	size_t get_column_count();
	MYSQL_ROW get_current_row();
	uint32_t num_rows();
	void free_result();
	~CMySQLRes();
};
extern int last_sql_query_error_code;
class CMySQLCon
{
private:
	CMySQLCon() = delete;
	CMySQLCon(const CMySQLCon&) = delete;
	MYSQL mysql;
	std::string hostname;
	std::string username;
	std::string password;
public:
	const std::string dbname;
public:
	CMySQLCon(const std::string& hostname, const std::string& username, const std::string& password, const std::string& dbname);
	operator MYSQL*();
	bool query(const std::string& sql);
	std::string quote(const std::string& value);
	bool set_password(const std::string& new_password);
	std::string query_string(const std::string& sql);
	std::vector<std::string> query_row(const std::string& sql);
	int query_int(const std::string& sql);
	CMySQLRes store_result();
	uint32_t affected_rows();
	uint32_t insert_id();
	~CMySQLCon();
};
extern std::unique_ptr<CMySQLCon> mysql_admin;
extern std::unique_ptr<CMySQLCon> mysql_sinfar;
extern std::unique_ptr<CMySQLCon> mysql_restricted;
extern std::deque<CMySQLRes> mysql_server_results;
CMySQLCon* get_server_mysql_con();
std::string mysql_escape_string(const std::string& string);
std::string to_sql(const std::string& string);
std::unique_ptr<char> object_to_sql(uint32_t object_id, int& result_len);
uint32_t read_object_from_sql_result(CMySQLRes& sql_result, uint32_t column, const CScriptLocation& location, uint32_t owner_id);

}
}