/* selectserver.c */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9005  /* default port number */
#define QLEN 6      /* size of request queue */

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    int sockfd, new_fd; /* listen on sockfd, new connection on new_fd */
    struct sockaddr_in cad; /* structure to hold client's address */
    int alen; /* length of address */
    fd_set readfds, activefds; /* the set of read descriptors */
    int i, maxfd = 0, numbytes;
    char buf[100];

    /* 소켓 생성 */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        exit(1);
    }

    /* 주소 구조체 초기화 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 소켓에 주소 바인딩 */
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(1);
    }

    /* 연결 요청 대기열 설정 */
    if (listen(sockfd, QLEN) < 0) {
        fprintf(stderr, "listen failed\n");
        exit(1);
    }

    alen = sizeof(cad);

    /* Initialize the set of active sockets. */
    FD_ZERO(&activefds);
    FD_SET(sockfd, &activefds);
    maxfd = sockfd;

    /* Main server loop - accept & handle requests */
    fprintf(stderr, "Server up and running.\n");

    while (1) {
        printf("SERVER: Waiting for contact ..., %d\n", maxfd);

        /* Block until input arrives on one or more active sockets. */
        readfds = activefds; // 중요: select 호출 전 원본 set 복사
        
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i <= maxfd; ++i) {
            if (FD_ISSET(i, &readfds)) {
                
                /* 새로운 연결 요청 처리 (리스닝 소켓인 경우) */
                if (i == sockfd) {
                    if ((new_fd = accept(sockfd, (struct sockaddr *)&cad, (socklen_t*)&alen)) < 0) {
                        fprintf(stderr, "accept failed\n");
                        exit(1);
                    }
                    
                    FD_SET(new_fd, &activefds); // add the new socket desc to our active connections set
                    printf("set\n");
                    
                    if (new_fd > maxfd)
                        maxfd = new_fd;
                }
                /* 기존 연결 소켓에서 데이터 수신 */
                else {
                    /* Data arriving on an already-connected socket. */
                    // 이미지의 로직을 따르되, 오류 처리를 위해 recv의 반환값을 먼저 확인합니다.
                    if ((numbytes = recv(i, buf, 100, 0)) <= 0) {
                        // 0이면 연결 종료, -1이면 에러
                        if (numbytes == 0) {
                            printf("Socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &activefds);
                    } 
                    else {
                        // 데이터 수신 성공 시 에코 및 연결 종료 (이미지 로직 반영)
                        buf[numbytes] = '\0';
                        if (send(i, buf, strlen(buf), 0) == -1) {
                            perror("send");
                        }
                        
                        // 이미지의 코드 흐름상 데이터를 한 번 주고받고 닫는 구조로 보입니다.
                        // (일반적인 채팅 서버는 여기서 닫지 않지만, 이미지의 들여쓰기를 따름)
                        close(i);
                        FD_CLR(i, &activefds);
                    }
                } // end else
            } // end if FD_ISSET
        } // end for
    } // end while
    
    close(sockfd);
    return 0;
}