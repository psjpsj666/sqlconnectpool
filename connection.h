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
	//��ʼ�����Ӳ���
	connection();

	//�ر����ݿ�
	~connection();

	//�������ݿ�mysql -u host -p password -- use database;
	/*
	hostip��mysql����ip��ַ
	user��mysql��¼�û���
	password��mysql��¼����
	dbname����Ҫ���ӵ�mysql���ݿ�����
	port��mysql�˿ں�
	*/
	bool connect(std::string hostip, std::string user, std::string password, std::string dbname, unsigned int port = 3306);

	//�������ݿ⣺update, insert, delete
	bool update(std::string& sql);

	//��ѯ���ݿ⣺select
	MYSQL_RES* query(std::string& sql);

	//ˢ����ʼ�Ŀ���ʱ���
	void refresh_alive_time();

	//���ش���ʱ��
	clock_t get_alive_time();

private:
	MYSQL* _conn;
	clock_t alive_time;  //��¼�������״̬�����ʼʱ��㣨Ӧ���ڽ�����е�ʱ�����ʱ��㣩
};


#endif // !CONNECTION_H

