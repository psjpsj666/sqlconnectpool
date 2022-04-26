#pragma once
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <cstring>
#include <map>
#include <exception>
#include <functional>
#include "connection.h"


class sqlconnpool
{
public:
	static sqlconnpool* get_pool();					//����ģʽ�����ȡ����ӿ�
	std::shared_ptr<connection> get_connection();	//�����߻�ȡһ������
	~sqlconnpool() = default;

private:
	sqlconnpool();				//˽�л����캯��
	bool deal_config();			//���������ļ����������ļ�����������
	void produce_connection();		//����������һ������
	void scan_connection_time();	//��ʱ���Ĵ�����

private:
	std::string m_hostip;
	std::string m_user;
	std::string m_password;
	std::string m_dbname;
	unsigned int m_port;
	int m_init_size;		//���ӳصĳ�ʼ�߳�����
	int m_max_size;		//���ӳص�����߳�����
	int m_max_idletime;	//���ӳظ��߳�������ʱ��
	int m_timeout;		//��ȡ���ӵĳ�ʱʱ��
	
	std::queue<connection*> m_connection_queue; //���Ӷ��У����ڴ�����ӳ��е���������
	std::mutex m_queue_mutex;
	std::atomic<int> m_connection_cnt;	//��������ӵ�е���������
	std::condition_variable cv;		//�������������ߺ�������֮���ͨ��
};

#endif

