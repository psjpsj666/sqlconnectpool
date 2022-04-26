#include "connection.h"

connection::connection()
{
	_conn = mysql_init(NULL);
	alive_time = 0;
}

connection::~connection()
{
	if(!_conn)
		mysql_close(_conn);
}

bool connection::connect(std::string hostip, std::string user, std::string password, std::string dbname, unsigned int port)
{
	_conn = mysql_real_connect(_conn, hostip.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
	if (!_conn)
	{
		LOG("MySQL����ʧ��");
		return false;
	}
	LOG("MySQL ���ӳɹ�");
	return true;
}

bool connection::update(std::string& sql)
{
	int ret = mysql_query(_conn, sql.c_str());
	if (ret == 0)
	{
		LOG("MySQL���³ɹ�");
		return true;
	}
	LOG("MySQL����ʧ��" + std::string(mysql_error(_conn)));
	return false;
}

MYSQL_RES* connection::query(std::string& sql)
{
	int ret = mysql_query(_conn, sql.c_str());
	MYSQL_RES* res = mysql_use_result(_conn);
	if (ret == 0)
	{
		LOG("MySQL��ѯ�ɹ�");
		return res;
	}
	LOG("MySQL��ѯʧ�ܣ�"+ std::string(mysql_error(_conn)));
	return nullptr;
}

void connection::refresh_alive_time()
{
	alive_time = clock_t();
}

clock_t connection::get_alive_time()
{
	return clock() - alive_time;
}