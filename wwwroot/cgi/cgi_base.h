#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//inline-> 建议编译器 内联; 
static int GetQueryString(char output[]) {
	//1.方法
	//2.GET-》环境变量query_string
	//3.POST-》环境变量content_length 标准输入body
	char* method = getenv("REQUEST_METHOD");
	if (method == NULL) {
		fprintf(stderr, "CGI: method not find\n");
		//stderr FILE* 指针，于文件描述符概念不一样
		return -1;
	}
	fprintf(stderr, "CGI: 方法：%s\n", method);
	if (strcasecmp(method, "GET") == 0) {
		char* query_string_cgi = getenv("QUERY_STRING");
		fprintf(stderr,"CGI: 参数: %s\n", query_string_cgi);
		if (query_string_cgi == NULL) {
			fprintf(stderr, "CGI: QUERY_STRING failed\n");
			return -1;
		}
		strcpy(output, query_string_cgi);
	} else {
		char* content_length_str = getenv("CONTENT_LENGTH");
		if (content_length_str == NULL) {
			fprintf(stderr, "CONTENT_LENGTH failed\n");
			return -1;
		}
		fprintf(stderr, "CGI: CL: %s\n",content_length_str);
		int content_length = atoi(content_length_str);
		int i = 0;
		for(; i < content_length; ++i) {
			//表示已经往output写了多少字符
			read(0, output+i, 1);
		}
		output[content_length] = '\0';
	}
	return 0;
}
