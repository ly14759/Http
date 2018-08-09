///////////////////////////////////////////////////
// 基于 mysql api 实现mysql客户端查找所有数据
// 不需要知道用户传递的参数
//////////////////////////////////////////////////
#include <stdio.h>
#include <mysql/mysql.h>

#include "cgi_base.h"

#define IP 192.168.46.117
#define USER root
#define PASSWORD op
#define NAME test

int main() {
	//1.获取参数 query_string
	char buf[1024] = {0};
	//2.初始化mysql 客户端句柄
	//	句柄--遥控器
	MYSQL* connect_fd = mysql_init(NULL);

	//3.与服务器建立连接
	//参数：1)初始化过的mysql客户端句柄
	//		2)服务器地址ip地址
	//		3)登陆mysql服务器的用户名
	//		4)登陆mysql服务器的密码
	//		5)数据库名
	//		6)mysql服务器的端口号3306
	//		7)不关注NULL
	//		8)不关注0
	if (mysql_real_connect(connect_fd, "127.0.0.1",
				"root","op","test",3306,NULL, 0) == NULL) {
		fprintf(stderr, "mysql real connect failed\n");
	}
	fprintf(stderr, "Mysql connect OK\n");
	//4.拼装 sql 语句
	//  不用分号结尾
	const char* sql = "select * from tests";

	//5.sql语句发送到服务器上
	int ret = mysql_query(connect_fd, sql);
	if (ret < 0) {
		fprintf(stderr, "mysql_query faile\n");
		return 1;
	}
	fprintf(stderr, "mysql query OK\n");
	
	//6.查看sql执行结果-》一个表
	MYSQL_RES* result = mysql_store_result(connect_fd);
	if (result == NULL) {
		fprintf(stderr, "mysql_store_result failed\n");
	}
	fprintf(stderr, "mysql_store_result OK\n");
	//成功-》进一步获取详细信息
	//	a)行数和列数
	//	b)表头结构-》字段，含义
	//	c)每一行具体所有的字段
	int rows = mysql_num_rows(result);
	int fields = mysql_num_fields(result);
	
	printf("<html>");
	printf("<div>rows =%d fields=%d</div>", rows, fields);
	// mysql-fetch_field 每次调用获取到一个表头中的字段
	MYSQL_FIELD* f = mysql_fetch_field(result);
	while (f != NULL) {
		//name，把一列的名字打印出来
		printf("%s\t", f->name);
		f = mysql_fetch_field(result);
	}
	int i = 0;
	for (; i < rows; ++i) {
		// row 类似一列的内容 个数 = 列数
		MYSQL_ROW row = mysql_fetch_row(result);
		int j = 0;
		for (; j < fields; ++j) {
			printf("%s\t", row[j]);
		}
		printf("<br>"); //换行
	}
	printf("</html>");
	//7.关闭连接
	//mysql_close(connect_fd);
	return 0;
}
