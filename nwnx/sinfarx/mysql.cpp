#include "mysql.h"
#include "nwscript.h"
#include "erf.h"
#include "objectstorage.h"
#include "log.h"
#include <vector>

using namespace nwnx::log;
using namespace nwnx::erf;
using namespace nwnx::nwscript;

namespace nwnx { namespace mysql {

void my_mysql_stmt_bind_set_text(MYSQL_BIND& bind, const char* text)
{
	if (text)
	{
		if (bind.buffer) free(bind.buffer);
		bind.buffer_length = strlen(text)+1;
		bind.buffer = malloc(bind.buffer_length);
		memcpy(bind.buffer, text, bind.buffer_length);
		*bind.length = bind.buffer_length-1;
		*bind.is_null = false;
	}
	else
	{
		if (bind.buffer) free(bind.buffer);
		bind.buffer_length = 0;
		bind.buffer = NULL;
		*bind.length = 0;
		*bind.is_null = true;
	}
}
void my_mysql_stmt_prepare(MYSQL_STMT* stmt, const char* sql)
{
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		fprintf(stderr, " mysql_stmt_prepare failed(%s):%s\n", sql, mysql_stmt_error(stmt));
	}
}

void CMySQLRes::swap(const CMySQLRes& from)
{
	mysql_result = from.mysql_result;
	num_col = from.num_col;
	current_row = from.current_row;
	lengths = from.lengths;
	from.mysql_result = NULL;
	from.num_col = 0;
	from.current_row = NULL;
	from.lengths = NULL;
}
CMySQLRes::CMySQLRes(MYSQL_RES* mysql_result) : mysql_result(mysql_result), current_row(NULL), lengths(NULL)
{
	num_col = (mysql_result ? mysql_num_fields(mysql_result) : 0);
}
CMySQLRes::CMySQLRes() : CMySQLRes(NULL){};
CMySQLRes::CMySQLRes(const CMySQLRes& copy_from)
{
	swap(copy_from);
}
CMySQLRes& CMySQLRes::operator=(const CMySQLRes& assign_from)
{
	if (this != &assign_from)
	{
		free_result();
		swap(assign_from);
	}
	return *this;
}
MYSQL_ROW CMySQLRes::fetch_row()
{
	if (mysql_result)
	{
		current_row = mysql_fetch_row(mysql_result);
	}
	else
	{
		current_row = NULL;
		logf(level::error, "Trying to fetch a row for a NULL result set");
	}
	return current_row;
};
MYSQL_LENGTHS CMySQLRes::get_lengths()
{
	if (!lengths)
	{
		if (mysql_result)
		{
			lengths = mysql_fetch_lengths(mysql_result);
		}
		else
		{
			logf(level::error, "Trying to fetch lengths for a NULL result set");
		}
	}
	return lengths;
}
size_t CMySQLRes::get_column_count()
{
	return num_col;
}
MYSQL_ROW CMySQLRes::get_current_row()
{
	return current_row;
}
uint32_t CMySQLRes::num_rows()
{
	return mysql_num_rows(mysql_result);
}
void CMySQLRes::free_result()
{
	if (mysql_result)
	{
		mysql_free_result(mysql_result);
		mysql_result = NULL;
		num_col = 0;
		current_row = NULL;
		lengths = NULL;
	}
}
CMySQLRes::~CMySQLRes(){free_result();}

int last_sql_query_error_code = 0;
CMySQLCon::CMySQLCon(const std::string& hostname, const std::string& username, const std::string& password, const std::string& dbname) :
	hostname(hostname),
	username(username),
	password(password),
	dbname(dbname)
{
	mysql_init(&mysql);
	my_bool recon=true;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &recon);
	if (!mysql_real_connect(&mysql, hostname.c_str(), username.c_str(), password.c_str(), dbname.c_str(), 0, 0, 0))
	{
		fprintf(stderr, "Couldn't connect to the database:%s(%s) with user:%s\n", dbname.c_str(), hostname.c_str(), username.c_str());
	}
}
CMySQLCon::operator MYSQL*(){return &mysql;}
std::string CMySQLCon::quote(const std::string& value)
{
	char* encoded_buffer = (char*)malloc(value.length()*2+1+2);
	encoded_buffer[0] = '\'';
	int encoded_len = mysql_real_escape_string(*mysql_restricted, encoded_buffer+1, value.c_str(), value.length());
	encoded_buffer[encoded_len+1] = '\'';
	encoded_buffer[encoded_len+2] = 0;
	return encoded_buffer;
}
bool CMySQLCon::query(const std::string& sql)
{
	int result = mysql_query(&mysql, sql.c_str());
	if (result != 0)
	{
		logf(level::error, "SQL Query Failed: (%s)=%s", sql.c_str(), mysql_error(&mysql));
		last_sql_query_error_code = mysql_errno(&mysql);
		return false;
	}
	else
	{
		last_sql_query_error_code = 0;
		return true;
	}
}
bool CMySQLCon::set_password(const std::string& new_password)
{
	if (query("SET PASSWORD = PASSWORD("+quote(new_password)+")"))
	{
		password = new_password;
		return true;
	}
	else
	{
		return false;
	}
}
std::string CMySQLCon::query_string(const std::string& sql)
{
	if (query(sql))
	{
		CMySQLRes result = store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			return (row[0] ? row[0] : "");
		}
	}
	return "";
}
std::vector<std::string> CMySQLCon::query_row(const std::string& sql)
{
	std::vector<std::string> row_values;
	if (query(sql))
	{
		CMySQLRes result = store_result();
		MYSQL_ROW row;
		if ((row = result.fetch_row()) != NULL)
		{
			size_t column_count = result.get_column_count();
			for (size_t column_index=0; column_index<column_count; column_index++)
			{
				row_values.push_back(row[column_index] ? row[column_index] : "");
			}
		}
	}
	return row_values;
}
int CMySQLCon::query_int(const std::string& sql)
{
	std::string result = query_string(sql);
	return (result.empty() ? 0 : atoi(result.c_str()));
}
CMySQLRes CMySQLCon::store_result()
{
	return CMySQLRes(mysql_store_result(&mysql));
}
uint32_t CMySQLCon::affected_rows()
{
	return mysql_affected_rows(&mysql);
}
uint32_t CMySQLCon::insert_id()
{
	return mysql_insert_id(&mysql);
}
CMySQLCon::~CMySQLCon()
{
	mysql_close(&mysql);;
}


