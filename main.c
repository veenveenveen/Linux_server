//
//  main.cpp
//  socket_linux_c++
//
//  Created by 黄启明 on 2017/6/7.
//  Copyright © 2017年 黄启明. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#include<string.h>

#define BACKLOG 5     //完成三次握手但没有accept的队列的长度
#define CONCURRENT_MAX 8   //应用层同时可以处理的连接
#define QUIT_CMD ".quit"

#define SERVER_PORT 9897
#define BUFFER_SIZE 1024

int client_fds[CONCURRENT_MAX];
char buffer[BUFFER_SIZE];

pthread_t downloadTrd;

const char *filename = "/home/hqm/桌面/Chinese_voice/results/result";
const char *writeFilePath = "/home/hqm/桌面/Chinese_voice/receivedAudio/receivedAudioFile/voice.wav";

void *downloadThread() {
    char input_msg[BUFFER_SIZE];
    //char recv_msg[BUFFER_SIZE];
    
    
    int server_sockfd;//服务器socket
    struct sockaddr_in server_addr;//服务器网络地址结构体
    server_addr.sin_family = AF_INET;//设为IP通讯
    server_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址，允许连接到所有本地地址上
    server_addr.sin_port = htons(SERVER_PORT);//服务器端口号
    bzero(&(server_addr.sin_zero), 8);
    
    /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server error");
//        return 1;
    }
    
    /*将套接字绑定到服务器的网络地址上*/
    if ((bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
        perror("bind error");
//        return 1;
    }
    
    /*监听连接请求--监听队列长度*/
    if (listen(server_sockfd, BACKLOG) < 0) {
        perror("listen error");
//        return 1;
    }
    
    ///fd_set
    fd_set server_fd_set;
    int max_fd = -1;
    struct timeval tv;//超时时间设置
    while (1) {
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        FD_ZERO(&server_fd_set);
        FD_SET(STDIN_FILENO, &server_fd_set);
        if (max_fd < STDIN_FILENO) {
            max_fd = STDIN_FILENO;
        }
        printf("STDIN_FILENO = %d\n",STDIN_FILENO);
        
        //服务器端socket
        FD_SET(server_sockfd,&server_fd_set);
        if (max_fd < server_sockfd) {
            max_fd = server_sockfd;
        }
        //客户端连接
        for (int i = 0; i < CONCURRENT_MAX; i++) {
            if (client_fds[i] != 0) {
                FD_SET(client_fds[i],&server_fd_set);
                if (max_fd < client_fds[i]) {
                    max_fd = client_fds[i];
                }
            }
        }
        int ret = select(max_fd+1, &server_fd_set, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select 出错");
            continue;
        }
        else if (ret == 0) {
            perror("select 超时");
            continue;
        }
        else {
            if (FD_ISSET(STDIN_FILENO, &server_fd_set)) {
                printf("发送消息：\n");
                bzero(input_msg, BUFFER_SIZE);
                fgets(input_msg, BUFFER_SIZE, stdin);
                //输入 .quit 则退出服务器
                if (strcmp(input_msg, QUIT_CMD) == 0) {
                    exit(0);
                }
                for (int i = 0; i < CONCURRENT_MAX; i++) {
                    if (client_fds[i] != 0) {
                        printf("client_fds[%d] = %d\n",i,client_fds[i]);
                        if (send(client_fds[i],input_msg,BUFFER_SIZE ,0) <0) {
                            perror("send error");
//                            return 1;
                        }
                    }
                }
            }
            if (FD_ISSET(server_sockfd,&server_fd_set)) {
                //有新的连接请求
                struct sockaddr_in client_addr;
                socklen_t addr_len;
                int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
                printf("new connection client_sockfd = %d\n",client_sockfd);
                if(client_sockfd > 0)
                {
                    int index = -1;
                    for(int i = 0; i < CONCURRENT_MAX; i++)
                    {
                        if(client_fds[i] == 0)
                        {
                            index = i;
                            client_fds[i] = client_sockfd;
                            break;
                        }
                    }
                    if(index >= 0)
                    {
                        printf("新客户端(%d)加入成功 %s:%d\n", index, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }
                    else
                    {
                        bzero(input_msg, BUFFER_SIZE);
                        strcpy(input_msg, "服务器加入的客户端数达到最大值,无法加入!\n");
                        send(client_sockfd, input_msg, BUFFER_SIZE, 0);
                        printf("客户端连接数达到最大值，新客户端加入失败 %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }
                }
            }
            for(int i =0; i < CONCURRENT_MAX; i++)
            {
                if(client_fds[i] !=0)
                {
                    if(FD_ISSET(client_fds[i], &server_fd_set))
                    {
                        //                        int temp;
                        //                        if((temp=pthread_create(&downloadTrd, NULL, downloadThread(client_fds[i]), NULL))!= 0)
                        //                        {
                        //                            printf("can't create thread: %s\n",strerror(temp));
                        //                            return 1;
                        //                        }
                        
                        if (!access(writeFilePath, 0)) {
                            //access返回值为0说明文件存在
                            printf("文件存在\n");
                        }
                        
                        // 打开文件，准备写入
                        FILE *fp = fopen(writeFilePath, "w+");
                        if(NULL == fp)
                        {
                            printf("File:\t%s Can Not Open To Write\n", writeFilePath);
                            exit(1);
                        }
                        
                        // 从服务器接收数据到buffer中
                        // 每接收一段数据，便将其写入文件中，循环直到文件接收完并写完为止
                        bzero(buffer, BUFFER_SIZE);
                        long int length = 0;
                        while((length = recv(client_fds[i], buffer, BUFFER_SIZE, 0)) > 0)
                        {
                            if(fwrite(buffer, sizeof(char), length, fp) < length)
                            {
                                printf("File:\t%s Write Failed\n", writeFilePath);
                                break;
                            }
                            bzero(buffer, BUFFER_SIZE);
                        }
                        if(length < 0)
                        {
                            printf("从客户端(%d)接受消息出错.\n", i);
                        }
                        else if(length == 0)
                        {
                            FD_CLR(client_fds[i], &server_fd_set);
                            client_fds[i] = 0;
                            printf("客户端(%d)退出了\n", i);
                        }
                        fclose(fp);
                        
                        /*
                         system("/home/hqm/桌面/Chinese_voice/voice_recognized.sh");
                         
                         ///从文件中读取数据
                         int N = 100;
                         char str[N+1];
                         memset(str, 0, N+1);
                         if( (fp=fopen(filename,"rt")) == NULL ){
                         printf("Cannot open file, press any key to exit!\n");
                         exit(1);
                         }
                         
                         while(fgets(str, N, fp) != NULL){
                         printf("%s", str);
                         }
                         printf("%s",str);
                         fclose(fp);
                         
                         ///
                         for (int j = 0; j < CONCURRENT_MAX; j++) {
                         if (client_fds[i] == client_fds[j]) {
                         continue;
                         }
                         if (client_fds[j] != 0) {
                         if (send(client_fds[j],str,strlen(str) ,0) <0) {
                         perror("send error");
                         return 1;
                         }
                         }
                         }
                         //close(client_fds[i]);
                         */
                        //处理某个客户端过来的消息
                        /*
                         bzero(buffer, BUFFER_SIZE);
                         long byte_num = recv(client_fds[i], buffer, BUFFER_SIZE, 0);
                         if (byte_num > 0)
                         {
                         if(byte_num > BUFFER_SIZE)
                         {
                         byte_num = BUFFER_SIZE;
                         }
                         //recv_msg[byte_num] = '\0';
                         //printf("客户端(%d):%s\n", i, recv_msg);
                         ///
                         int length = 0;
                         if(fwrite(buffer, sizeof(char), length, fp) < length)
                         {
                         printf("File:\t%s Write Failed\n", writeFilePath);
                         break;
                         }
                         bzero(buffer, BUFFER_SIZE);
                         fclose(fp);
                         }
                         else if(byte_num < 0)
                         {
                         printf("从客户端(%d)接受消息出错.\n", i);
                         }
                         else
                         {
                         FD_CLR(client_fds[i], &server_fd_set);
                         client_fds[i] = 0;
                         printf("客户端(%d)退出了\n", i);
                         }
                         */
                        
                    }
                }
            }
        }
        
        
    }
    return (void *)0;
}

int main(int argc, const char * argv[]) {
    
    int temp;
    if((temp=pthread_create(&downloadTrd, NULL, downloadThread, NULL))!= 0)
    {
        printf("can't create thread: %s\n",strerror(temp));
        return 1;
    }
    
    printf("hello");
    while (1);
    return 0;
}
