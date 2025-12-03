#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 3490
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char buf[BUF_SIZE];
    char name[20] = "Unknown";
    
    fd_set reads, copy_reads;
    int fd_max, str_len;

    if (argc != 2) {
        printf("Usage : %s <IP>\n", argv[0]);
        exit(1);
    }

    // 1. 소켓 생성 및 연결
    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect() error");
        exit(1);
    }
    
    printf("Connected to Chat Server. Start typing messages!\n");

    // 2. select 설정 (표준입력 0 + 서버소켓 sock)
    FD_ZERO(&reads);
    FD_SET(0, &reads);    // 0: Standard Input (키보드)
    FD_SET(sock, &reads); // 서버 소켓
    fd_max = sock;

    while (1) {
        copy_reads = reads;
        
        if (select(fd_max + 1, &copy_reads, 0, 0, NULL) == -1)
            break;

        // Case A: 서버로부터 메시지 수신
        if (FD_ISSET(sock, &copy_reads)) {
            str_len = recv(sock, buf, BUF_SIZE - 1, 0);
            if (str_len <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            buf[str_len] = 0;
            printf("%s", buf); // 서버 메시지 출력 (이미 개행 포함됨)
        }

        // Case B: 키보드 입력 발생
        if (FD_ISSET(0, &copy_reads)) {
            if (fgets(buf, BUF_SIZE, stdin) != NULL) {
                // 'q' 입력 시 종료
                if (!strcmp(buf, "q\n") || !strcmp(buf, "Q\n")) {
                    close(sock);
                    exit(0);
                }
                write(sock, buf, strlen(buf)); // 서버로 전송
            }
        }
    }

    close(sock);
    return 0;
}