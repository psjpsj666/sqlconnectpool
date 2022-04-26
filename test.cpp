#include "connection.h"
#include "sqlconnpool.h"

using namespace std;

int main()
{
	string sql("INSERT INTO user_list (UserID, UserName, Password) VALUES(55,'Mike','ggbyto')");

	sqlconnpool* tmp = sqlconnpool::get_pool();
	string stat;
	while (cin >> stat)
	{
		if (stat == "quit") break;
		shared_ptr<connection> p = tmp->get_connection();
		p->update(sql);
	}

	printf("over\n");
	return 0;
}