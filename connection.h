#pragma once
#ifndef CONNECTION_H
#define CONNECTION_H

#include <mysql.h>
#include <string>
#include <iostream>
#include <ctime>

#define LOG(MSG) std::cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__ << ": " << MSG << std::endl;

class connection
{
public:
	//初始化连接参数
	connection();

	//关闭数据库
	~connection();

	//连接数据库mysql -u host -p password -- use database;
	/*
	hostip：mysql主机ip地址
	user：mysql登录用户名
	password：mysql登录密码
	dbname：需要连接的mysql数据库名称
	port：mysql端口号
	*/
	bool connect(std::string hostip, std::string user, std::string password, std::string dbname, unsigned int port = 3306);

	//更新数据库：update, insert, delete
	bool update(std::string& sql);

	//查询数据库：select
	MYSQL_RES* query(std::string& sql);

	//刷新起始的空闲时间点
	void refresh_alive_time();

	//返回存活的时间
	clock_t get_alive_time();

private:
	MYSQL* _conn;
	clock_t alive_time;  //记录进入空闲状态后的起始时间点（应该在进入队列的时候更新时间点）
};


#endif // !CONNECTION_H

