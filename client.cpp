#define _GUN_SOURCE 1
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>


#define BUFSIZE 4096


void init(int port, const char *host)
{
    int socksev, sockcli;
    struct sockaddr_in addrsev;
    memset(&addrsev, 0, sizeof(addrsev));

    addrsev.sin_addr.s_addr = inet_addr(host);
    addrsev.sin_family = AF_INET;
    addrsev.sin_port = htons(port);

    socklen_t addrlen;

    socksev = socket(AF_INET, SOCK_STREAM, 0);
    if(socksev==-1)
    {
        perror("socket");
        exit(1);
    }

    if(connect(socksev, (sockaddr *)&addrsev, sizeof(addrsev))<0)
    {
        perror("connect");
        exit(1);
    }

    int pipefd[2];
    if(pipe(pipefd)==1)
    {
        perror("pipe");
        exit(1);
    }

    char msg[BUFSIZE];
    struct pollfd fds[2];
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = socksev;
    fds[1].events = POLLIN | POLLERR | POLLRDHUP;

    int fdn;

    while(1)
    {
        fdn = poll(fds, 2, -1);
        if(fdn<0)
        {
            perror("poll");
            break;
        }
        if(fds[1].revents & POLLRDHUP)
        {
            printf("server(socket=%d) disconnected!\n", fds[1].fd);
            break;
        }
        else if(fds[1].revents & POLLIN)
        {
            memset(msg, 0, sizeof(msg));
            recv(fds[1].fd, msg, sizeof(msg), 0);
            printf("recv data: %s\n", msg);
        }else if(fds[1].revents & POLLERR)
        {
            perror("connection error");
            break;
        }
        else if(fds[0].revents & POLLIN)
        {
            memset(msg, 0, sizeof(msg));
            while(read(fileno(stdin), msg, sizeof(msg))<=1);
            msg[strlen(msg)-1] = 0;
            send(fds[1].fd, msg, strlen(msg)+1, 0);
            // splice(fileno(stdin), 0, pipefd[1], 0, 32768, SPLICE_F_MOVE|SPLICE_F_MORE);
            // splice(pipefd[0], 0, socksev, 0, 32768, SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(socksev);
}

int main(int argc, char const *argv[])
{
    
    init(8200, "127.0.0.1");

    return 0;
}
