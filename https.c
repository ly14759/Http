#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <ctype.h>
#include <pthread.h>
#include <fcntl.h>
#include <wait.h>

#define PORT 8080
#define SIZE (1024*10)

typedef struct HttpRequesrt {
	char first_line[SIZE];
	char* method;
	char* url;
	char* url_path;
	char* query_string;
	int conent_length;
}HttpRequest;

int Handler404(int new_sock)
{
	printf("IN handler404\n");
	const char* first_line = "HTTP/1.1 404 Not Found\n";
	send(new_sock, first_line, strlen(first_line), 0);
	const char* blank_line = "\n";
	const char* body = "<h1> GG </h1>";
	char content_length[SIZE] = {0};
	sprintf(content_length, "Content-Length: %lu\n", strlen(body));
	send(new_sock, first_line, strlen(first_line), 0);
	send(new_sock, content_length, strlen(content_length), 0);
	send(new_sock, blank_line, strlen(blank_line), 0);
	send(new_sock, body, strlen(body), 0);
	return 0;
}

//读取一行
int ReadLine(int new_sock, char buf[], size_t size )
{
	//read line
	// \t \n \t\n
	int i = 0;
	char c = 'A';
	
	while (c != '\n' && i < size - 1)
	{
		ssize_t read_size = recv(new_sock, &c, 1, 0);
		if (read_size > 0) {
			if (c == '\r') {
				recv(new_sock, &c, 1, MSG_PEEK);
				if (c != '\n') 
					c = '\n';
				else
					recv(new_sock, &c, 1, 0);
			}
			buf[i++] = c;
		}
		else
			return -1;
	}
	buf[i] = '\0';
	return 0;
}

//对首行进行字符拆分
ssize_t Split(char first_line[], const char* split_char, char* output[]) {
	char* ptr = NULL;
	char* tmp = NULL;
	ptr = strtok_r(first_line, split_char, &tmp);
	int index = 0;
	while (ptr != NULL) 
	{
		output[index++] = ptr;
		ptr = strtok_r(NULL, split_char, &tmp);
	}
	return index;
}

//解析首行-拆分
int ParseFirstLine(char* first_line[], char** method_ptr, char** url_ptr)
{
	//GRT / HTTP/1.1
	char* tokens[100] = {NULL};
	ssize_t n = Split(first_line, " ", tokens);
	if (n != 3) {
		printf("line Split errn =  %ld\n", n);
		return -1;
	}
	*method_ptr = tokens[0];
	*url_ptr = tokens[1];
	return 0;
}

//读取url中参数
int ParseQueryString(char url[], char** url_path_ptr, char** query_string_ptr)
{
	//*url_path_ptr = url;// http://xxx.com/...
	*url_path_ptr = url;
	//char *ptr = url;
    //ptr = ptr + 6;
	//while (*ptr != '/')
	//{
	//	ptr++;
	//}
	//*url_path_ptr = ptr;
	printf("url_path: %s\n",*url_path_ptr);
	char* p = url;
	while(*p != NULL)
	{
		if (*p == '?') {
			*p = '\0';
			*query_string_ptr = p + 1;
			return 0;
		}
		++p;
	}
	*query_string_ptr = NULL;
	return 0;
}

//解析报头
int HandlerHeader(int new_sock, int* content_length_ptr) 
{
	printf("GO in HandlerHeader\n");
	char buf[SIZE] = {0};
	while(1) {
		if (ReadLine(new_sock, buf,sizeof(buf)) < 0) {
			printf("ReadLine failed!\n");
			return -1;
		}
		if (strcmp(buf, "\n") == 0) {
			return 0;
		}
		const char* content_length_str = "Content-Length: ";
		if (strncmp(buf, content_length_str, strlen(content_length_str)) == 0){
			//printf("buf: %s\n",buf);
		//	printf("CL: %s\n", content_length_ptr);
			*content_length_ptr =  atoi(buf + strlen(content_length_str));
			//printf("CL: %d\n", *content_length_ptr);
		}
	} //end while(1);
	return 0;
}

//路径是否为目录
int IsDir(const char* file_path) {
	struct stat st;
	int ret = stat(file_path, &st);
	if (ret < 0) {
		return 0;
	}
	if (S_ISDIR(st.st_mode)) {
		return 1;
	}
	return 0;
}

//添加默认路径
void HanderFilePath(const char* url_path, char file_path[])
{
	//printf("Go in path1\n");
	sprintf(file_path, "./wwwroot/%s", url_path);
	//printf("Go in path\n");
	if (file_path[strlen(file_path) - 1] == '/') {
		strcat(file_path, "index.html");
	} else {
		if (IsDir(file_path)) {
			strcat(file_path, "/index.html");
		}
	}
	//IsDir...
	return;
}

