#include "log.h"
#include "nwscript.h"
#include "server.h"
#include "erf.h"
#include "mysql.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

using namespace nwnx;
using namespace nwnx::core;
using namespace nwnx::nwscript;
using namespace nwnx::mysql;
using namespace nwnx::server;

namespace nwnx { namespace log {

MYSQL_STMT* stmt_log;
MYSQL_BIND bind_log[4];

void logf(level level, const char* fmt, ...)
{
	char log_message[1000000] = { 0 };
	va_list va_list;
	va_start(va_list, fmt);
	vsnprintf(log_message, 1000000, fmt, va_list);
	va_end(va_list);
	log(log_message, level, get_last_script_ran());
}

bool initialized = false;
void log(const std::string& message, level level, const std::string& script, int erf_id)
{
	if (message.empty() || !initialized) return;
	std::string script_prefix;
	if (erf_id == 0)
	{
		if (script != "")
		{
			script_prefix = erf::get_res_prefix(script);
			erf::RESERVED_PREFIX_DATA* prefix_data = erf::get_prefix_data(script_prefix);
			if (prefix_data)
			{
				erf_id = prefix_data->erf_id;
			}
			else
			{
				erf_id = 1;
			}
		}
		else
		{
			erf_id = 1;
		}
	}
	
	*(unsigned short*)bind_log[0].buffer = erf_id;
	my_mysql_stmt_bind_set_text(bind_log[1], script.empty()?NULL:script.c_str());
	*(unsigned char*)bind_log[2].buffer = (unsigned char)level;
	my_mysql_stmt_bind_set_text(bind_log[3], message.c_str());
	mysql_stmt_bind_param(stmt_log, bind_log);
	if (mysql_stmt_execute(stmt_log) != 0)
	{   
        fprintf(stderr, "stmt_log failed:%s\n", mysql_error(*mysql_admin));
    	if (script_prefix == "") script_prefix = "mod";
    	char log_filename[255];
    	sprintf(log_filename, "%s/logs.0/%s_log.txt", get_nwn_folder(), boost::algorithm::to_lower_copy(script_prefix).c_str());
    	FILE* fLog = fopen(log_filename, "a");
    	if (fLog)
    	{
    		time_t t=time(NULL);
    		char* timestamp = ctime(&t);
    		timestamp[strlen(timestamp)-1] = 0;
    		fprintf(fLog, "[%d-%s] %s\n", current_server_port, timestamp, message.c_str());
    		fclose(fLog);
    	}
    	else
    	{
    		fprintf(stderr, "failed to open:%s error:%d\n", log_filename, errno);
    	}
	}
}

void init()
{
	stmt_log = mysql_stmt_init(*mysql_admin);
	my_mysql_stmt_prepare(stmt_log, std::string("INSERT INTO sinfar.logs(time, server_port, erf_id, script, level, message) VALUES(NOW(),"+boost::lexical_cast<std::string>(current_server_port)+",?,?,?,?)").c_str());
	memset(bind_log, 0, sizeof(bind_log));
	bind_log[0].buffer_type = MYSQL_TYPE_SHORT;
	bind_log[0].buffer = new unsigned short;
	bind_log[0].is_unsigned = true;
	bind_log[1].buffer_type = MYSQL_TYPE_STRING;
	bind_log[1].length = new unsigned long;
	bind_log[1].is_null = new my_bool;
	*(bind_log[1].is_null) = false;
	bind_log[2].buffer_type = MYSQL_TYPE_TINY;
	bind_log[2].buffer = new unsigned char;
	bind_log[2].is_unsigned = true;
	bind_log[3].buffer_type = MYSQL_TYPE_STRING;
	bind_log[3].length = new unsigned long;
	bind_log[3].is_null = new my_bool;
	*(bind_log[3].is_null) = false;
	
	initialized = true;
}
REGISTER_INIT(init);
	
VM_FUNC_NEW(Log, 148)
{
	std::string log_str = vm_pop_string();
	std::string erf_prefix = vm_pop_string();
	int erf_id = 0;
	erf::RESERVED_PREFIX_DATA* prefix_data = erf::get_prefix_data(erf_prefix);
	if (prefix_data)
	{
		erf_id = prefix_data->erf_id;
	}
	log(log_str, level::info, get_last_script_ran(), erf_id);
}	
	
}
}