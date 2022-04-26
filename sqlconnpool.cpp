#include "sqlconnpool.h"

//懒汉单例模式，只有调用get_pool函数是才生出对象
sqlconnpool* sqlconnpool::get_pool()
{
	static sqlconnpool m_pool;
	return &m_pool;
}

//处理配置文件
bool sqlconnpool::deal_config()
{
	FILE* m_config = fopen("config/mysql.conf", "r");
	if (!m_config)
	{
		LOG("配置文件初始化配置失败");
		return false;
	}
	std::map<std::string, std::string> mmap;
	mmap.clear();
	while (!feof(m_config))
	{
		char buf[1024] = { 0 };
		//每次读取一行数据
		fgets(buf, 1023, m_config);
		//去掉buf最后的'\0'
		std::string m_str(buf,strlen(buf)-1);
		size_t idx = m_str.find("=");
		//去掉注释影响
		if (idx == -1 || m_str[0] == '#')
			continue;
		std::string key = m_str.substr(0, idx);
		std::string val = m_str.substr(idx + 1, m_str.find('\n', idx) - idx - 1);
		mmap.insert(std::make_pair(key, val));
	}
	m_hostip = mmap["hostip"];
	m_port = atoi(mmap["port"].c_str());
	m_user = mmap["user"];
	m_password = mmap["password"];
	m_dbname = mmap["dbname"];
	m_init_size = atoi(mmap["initSize"].c_str());
	m_max_size = atoi(mmap["maxSize"].c_str());
	m_max_idletime = atoi(mmap["maxIdleTime"].c_str());
	m_timeout = atoi(mmap["connectionTimeout"].c_str());
	return true;
}

//数据库连接池构造函数
sqlconnpool::sqlconnpool()
{
	//预处理，用配置文件对数据库连接池要链接的数据库属性进行配置
	if (!deal_config())
	{
		LOG("未能完成连接池的初始化");
		throw std::exception();
	}
	for (size_t i = 0; i < m_init_size; ++i)
	{
		connection* p = new connection();
		p->connect(m_hostip, m_user, m_password, m_dbname, m_port);
		p->refresh_alive_time();
		m_connection_queue.push(p);
		m_connection_cnt++;
	}
	//启动一个生产者线程
	std::thread produce(std::bind(&sqlconnpool::produce_connection, this));
	produce.detach();
	//启动一个定时线程，起到定时器的作用，时间一到对队列进行扫描，如果有超时的就扔出去
	std::thread scanner_connection_time(std::bind(&sqlconnpool::scan_connection_time, this));
	scanner_connection_time.detach();
}

//生产者用于生产连接的函数
void sqlconnpool::produce_connection()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		while (!m_connection_queue.empty())
		{
			//队列内还有空闲连接
			/*先unlock之前获得的mutex，然后阻塞当前的执行线程。把当前线程添加到等待线程列表中，该线程会持续
			block 直到被 notify_all() 或 notify_one()唤醒。被唤醒后，该thread会重新获取mutex，获取到mutex后执行后面的动作*/
			cv.wait(lock);
		}
		//总的连接数不能超过上限
		if (m_connection_cnt >= m_max_size)
			continue;
		connection* p = new connection();
		p->connect(m_hostip, m_user, m_password, m_dbname, m_port);
		p->refresh_alive_time();
		m_connection_queue.push(p);
		m_connection_cnt++;
		//生产完毕，通知可以使用
		cv.notify_all();
	}
}

//定时器线程的处理函数
void sqlconnpool::scan_connection_time()
{
	//扫描队列里全部的节点，将超过最大允许存活时间的去掉
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(m_max_idletime));
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		while (m_connection_cnt)
		{
			connection* p = m_connection_queue.front();
			if (p->get_alive_time() > m_max_idletime * 100 && m_connection_cnt > m_init_size / 2 + 1)
			{
				//删除一个连接
				m_connection_queue.pop();
				--m_connection_cnt;
				delete p;
			}
			else
			{
				//如果队列头部线程没有超时，则后面的肯定不会超时
				break;
			}
		}
	}
}

//消费者线程用来获取连接的函数
std::shared_ptr<connection> sqlconnpool::get_connection()
{
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	while (m_connection_queue.empty())
	{
		//等待队列为空
		cv.notify_all();     //让生产者生产一个连接出来
		if (std::cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(m_timeout)))
		{
			//如果等待超时了
			if (m_connection_queue.empty())
			{
				LOG("申请连接失败");
				return nullptr;
			}
		}
	}
	// 使用智能指针，并重写删除器，将原始指针归还到队列里面而不是析构
	std::shared_ptr<connection> p(m_connection_queue.front(), [this](connection* tmp) {
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		tmp->refresh_alive_time();  //重置时间
		m_connection_queue.push(tmp);
		}
	);
	m_connection_queue.pop();
	cv.notify_all();   //消费者取出一个，通知生产者去生产
	return p;
}