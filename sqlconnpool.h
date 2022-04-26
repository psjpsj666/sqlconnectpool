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
	static sqlconnpool* get_pool();					//单例模式对外获取对象接口
	std::shared_ptr<connection> get_connection();	//消费者获取一个连接
	~sqlconnpool() = default;

private:
	sqlconnpool();				//私有化构造函数
	bool deal_config();			//处理配置文件，从配置文件加载配置项
	void produce_connection();		//生产者生产一个连接
	void scan_connection_time();	//定时器的处理函数

private:
	std::string m_hostip;
	std::string m_user;
	std::string m_password;
	std::string m_dbname;
	unsigned int m_port;
	int m_init_size;		//连接池的初始线程数量
	int m_max_size;		//连接池的最大线程数量
	int m_max_idletime;	//连接池各线程最大空闲时间
	int m_timeout;		//获取连接的超时时间
	
	std::queue<connection*> m_connection_queue; //连接队列，用于存放连接池中的所有连接
	std::mutex m_queue_mutex;
	std::atomic<int> m_connection_cnt;	//保存现在拥有的连接数量
	std::condition_variable cv;		//用于连接生产者和消费者之间的通信
};

#endif