//获得文件大小
ssize_t GetFileSize(const char* file_path)
{
	struct stat st;
	int ret = stat(file_path, &st);
	if (ret < 0) {
		return 0;
	}
	return st.st_size;
}

//读取静态文件
int WriteStaticFile(int new_sock, const char* file_path)
{
	int fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 404;
	}
	//GetFile,,,
	size_t file_size = GetFileSize(file_path);

	const char* first_line = "HTTP/1.1 200 OK\n";
	char header[SIZE] = {0};
	sprintf(header, "Content-Length: %lu\n", file_size);
	const char* blank_line = "\n";

	send(new_sock, first_line, strlen(first_line), 0);
	send(new_sock, header, strlen(header), 0);
	send(new_sock, blank_line, strlen(blank_line), 0);

	sendfile(new_sock, fd, NULL, file_size);
	close(fd);
	return 200;
}

//静态页面处理
int HandlerStaticFile(int new_sock, const HttpRequest* req)
{
	printf("Go in file\n");
	char file_path[SIZE] = {0};
	HanderFilePath(req->url_path, file_path);
	int err_code = WriteStaticFile(new_sock, file_path);
	return err_code;
}

//CGI-父进程核心操作
int HandlerCGIFather(int new_sock, int father_read, int father_write, const HttpRequest* req)
{
		//a);
	if (strcasecmp(req->method, "POST") == 0) {
		//根据body的长度读取字节
		char c = '\0';
		int i = 0;
		//body一个一个的读到管道; 避免太长，read被信号打断
		//sendfile 不能用，目标必须是sock
		printf("CGIFA: CL: %d\n", req->conent_length);
		for (; i < req->conent_length; ++i) {
			read(new_sock, &c, 1);
			printf("C:%c\n", c);
			write(father_write, &c, 1);
		}
	}
		// b)
		const char* first_line = "HTTP/1.1 200 OK\n";
		send(new_sock, first_line, strlen(first_line), 0);
		const char* blank_line = "\n";
		send(new_sock, blank_line, strlen(blank_line), 0);
		// c)
		// 一个一个字符的读，sendfile数据长度不确定
		char c = '\0';
		while(read(father_read, &c, 1) > 0) {
			printf("c: %c\n", c);
			write(new_sock, &c, 1);
		}
//		fflush(&stdout);
		// d)
		// 已忽略SIGCHLD信号，子进程退出直接回收
		// 或用wiatpid，传入参数pid,回收特定的子进程
		printf("cgi farher over\n");
		return 200;
	}


//CGI-子进程核心操作
int HandlerCGIChild(int child_read, int child_wirte, const HttpRequest* req)
{
	// a)
	// 设置环境变量的步骤不能由父进程来进行，因为同一时刻由多个请求，
	//		每个请求都会修改环境变量,会造成环境变量的问题
	char method_env[SIZE] = {0};
	char query_string_env[SIZE] = {0};
	char content_length_env[SIZE] = {0};
	sprintf(method_env, "REQUEST_METHOD=%s", req->method);
	putenv(method_env);
	if (strcasecmp(req->method, "GET") == 0) {
		sprintf(query_string_env, "QUERY_STRING=%s", req->query_string);
		//printf("参数：%s\n", req->query_string);
		putenv(query_string_env);
		char* ret = getenv("QUERY_STRING");
		printf("CGI GET:参数env: %s\n", ret);
	} else {
		sprintf(content_length_env, "CONTENT_LENGTH=%d", req->conent_length);
		putenv(content_length_env);
	}
	printf("out1\n");
	// b)
	dup2(child_read, 0);
	printf("close cr\n");
	dup2(child_wirte, 1);
	//printf("close cw\n");
	// c)
	char file_path[SIZE] = {0};
	//printf("path: %s\n", req->url_path);
	HanderFilePath(req->url_path, file_path);
	// l列表 lp不需要路径 le自助设计环境变量
	// v数组 vp不需要路径 ve自助设计环境变量
	// execl 参数：路径 命令名字(也是路径) 参数 NULL参数结束
	//printf("CGI:exec GO->> ");
	//printf("%s\n",file_path);
	execl(file_path, file_path, NULL);
	// d)
	// 替换失败，没有存在的必要，不终止会对父进程的逻辑造成干扰;
	exit(0);
}

