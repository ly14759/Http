//////////////////////////////////////////////////////////
//                                                      //
//        辅助测试CGI程序 - 计算器加法                  //
//                                                      //
//////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cgi_base.h"
//int GetQueryString(char output[]) {
//	//1.方法
//	//2.GET-》环境变量query_string
//	//3.POST-》环境变量content_length 标准输入body
//	char* method = getenv("REQUEST_METHOD");
//	if (method == NULL) {
//		fprintf(stderr, "CGI: method not find\n");
//		//stderr FILE* 指针，于文件描述符概念不一样
//		return -1;
//	}
//	fprintf(stderr, "CGI: 方法：%s\n", method);
//	if (strcasecmp(method, "GET") == 0) {
//		char* query_string_cgi = getenv("QUERY_STRING");
//		fprintf(stderr,"CGI: 参数: %s\n", query_string_cgi);
//		if (query_string_cgi == NULL) {
//			fprintf(stderr, "CGI: QUERY_STRING failed\n");
//			return -1;
//		}
//		strcpy(output, query_string_cgi);
//	} else {
//		char* content_length_str = getenv("CONTENT_LENGTH");
//		if (content_length_str == NULL) {
//			fprintf(stderr, "CONTENT_LENGTH failed\n");
//			return -1;
//		}
//		fprintf(stderr, "CGI: CL: %s\n",content_length_str);
//		int content_length = atoi(content_length_str);
//		int i = 0;
//		for(; i < content_length; ++i) {
//			//表示已经往output写了多少字符
//			read(0, output+i, 1);
//		}
//		output[content_length] = '\0';
//	}
//	return 0;
//}

int main()
{
	//1.获取参数 (方法， query_string, body)
	char buf[1024 * 4] = {0};
	int ret = GetQueryString(buf);
	if (ret < 0) {
		fprintf(stderr, "CGI: GetQueryString err\n");
		return 1;
	}
	fprintf(stderr, "CGI:buf: %s\n", buf);
	//2.解析buf中的参数，与业务相关
	//	解析时按照客户端构造请求的key来解析
	//	key = a b   a=10&b=20
	//
	int a, b;
	//解析字符串，容错率低
	sscanf(buf, "a=%d&b=%d", &a, &b);
	//3.根据业务具体要求-》加法
	int c = a + b;
	//4.结果构造成页面返回浏览器
	printf("<h1>ret=%d</h1>", c);
	return 0;
}
