/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2

void accept_request(void *);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(void *arg)
{
    int client = (intptr_t)arg;  // 将参数转换为整数类型，并赋值给client变量
    char buf[1024];  // 定义一个大小为1024的字符数组
    size_t numchars;  // 定义一个大小为size_t的变量
    char method[255];  // 定义一个大小为255的字符数组
    char url[255];  // 定义一个大小为255的字符数组
    char path[512];  // 定义一个大小为512的字符数组
    size_t i, j;  // 定义两个大小为size_t的变量
    struct stat st;  // 定义一个stat结构体变量
    int cgi = 0;  // 定义一个整数变量并初始化为0，用于判断是否为CGI程序
    char *query_string = NULL;  // 定义一个字符指针变量并初始化为NULL

    numchars = get_line(client, buf, sizeof(buf));  // 调用get_line函数，将结果赋值给numchars变量
    i = 0; j = 0;  // 初始化i和j变量为0
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))  // 当buf[i]不为空格且i小于method数组的大小时，执行循环
    {
        method[i] = buf[i];  // 将buf[i]赋值给method[i]
        i++;  // i自增
    }
    j=i;  // 将i的值赋值给j
    method[i] = '\0';  // 在method[i]处添加字符串结束符'\0'

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))  // 如果method不是GET也不是POST，执行条件内的代码
    {
        unimplemented(client);  // 调用unimplemented函数，传入client参数
        return;  // 返回
    }

    if (strcasecmp(method, "POST") == 0)  // 如果method是POST，执行条件内的代码
        cgi = 1;  // 将cgi变量赋值为1

    i = 0;  // 将i变量初始化为0
    while (ISspace(buf[j]) && (j < numchars))  // 当buf[j]为空格且j小于numchars时，执行循环
        j++;  // j自增
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))  // 当buf[j]不为空格且i小于url数组的大小且j小于numchars时，执行循环
    {
        url[i] = buf[j];  // 将buf[j]赋值给url[i]
        i++; j++;  // i和j自增
    }
    url[i] = '\0';  // 在url[i]处添加字符串结束符'\0'

    if (strcasecmp(method, "GET") == 0)  // 如果method是GET，执行条件内的代码
    {
        query_string = url;  // 将url赋值给query_string
        while ((*query_string != '?') && (*query_string != '\0'))  // 当*query_string不是'?'且不是字符串结束符时，执行循环
            query_string++;  // query_string自增
        if (*query_string == '?')  // 如果*query_string是'?'，执行条件内的代码
        {
            cgi = 1;  // 将cgi变量赋值为1
            *query_string = '\0';  // 在*query_string处添加字符串结束符'\0'
            query_string++;  // query_string自增
        }
    }

    sprintf(path, "htdocs%s", url);  // 将格式化的字符串写入path数组
    if (path[strlen(path) - 1] == '/')  // 如果path数组的最后一个字符是'/'，执行条件内的代码
        strcat(path, "index.html");  // 将"index.html"拼接到path数组的末尾
    if (stat(path, &st) == -1) {  // 如果获取path路径下的文件信息失败，执行条件内的代码
    /*
        stat(path, &st) == -1 是一个条件语句，用于检查给定路径的文件或目录的状态是否无法获取。
        在这里，stat() 函数用于获取文件或目录的信息，并将其存储在 st 结构体中。
        如果 stat() 函数返回 -1，意味着无法获取文件或目录的状态。这可能是由于给定路径不存在、权限不足或其他错误导致的。
    */
        while ((numchars > 0) && strcmp("\n", buf))  // 当numchars大于0且buf与"\n"不相等时，执行循环
            numchars = get_line(client, buf, sizeof(buf));  // 调用get_line函数，将结果赋值给numchars变量
        not_found(client);  // 调用not_found函数，传入client参数
    }
    else  // 否则，执行else语句块内的代码
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)  // 如果文件类型是目录，执行条件内的代码
            strcat(path, "/index.html");  // 将"/index.html"拼接到path数组的末尾
        if ((st.st_mode & S_IXUSR) ||  // 如果文件具有用户执行权限或组执行权限或其他执行权限，执行条件内的代码
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
            cgi = 1;  // 将cgi变量赋值为1
        /*
        CGI（通用网关接口，Common Gateway Interface）是一种用于在Web服务器上执行程序的标准接口。
        它允许Web服务器接收来自客户端的请求，并将请求传递给服务器上的外部程序进行处理。
        CGI程序可以是任何可执行文件，例如C、C++、Python、Perl等编写的脚本或程序。
        在给定的代码中，如果请求的方法是POST或URL中包含查询字符串（GET请求），则将cgi变量设置为1，表示需要执行CGI程序。
        在这种情况下，将调用execute_cgi函数来处理请求。
        CGI程序可以用于动态生成网页内容、处理表单数据、与数据库交互等。它可以接收来自Web服务器的请求参数，并生成相应的响应内容。
        通过CGI，可以实现与用户的交互、数据处理和动态内容生成等功能。
        */
        if (!cgi)  // 如果cgi为0，执行条件内的代码
            serve_file(client, path);  // 调用serve_file函数，传入client和path参数
        else  // 否则，执行else语句块内的代码
            execute_cgi(client, path, method, query_string);  // 调用execute_cgi函数，传入client、path、method和query_string参数
    }

    close(client);  // 关闭client连接
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];
    // 从资源文件中读取一行数据到缓冲区
    fgets(buf, sizeof(buf), resource);
    // 循环直到资源文件的末尾
    while (!feof(resource))
    {
        // 将缓冲区的内容发送给客户端
        send(client, buf, strlen(buf), 0);
        // 继续从资源文件中读取下一行数据到缓冲区
        fgets(buf, sizeof(buf), resource);
    }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,
        const char *method, const char *query_string)
{
    char buf[1024];  // 缓冲区，用于存储读取的数据
    int cgi_output[2];  // 用于存储 CGI 脚本的输出管道
    int cgi_input[2];  // 用于存储 CGI 脚本的输入管道
    pid_t pid;  // 子进程的进程ID
    int status;  // 子进程的退出状态
    int i;
    char c;
    int numchars = 1;  // 读取的字符数
    int content_length = -1;  // POST 请求的内容长度

    buf[0] = 'A'; buf[1] = '\0';  // 初始化缓冲区

    // 如果是 GET 请求，则读取并丢弃请求头部
    if (strcasecmp(method, "GET") == 0)
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
    // 如果是 POST 请求
    else if (strcasecmp(method, "POST") == 0) /*POST*/
    {
        numchars = get_line(client, buf, sizeof(buf));
        // 读取请求头部，获取 Content-Length
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        // 如果未找到 Content-Length，则返回错误
        if (content_length == -1) {
            bad_request(client);
            return;
        }
    }
    // 如果是 HEAD 请求或其他请求类型，则不做处理

    // 创建 CGI 脚本的输出管道
    if (pipe(cgi_output) < 0) {
        cannot_execute(client);
        return;
    }
    // 创建 CGI 脚本的输入管道
    if (pipe(cgi_input) < 0) {
        cannot_execute(client);
        return;
    }

    // 创建子进程来执行 CGI 脚本
    if ( (pid = fork()) < 0 ) {
        cannot_execute(client);
        return;
    }

    // 在父进程中发送 HTTP 响应头部
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    if (pid == 0)  /* child: CGI script */
    {
        char meth_env[255];  // 存储请求方法的环境变量
        char query_env[255];  // 存储查询字符串的环境变量
        char length_env[255];  // 存储内容长度的环境变量

        // 重定向子进程的标准输出和标准输入到管道
        dup2(cgi_output[1], STDOUT);
        /*
            dup2() 函数是一个系统调用，用于复制文件描述符。它接受两个参数：旧文件描述符（oldfd）和新文件描述符（newfd）。
            dup2() 函数将新文件描述符指向与旧文件描述符相同的文件或管道。
        */
        dup2(cgi_input[0], STDIN);
        close(cgi_output[0]);
        close(cgi_input[1]);

        // 设置环境变量
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        /*
            putenv(meth_env) 是一个函数调用，用于设置环境变量。
            在C语言中，putenv() 函数用于修改或添加环境变量。它接受一个字符串参数，
            该字符串包含一个环境变量的名称和值，格式为 "name=value"。在这种情况下，
            meth_env 是一个包含环境变量名称和值的字符串。
        */
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {   /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }

        // 执行 CGI 脚本
        execl(path, NULL);
        exit(0);
    } else {    /* parent */
        // 关闭父进程中不需要的管道端口
        close(cgi_output[1]);
        close(cgi_input[0]);

        // 如果是 POST 请求，则将数据发送给 CGI 脚本
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++) {
                recv(client, &c, 1, 0);
                /*
                recv(client, &c, 1, 0) 是一个系统调用函数，用于从套接字接收数据。
                在C语言中，recv() 函数用于从指定的套接字接收数据。它接受四个参数：第一个参数 client 是套接字描述符，
                表示要接收数据的套接字；第二个参数 &c 是一个指向接收缓冲区的指针，用于存储接收的数据；
                第三个参数 1 表示要接收的数据的最大长度；第四个参数 0 是一个标志，用于指定接收数据的方式。
                */
                write(cgi_input[1], &c, 1);
            }

        // 从 CGI 脚本读取输出，并发送给客户端
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        // 关闭管道端口
        close(cgi_output[0]);
        close(cgi_input[1]);

        // 等待子进程退出
        waitpid(pid, &status, 0);
    }
}
/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;  // 初始化变量i，用于追踪当前读取到的字符位置
    char c = '\0';  // 初始化变量c，用于存储当前读取到的字符
    int n;  // 初始化变量n，用于存储recv函数的返回值

    while ((i < size - 1) && (c != '\n'))  // 循环直到读取到换行符或达到缓冲区大小的上限
    {
        n = recv(sock, &c, 1, 0);  // 从套接字sock接收一个字符，并将其存储在变量c中
        /* DEBUG printf("%02X\n", c); */  // 调试语句，用于打印变量c的十六进制值
        if (n > 0)  // 如果接收到的字符数大于0
        {
            if (c == '\r')  // 如果当前字符是回车符
            {
                n = recv(sock, &c, 1, MSG_PEEK);  // 预读下一个字符，但不从套接字中删除它
                /* DEBUG printf("%02X\n", c); */  // 调试语句，用于打印变量c的十六进制值
                if ((n > 0) && (c == '\n'))  // 如果下一个字符是换行符
                    recv(sock, &c, 1, 0);  // 从套接字中读取下一个字符，并将其存储在变量c中
                else
                    c = '\n';  // 否则，将变量c设置为换行符
            }
            buf[i] = c;  // 将当前字符存储在缓冲区中
            i++;  // 增加变量i的值，指向下一个位置
        }
        else
            c = '\n';  // 如果接收到的字符数小于等于0，则将变量c设置为换行符，以结束循环
    }
    buf[i] = '\0';  // 在缓冲区的末尾添加字符串终止符

    return(i);  // 返回读取到的字符数
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
    int httpd = 0;  // 创建一个套接字描述符
    int on = 1;  // 用于设置套接字选项的变量
    struct sockaddr_in name;  // 存储套接字地址的结构体

    httpd = socket(PF_INET, SOCK_STREAM, 0);  // 创建一个TCP套接字
    /*
    这行代码创建了一个套接字。具体解释如下：
    - `socket` 是一个系统调用，用于创建一个套接字。
    - `PF_INET` 是套接字的地址族，表示使用IPv4地址。
       PF_INET6：表示使用IPv6地址。IPv6是下一代互联网协议，使用128位的地址空间，以取代IPv4的32位地址空间。
    - `SOCK_STREAM` 是套接字的类型，表示使用面向连接的TCP套接字。
        其它套接字类型
        SOCK_DGRAM：面向消息的套接字类型，使用UDP协议。UDP是一种无连接的、不可靠的传输协议，适用于需要快速传输但不需要可靠性的应用程序。
        SOCK_RAW：原始套接字类型，可以直接访问底层网络协议。使用原始套接字可以发送和接收网络层（IP）或传输层（TCP、UDP）的原始数据包。
        SOCK_SEQPACKET：面向连接的可靠数据包套接字类型。类似于 SOCK_STREAM，但提供了数据包边界保护，确保数据包按照发送的顺序到达。
        SOCK_RDM：可靠的数据报套接字类型，提供有序、可靠的数据传输。但在实际中很少使用。
    - `0` 是套接字的协议参数，表示使用默认的协议。
因此，`httpd = socket(PF_INET, SOCK_STREAM, 0);` 创建了一个TCP套接字，并将其描述符存储在 `httpd` 变量中。
    */
    if (httpd == -1)
        error_die("socket");  // 如果套接字创建失败，则报错

    memset(&name, 0, sizeof(name));  // 清空套接字地址结构体

    name.sin_family = AF_INET;  // 设置套接字地址族为IPv4
    //在套接字编程中，AF_INET 和 PF_INET 实际上是等价的，都表示使用IPv4地址族。

    name.sin_port = htons(*port);  // 设置套接字端口号，并将主机字节序转换为网络字节序
    //htons(*port) 是一个函数调用，用于将一个16位的端口号从主机字节序转换为网络字节序。

    name.sin_addr.s_addr = htonl(INADDR_ANY);  // 设置套接字IP地址为任意可用地址
    /*
    htonl 是一个函数，用于将32位的整数值从主机字节序转换为网络字节序（大端字节序）。
    INADDR_ANY 是一个常量，表示任意IP地址。
    */

    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  
    {  
        /*
            `SOL_SOCKET` 是套接字选项的级别，用于指定要设置或获取的套接字选项的类型。
            在套接字编程中，可以使用 `setsockopt` 函数来设置或修改套接字选项的值，以便控制套接字的行为和属性。
            `SOL_SOCKET` 是 `setsockopt` 函数中的一个参数，用于指定要操作的选项级别。`
            `SOL_SOCKET` 表示套接字级别的选项，用于设置或获取与套接字本身相关的选项。
            这些选项可以影响套接字的行为、性能和属性，例如套接字的超时设置、缓冲区大小、地址重用等。
            通过将 `SOL_SOCKET` 作为选项级别传递给 `setsockopt` 函数，可以对套接字级别的选项进行设置或修改。
            然后，可以使用选项名称和选项值来指定要设置或获取的具体选项。
        */
        error_die("setsockopt failed");  // 设置套接字选项失败，则报错
    }

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        /*
        这段代码是在将套接字 httpd 绑定到指定的地址上。
        bind 函数用于将套接字与特定的地址进行绑定。它接受套接字描述符 httpd、地址结构体指针 (struct sockaddr *)&name 
        和地址结构体的大小 sizeof(name) 作为参数。
        在这段代码中，(struct sockaddr *)&name 是将 name 结构体转换为通用的 struct sockaddr 结构体指针。
        这是因为 bind 函数接受的地址参数类型是 struct sockaddr，而 name 是 struct sockaddr_in 类型的结构体。
        bind 函数的作用是将套接字 httpd 绑定到指定的地址上，以便在该地址上监听传入的连接请求。
        如果绑定成功，则返回值为 0；如果绑定失败，则返回值小于 0。
        绑定失败可能是因为指定的地址已经被其他套接字占用或者权限不足等原因。
        */
        error_die("bind");  // 绑定套接字到指定的地址和端口号失败，则报错
    if (*port == 0)  /* if dynamically allocating a port */
    {
        //在套接字编程中，通常可以手动指定要使用的端口号，也可以选择动态分配一个可用的端口号。
        //当 port 的值为 0 时，表示正在动态分配一个端口号。
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
        /*
            getsockname 函数的作用是获取与套接字 httpd 关联的本地地址信息，并将其存储在 name 结构体中。
            同时，它还会更新 namelen 变量的值，以指示实际存储的地址结构体大小。
            如果获取本地地址信息成功，则 getsockname 函数返回值为 0；如果获取失败，则返回值为 -1。获取失败可能是因为套接字描述符无效或其他错误导致的。
            在获取成功后，可以通过访问 name 结构体的成员来获取本地地址的详细信息。
        */
            error_die("getsockname");  // 获取动态分配的端口号失败，则报错
        *port = ntohs(name.sin_port);  // 将获取到的端口号转换为主机字节序，并存储在传入的指针变量中
        /*
            在网络通信中，不同的计算机体系结构可能使用不同的字节序（大端字节序或小端字节序）。
            为了确保在不同的计算机之间正确地解释和处理数据，需要进行字节序的转换。
            当涉及到网络通信时，通常会使用网络字节序（大端字节序）来表示数据。
            因此，在发送或接收网络数据时，需要将数据转换为网络字节序，以便其他计算机能够正确地解释和处理数据。
        */
    }
    if (listen(httpd, 5) < 0)
    /*
    这段代码是在将套接字 httpd 设置为监听状态，以便接受传入的连接请求。
    listen 函数用于将套接字设置为监听状态，以便接受传入的连接请求。
    它接受套接字描述符 httpd 和一个整数参数，指定同时排队等待处理的连接请求的最大数量。
    在这段代码中，listen(httpd, 5) 将套接字 httpd 设置为监听状态，并指定最大排队等待连接请求的数量为 5。
    如果 listen 函数调用成功，则返回值为 0，表示套接字已成功设置为监听状态。
    如果调用失败，则返回值小于 0，表示设置监听状态失败，可能是由于套接字无效或其他错误导致的。
    一旦套接字处于监听状态，它将开始接受传入的连接请求，并将其排队等待处理。
    当有新的连接请求到达时，可以使用 accept 函数接受连接，并与客户端建立通信。
    */
        error_die("listen");  // 监听套接字失败，则报错
    return(httpd);  // 返回套接字描述符
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void)
{
    int server_sock = -1;  // 服务器套接字
    u_short port = 4000;  // 端口号
    int client_sock = -1;  // 客户端套接字
    struct sockaddr_in client_name;  // 客户端地址结构体
    socklen_t  client_name_len = sizeof(client_name);  // 客户端地址结构体的长度
    /*
    socklen_t是一个数据类型，用于表示套接字地址结构体的长度。
    它是一个无符号整数类型，在网络编程中经常用于传递套接字地址结构体的长度参数。
    */
    pthread_t newthread;  // 线程标识符
    /*
    `pthread_t`的定义可以在 `<pthread.h>` 头文件中找到。这是一个不透明的数据类型，实际上是一个结构体或者指针的别名，用于表示线程的标识符。
    在不同的系统中，`pthread_t`的具体定义可能会有所不同。以下是一个可能的定义示例：
    ```c
    typedef unsigned long int pthread_t;
    ```
    这个定义将`pthread_t`定义为`unsigned long int`类型的别名。在其他系统中，可能会使用其他类型，如指针类型。
    无论具体的定义是什么，重要的是要理解`pthread_t`是一个用于标识线程的数据类型，可以用于创建、管理和操作线程。
    具体的线程操作函数（如`pthread_create`、`pthread_join`等）会使用`pthread_t`类型的参数来标识和操作线程。
    */
    server_sock = startup(&port);  // 启动服务器并获取服务器套接字
    
    printf("httpd running on port %d\n", port);  // 打印服务器运行的端口号

    while (1)  // 无限循环，接受客户端连接
    {
        client_sock = accept(server_sock,
                (struct sockaddr *)&client_name,
                &client_name_len);  // 接受客户端连接请求，获取客户端套接字
        if (client_sock == -1)
            error_die("accept");  // 如果接受连接失败，则打印错误信息并退出
        /* accept_request(&client_sock); */
        if (pthread_create(&newthread , NULL, (void *)accept_request, (void *)(intptr_t)client_sock) != 0)
            perror("pthread_create");  // 创建新线程来处理客户端请求
    }

    close(server_sock);  // 关闭服务器套接字

    return(0);  // 程序正常结束
}