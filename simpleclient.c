#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int sockfd; // 声明一个整型变量 sockfd，用于存储套接字描述符
    int len; // 声明一个整型变量 len，用于存储地址结构体的大小
    struct sockaddr_in address; // 声明一个 sockaddr_in 结构体变量 address，用于存储服务器地址信息
    int result; // 声明一个整型变量 result，用于存储连接结果
    char ch = 'A'; // 声明一个字符变量 ch，初始化为 'A'

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 创建一个 TCP 套接字，返回套接字描述符，存储在 sockfd 中
    address.sin_family = AF_INET; // 设置地址族为 IPv4
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // 设置服务器 IP 地址为 127.0.0.1
    address.sin_port = htons(9734); // 设置服务器端口号为 9734
    len = sizeof(address); // 获取地址结构体的大小
    result = connect(sockfd, (struct sockaddr *)&address, len); // 连接到服务器，返回连接结果，存储在 result 中

    if (result == -1) // 如果连接失败
    {
        perror("oops: client1"); // 输出错误信息
        exit(1); // 退出程序
    }
    write(sockfd, &ch, 1); // 向服务器发送一个字符
    read(sockfd, &ch, 1); // 从服务器接收一个字符
    printf("char from server = %c\n", ch); // 打印从服务器接收到的字符
    close(sockfd); // 关闭套接字
    exit(0); // 退出程序
}