std::unique_ptr<CMySQLCon> mysql_admin;
std::unique_ptr<CMySQLCon> mysql_sinfar;
std::unique_ptr<CMySQLCon> mysql_restricted;
CMySQLCon* get_server_mysql_con()
{
	if (get_current_script_in_core()) //the script can be ran on one of sinfar servers?
	{
		return mysql_sinfar.get();
	}
	else
	{
		return mysql_restricted.get();
	}
}
std::string mysql_escape_string(const std::string& string)
{
	char escaped_string[string.length()*2+1];
	mysql_real_escape_string(*mysql_admin, escaped_string, string.c_str(), string.length());
	return escaped_string;
}
std::string to_sql(const std::string& string)
{
	return "'"+mysql_escape_string(string)+"'";
}
std::unique_ptr<char> object_to_sql(uint32_t object_id, int& result_len)
{
	int object_data_size;
	std::unique_ptr<char> object_data(nwnx::odbc::SaveObject(object_id, &object_data_size));
	if (object_data.get())
	{
		std::unique_ptr<char> encoded_data((char*)malloc(object_data_size * 2 + 1 + 2));
		char* encoded_data_ptr = encoded_data.get();
		int encoded_data_len = mysql_real_escape_string(*mysql_admin, encoded_data_ptr + 1, object_data.get(), object_data_size);
		encoded_data_ptr[0] = encoded_data_ptr[encoded_data_len + 1] = '\'';
		encoded_data_ptr[encoded_data_len + 2] = 0x0;
		result_len = encoded_data_len+2;
		return encoded_data;
	}
	result_len = 0;
	return object_data;
}
uint32_t read_object_from_sql_result(CMySQLRes& sql_result, uint32_t column, const CScriptLocation& location, uint32_t owner_id)
{
    uint32_t result_obj_id = OBJECT_INVALID;
    if (column < sql_result.get_column_count() && sql_result.get_current_row())
    {
        MYSQL_LENGTHS lengths = sql_result.get_lengths();
        if (lengths)
        {
            result_obj_id = nwnx::odbc::LoadObject(sql_result.get_current_row()[column], lengths[column], location, owner_id);
        }
    }
    return result_obj_id;
}
std::deque<CMySQLRes> mysql_server_results;

void init()
{
	
}
REGISTER_INIT(init);
	