//动态页面-CGI机制-动态页面生成技术
int HandlerCGI(int new_sock, const HttpRequest* req)
{
	//1.创建一对匿名管道
	//2.创建子进程 fork
	//3.父进程操作-》
	//	a)如果是post请求，把body数据写到管道中
	//		动态生成页面的过程交给子进程完成
	//	b)构造http响应，首行，header，\n 用于回复客户端
	//	c)从管道中读取数据-》子进程生成的页面
	//		也写到socet中
	//	d)进程等待，回收子进程的资源
	//4.子进程核心流程-》
	//	a)设置环境变量-》全局变量，父子cgi程序都可以用。
	//		METHOD,QUERY_STRING, CONTENT_LENGTH 方便，标准
	//	b)表标准输入和标准输出重定向到管道上;
	//		此时CGI程序读写标准输入/输出相当于读写管道
	//	c)程序替换-》先找到执行哪个CGI程序，替换(代码+数据);
	//		替换成功。动态页面交给CGI程序执行
	//	d)替换失败-》进程终止，不然子进程会继续做父进程的工作
	//5.收尾工作-关闭管道.
	printf("GO in CGI\n");
	int err_code = 200;
	int fd1[2], fd2[2];
	pipe(fd1);
	pipe(fd2);
	int father_read = fd1[0];	
	int child_wirte = fd1[1]; 
	int father_write = fd2[1];
	int child_read = fd2[0];

	pid_t ret = fork();
	if (ret > 0) {
		//提前关闭-》父进程从管道中读，read能正确返回不阻塞
		//所有写端关闭，才能读完，并退出
		close(child_wirte);
		close(child_read);
		err_code = HandlerCGIFather(new_sock, father_read, father_write, req);
	} else if (ret == 0) {
		close(father_read);
		close(father_write);
		err_code = HandlerCGIChild(child_read, child_wirte, req);	
	} else {
		perror("fork");
		goto END;
	}
	printf("father over1\n");
END:
	close(father_read);
	close(father_write);
	close(child_wirte);
	close(child_read);
	printf("father over\n");
	return err_code;
}

void Errdo(int code, int new_sock)
{
	switch (code) {
		case 200:
			return;
		case 404:
			Handler404(new_sock);
			break;
		default: break;
	}
	//close(new_sock);
}

void HandlerRequest(int new_sock ) {
	//1. du qiu jie xi
	//2. geng ju qing kuang fenlei
	int err_code = 200;
	HttpRequest req; 
	memset(&req, 0, sizeof(req));
	printf("GG\n");
	if (ReadLine(new_sock, req.first_line, sizeof(req.first_line) - 1) < 0)
	{
		printf("ReadLine\n");
		err_code = 404;
		Errdo(err_code, new_sock);
	}
	printf("ReadLine: %s\n", req.first_line);
	if (ParseFirstLine(req.first_line, &req.method, &req.url) < 0 ) {
		printf("ParseFirstLine");
		Errdo(404, new_sock);
	}
	printf("method:%s\n",req.method);
	if (ParseQueryString(req.url, &req.url_path, &req.query_string) < 0) {
		printf("ParseQueryString url = %s\n", req.url);
		Errdo(404, new_sock);
	}
	printf("query_string: %s\n", req.query_string);
	printf("CL: %s\n", req.conent_length);
	printf("参数下\n");
	if (HandlerHeader(new_sock, &req.conent_length) < 0) {
		printf("Header");
		Errdo(404, new_sock);
	}
	printf("HandlerHeader CL %d\n",req.conent_length);
	if (strcasecmp(req.method, "GET") == 0 && req.query_string == NULL) {
		err_code = HandlerStaticFile(new_sock, &req);
		Errdo(err_code, new_sock);
	} else if (strcasecmp(req.method, "GET") == 0 && req.query_string != NULL) {
		err_code = HandlerCGI(new_sock, &req);
		Errdo(err_code, new_sock);
	} else if (strcasecmp(req.method, "POST") == 0) {
		err_code = HandlerCGI(new_sock, &req);	
		Errdo(err_code, new_sock);
	} else 	{
		printf("method is not");
		Errdo(404, new_sock);
	}
	//close(new_sock);
}

void * ThreadEntry(void* arg) {
	//do what
	int new_sock = ((int64_t)arg);
	HandlerRequest(new_sock);
	return NULL;
}

void HttpServerStart(const char* ip, short port)
{
	//1. socket
	//2. bind
	//3. listem
	//4. accept
	int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);
	int ret = bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		perror("bind");
		return;
	}

	ret = listen(listen_sock, 5);
	if (ret < 0) {
		perror("listen");
		return;
	}

	while(1)
	{
		struct sockaddr_in peer;
		socklen_t len = sizeof(peer);
		//int* new_sock = (int*)malloc(sizeof(int));
		int64_t new_sock = accept(listen_sock, (struct sockaddr*)&peer, &len);
		if (new_sock < 0) {
			perror("accept");
			continue;
		} 
		// pid
		pthread_t tid;
		pthread_create(&tid, NULL, ThreadEntry, (void*)new_sock);
		pthread_detach(tid);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage ./https [ip] [port]\n");
		return 1;
	}
	signal(SIGCHLD, SIG_IGN);//忽略sigchld信号
	HttpServerStart(argv[1], atoi(argv[2]));
//	pthread_create(&tid, NULL, handler_request, (void*)nsock);
	return 0;
}
