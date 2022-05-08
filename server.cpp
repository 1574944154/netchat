#define _GUN_SOURCE 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#define BUFSIZE 4096

char buf[BUFSIZE];

int MAXNFDS = 1024;

int readline(int sock, char buf[])
{
    char c=1;
    int ret;
    char *t = buf;
    while(c)
    {
        ret=recv(sock, &c, 1, 0);
        if(ret<=0)
        {
            perror("recv");
            return -1;
        }
        *t++ = c;
    }
    return t-buf-1;
}

int process_request(int sock)
{
    memset(buf, 0, sizeof(buf));
    int size;
    if((size=read(sock, buf, sizeof(buf)))<=0)
    {
        printf("read %d\n", size);
        return size;
    }

    printf("get %d bytes from %d\n", size, sock);

    sprintf(buf, "%d", size);

    if(size=write(sock, buf, strlen(buf))<=0)
    {
        printf("write %d\n", size);   
        return size;
    }
    return 1;
}

int socket_init(int port, const char *host)
{
    int socksev;
    struct sockaddr_in addrsev;
    memset(&addrsev, 0, sizeof(addrsev));

    addrsev.sin_addr.s_addr = inet_addr(host);
    addrsev.sin_family = AF_INET;
    addrsev.sin_port = htons(port);

    socksev = socket(AF_INET, SOCK_STREAM, 0);
    if(socksev==-1)
    {
        perror("socket");
        return -1;
    }
    if(bind(socksev, (sockaddr *)&addrsev, sizeof(addrsev))<0)
    {
        perror("bind");
        return -1;
    }
    if(listen(socksev, 5)<0)
    {
        perror("listen");
        return -1;
    }
    return socksev;
}

void init(int port, const char *host)
{
    int socksev, sockcli;
    struct sockaddr_in addrcli;
    socklen_t addrlen;

    socksev = socket_init(port, host);
    if(socksev<0) exit(1);

    int maxfd = socksev;
    struct pollfd fds[MAXNFDS];
    for(int i=0;i<MAXNFDS;i++)
        fds[i].fd = -1;
    
    fds[socksev].fd = socksev;
    fds[socksev].events = POLLIN | POLLERR;

    int fdn;
    while(1)
    {
        fdn = poll(fds, maxfd+1, -1);

        if( fdn<0)
        {
            perror("poll");
            break;
        }

        for(int eventfd=0;eventfd<=maxfd;eventfd++)
        {
            if(fds[eventfd].fd==-1) continue;

            if(fds[eventfd].revents==0) continue;

            if((fds[eventfd].fd==socksev) && (fds[eventfd].revents & POLLIN))
            {
                if( ( sockcli=accept(socksev, (sockaddr *)&addrcli, &addrlen)) <0)
                {
                    perror("accept");
                    continue;
                }
                // FD_SET(sockcli, &readfdset);

                if(sockcli>MAXNFDS)
                {
                    printf("clientsock(%d)>MAXNFDS, close\n", sockcli);
                    close(sockcli);
                }

                fds[sockcli].fd = sockcli;
                fds[sockcli].events = POLLIN | POLLRDHUP | POLLERR;
                fds[sockcli].revents = 0;

                if(maxfd<sockcli) maxfd = sockcli;

                printf("client(socket=%d) connected\n", sockcli);

            }
            else if(fds[eventfd].revents & POLLRDHUP)
            {
                printf("client(sock=%d) disconnected\n", fds[eventfd].fd);
                close(fds[eventfd].fd);
                fds[eventfd].fd = -1;
                
                if(fds[eventfd].fd==maxfd)
                {
                    for(int i=maxfd;i>0;i--)
                    {
                        if(fds[i].fd!=-1) 
                        {
                            maxfd = i;
                            break;
                        }
                    }
                }
            }
            else if(fds[eventfd].revents & POLLIN)
            {
                memset(buf, 0, sizeof(buf));
                // int ret = recv(fds[eventfd].fd, buf, sizeof(buf), 0);
                int ret = readline(fds[eventfd].fd, buf);
                printf("recv %d %d bytes data from client(socket=%d): %s\n", ret, strlen(buf), fds[eventfd].fd, buf);

                for(int i=0;i<=maxfd;i++)
                {
                    if(fds[i].fd!=-1 && i!=eventfd && fds[i].fd!=socksev)
                    {
                        printf("send data to %d\n", fds[i].fd);
                        send(fds[i].fd, buf, strlen(buf)+1, 0);
                    }
                }
            }else if(fds[eventfd].revents & POLLERR)
            {
                printf("get err! close\n");
                close(fds[eventfd].fd);
            }

        }
    }
}

int main(int argc, char const *argv[])
{
    init(8200, "127.0.0.1");
    return 0;
}