VM_FUNC_NEW(SQLEncodeSpecialChars, 34)
{
	CExoString to_encode = vm_pop_string();
	CExoString result;
	if (to_encode.text)
	{
		uint32_t to_encode_len = strlen(to_encode.text);
		char* encoded_buffer = (char*)malloc(to_encode_len*2+1);
		int encoded_len = mysql_real_escape_string(*mysql_restricted, encoded_buffer, to_encode.text, to_encode_len);
		result.text = encoded_buffer;
		result.len = encoded_len;
	}
	vm_push_string(&result);
}
VM_FUNC_NEW(SQLQuote, 43)
{
	std::string to_encode = vm_pop_string();
	char* encoded_buffer = (char*)malloc(to_encode.length()*2+1+2);
	encoded_buffer[0] = '\'';
	int encoded_len = mysql_real_escape_string(*mysql_restricted, encoded_buffer+1, to_encode.c_str(), to_encode.length());
	encoded_buffer[encoded_len+1] = '\'';
	encoded_buffer[encoded_len+2] = 0;
	CExoString result;
	result.text = encoded_buffer;
	result.len = encoded_len+2;
	vm_push_string(&result);
}
inline void vm_sql_exec(CMySQLCon* db_con)
{
	CExoString sql = vm_pop_string();
	CScriptDbResult dbresult;
	if (sql.text && db_con->query(sql.text))
	{
		mysql_server_results.push_front(db_con->store_result());
		if (mysql_server_results.size() > 200)
		{
			fprintf(stderr, "possible dbresult leak:%u\n", mysql_server_results.size());
		}
		dbresult.reset(new CScriptDbResultData(mysql_server_results.front()));
	}
	vm_push_dbresult(&dbresult);   
}
VM_FUNC_NEW(SQLExec, 42)
{
    vm_sql_exec(mysql_restricted.get());
}
VM_FUNC_NEW(SQLCoreExec, 548)
{
    vm_sql_exec(get_server_mysql_con());
}
VM_FUNC_NEW(SQLFetch, 36)
{
	vm_push_int(!mysql_server_results.empty() && mysql_server_results.front().fetch_row());
}
VM_FUNC_NEW(SQLAffectedRows, 324)
{
	vm_push_int(mysql_restricted->affected_rows());
}
VM_FUNC_NEW(SQLCoreAffectedRows, 549)
{
	vm_push_int(get_server_mysql_con()->affected_rows());
}
inline const char* vm_func_get_sql_data()
{
	uint32_t col = vm_pop_int() - 1;
	if (mysql_server_results.empty()) return NULL;
	CMySQLRes& res = mysql_server_results.front();
	if (col < res.get_column_count() && res.get_current_row())
	{
		return res.get_current_row()[col];
	}
	else
	{
		return NULL;
	}
}
VM_FUNC_NEW(SQLGetData, 37)
{
	CExoString result_exo(vm_func_get_sql_data());
	vm_push_string(&result_exo);
}
VM_FUNC_NEW(SQLGetInt, 38)
{
	const char* result = vm_func_get_sql_data();
	vm_push_int(result?atoi(result):0);
}
VM_FUNC_NEW(SQLGetFloat, 39)
{
	const char* result = vm_func_get_sql_data();
	vm_push_float(result?atof(result):0);
}
VM_FUNC_NEW(SQLGetObject, 35)
{
	uint32_t col = vm_pop_int() - 1;
	CScriptLocation location = vm_pop_location();
	uint32_t owner_id = vm_pop_object();
	uint32_t result_obj_id = OBJECT_INVALID;
	if (!mysql_server_results.empty())
	{
        result_obj_id = read_object_from_sql_result(mysql_server_results.front(), col, location, owner_id);
	}
	vm_push_object(result_obj_id);
}

VM_FUNC_NEW(SQLStoreObject, 40)
{
	CExoString sql = vm_pop_string();
	uint32_t object_id = vm_pop_object();
	int result = 0;
	if (sql.text)
	{
		int object_sql_len = 0;
		std::unique_ptr<char> object_sql = object_to_sql(object_id, object_sql_len);
		if (object_sql.get())
		{
			std::unique_ptr<char> formatted_sql((char*)malloc(strlen(sql.text)+object_sql_len+2+1));
			sprintf(formatted_sql.get(), sql.text, object_sql.get());
			result = mysql_restricted->query(formatted_sql.get());
		}
	}
	vm_push_int(result);
}
VM_FUNC_NEW(SQLRetrieveObject, 41)
{
	CExoString sql = vm_pop_string();
	CScriptLocation location = vm_pop_location();
	uint32_t owner_id = vm_pop_object();
	uint32_t result = OBJECT_INVALID;
	if (sql.text)
	{
		if (mysql_restricted->query(sql.text))
		{
			CMySQLRes sql_result = mysql_restricted->store_result();
            sql_result.fetch_row();
            result = read_object_from_sql_result(sql_result, 0, location, owner_id);
		}
	}
	vm_push_object(result);
}
VM_FUNC_NEW(SQLGetInsertId, 373)
{
	vm_push_int(mysql_restricted->insert_id());
}
VM_FUNC_NEW(SQLCoreGetInsertId, 550)
{
	vm_push_int(get_server_mysql_con()->insert_id());
}
VM_FUNC_NEW(SQLGetLastResultCode, 374)
{
	vm_push_int(last_sql_query_error_code);
}

VM_FUNC_NEW(VectorToString, 528)
{
    Vector v = vm_pop_vector();
    vm_push_string(vector_to_string(v));
}
VM_FUNC_NEW(StringToVector, 529)
{
    std::string v_str = vm_pop_string();
    vm_push_vector(string_to_vector(v_str));
}
VM_FUNC_NEW(LocationToString, 530)
{
    CScriptLocation loc = vm_pop_location();
    vm_push_string(location_to_string(loc));
}
VM_FUNC_NEW(StringToLocation, 531)
{
    std::string loc_str = vm_pop_string();
    vm_push_location(string_to_location(loc_str)); 
}
VM_FUNC_NEW(GetDatabaseName, 547)
{
    vm_push_string(mysql_restricted->dbname);
}

}
}
