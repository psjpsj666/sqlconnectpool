#include "sqlconnpool.h"

//��������ģʽ��ֻ�е���get_pool�����ǲ���������
sqlconnpool* sqlconnpool::get_pool()
{
	static sqlconnpool m_pool;
	return &m_pool;
}

//���������ļ�
bool sqlconnpool::deal_config()
{
	FILE* m_config = fopen("config/mysql.conf", "r");
	if (!m_config)
	{
		LOG("�����ļ���ʼ������ʧ��");
		return false;
	}
	std::map<std::string, std::string> mmap;
	mmap.clear();
	while (!feof(m_config))
	{
		char buf[1024] = { 0 };
		//ÿ�ζ�ȡһ������
		fgets(buf, 1023, m_config);
		//ȥ��buf����'\0'
		std::string m_str(buf,strlen(buf)-1);
		size_t idx = m_str.find("=");
		//ȥ��ע��Ӱ��
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

//���ݿ����ӳع��캯��
sqlconnpool::sqlconnpool()
{
	//Ԥ�����������ļ������ݿ����ӳ�Ҫ���ӵ����ݿ����Խ�������
	if (!deal_config())
	{
		LOG("δ��������ӳصĳ�ʼ��");
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
	//����һ���������߳�
	std::thread produce(std::bind(&sqlconnpool::produce_connection, this));
	produce.detach();
	//����һ����ʱ�̣߳��𵽶�ʱ�������ã�ʱ��һ���Զ��н���ɨ�裬����г�ʱ�ľ��ӳ�ȥ
	std::thread scanner_connection_time(std::bind(&sqlconnpool::scan_connection_time, this));
	scanner_connection_time.detach();
}

//�����������������ӵĺ���
void sqlconnpool::produce_connection()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		while (!m_connection_queue.empty())
		{
			//�����ڻ��п�������
			/*��unlock֮ǰ��õ�mutex��Ȼ��������ǰ��ִ���̡߳��ѵ�ǰ�߳���ӵ��ȴ��߳��б��У����̻߳����
			block ֱ���� notify_all() �� notify_one()���ѡ������Ѻ󣬸�thread�����»�ȡmutex����ȡ��mutex��ִ�к���Ķ���*/
			cv.wait(lock);
		}
		//�ܵ����������ܳ�������
		if (m_connection_cnt >= m_max_size)
			continue;
		connection* p = new connection();
		p->connect(m_hostip, m_user, m_password, m_dbname, m_port);
		p->refresh_alive_time();
		m_connection_queue.push(p);
		m_connection_cnt++;
		//������ϣ�֪ͨ����ʹ��
		cv.notify_all();
	}
}

//��ʱ���̵߳Ĵ�����
void sqlconnpool::scan_connection_time()
{
	//ɨ�������ȫ���Ľڵ㣬���������������ʱ���ȥ��
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(m_max_idletime));
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		while (m_connection_cnt)
		{
			connection* p = m_connection_queue.front();
			if (p->get_alive_time() > m_max_idletime * 100 && m_connection_cnt > m_init_size / 2 + 1)
			{
				//ɾ��һ������
				m_connection_queue.pop();
				--m_connection_cnt;
				delete p;
			}
			else
			{
				//�������ͷ���߳�û�г�ʱ�������Ŀ϶����ᳬʱ
				break;
			}
		}
	}
}

//�������߳�������ȡ���ӵĺ���
std::shared_ptr<connection> sqlconnpool::get_connection()
{
	std::unique_lock<std::mutex> lock(m_queue_mutex);
	while (m_connection_queue.empty())
	{
		//�ȴ�����Ϊ��
		cv.notify_all();     //������������һ�����ӳ���
		if (std::cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(m_timeout)))
		{
			//����ȴ���ʱ��
			if (m_connection_queue.empty())
			{
				LOG("��������ʧ��");
				return nullptr;
			}
		}
	}
	// ʹ������ָ�룬����дɾ��������ԭʼָ��黹�������������������
	std::shared_ptr<connection> p(m_connection_queue.front(), [this](connection* tmp) {
		std::unique_lock<std::mutex> lock(m_queue_mutex);
		tmp->refresh_alive_time();  //����ʱ��
		m_connection_queue.push(tmp);
		}
	);
	m_connection_queue.pop();
	cv.notify_all();   //������ȡ��һ����֪ͨ������ȥ����
	return p;
}