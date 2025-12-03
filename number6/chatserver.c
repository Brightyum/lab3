#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 3490
#define BUF_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    
    fd_set reads, copy_reads;
    int fd_max, fd_num, i, j, str_len;
    char buf[BUF_SIZE];
    char message[BUF_SIZE];

    // 1. 소켓 생성 및 설정
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() error");
        exit(1);
    }
    if (listen(server_sock, 5) == -1) {
        perror("listen() error");
        exit(1);
    }

    // 2. select 설정
    FD_ZERO(&reads);
    FD_SET(server_sock, &reads); // 서버 소켓(리스닝) 등록
    fd_max = server_sock;

    printf("Chat Server Started on Port %d...\n", PORT);

    while (1) {
        copy_reads = reads; // 원본 유지

        // 타임아웃 없이 무한 대기 (NULL)
        if ((fd_num = select(fd_max + 1, &copy_reads, 0, 0, NULL)) == -1)
            break;

        if (fd_num == 0) continue;

        // 3. 변화가 생긴 소켓 확인
        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &copy_reads)) {
                
                // Case A: 새로운 연결 요청 (리스닝 소켓)
                if (i == server_sock) {
                    addr_size = sizeof(client_addr);
                    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
                    
                    FD_SET(client_sock, &reads); // 관찰 대상에 추가
                    if (fd_max < client_sock) fd_max = client_sock;
                    
                    printf("Connected client: %d\n", client_sock);
                }
                // Case B: 데이터 수신 (클라이언트 소켓)
                else {
                    str_len = recv(i, buf, BUF_SIZE, 0);
                    
                    // 연결 종료
                    if (str_len <= 0) {
                        FD_CLR(i, &reads); // 관찰 대상에서 제거
                        close(i);
                        printf("Closed client: %d\n", i);
                    } 
                    // 메시지 브로드캐스팅 (핵심 로직)
                    else {
                        buf[str_len] = 0;
                        // 보낸 사람을 포함하여 "Client N: 메시지" 형태로 포맷팅
                        sprintf(message, "[Client %d]: %s", i, buf);
                        printf("%s", message); // 서버 로그 출력

                        // 연결된 모든 클라이언트에게 전송 (나 자신 제외)
                        for (j = 0; j <= fd_max; j++) {
                            // 리스닝 소켓(server_sock)과 보낸 사람(i)은 제외
                            if (FD_ISSET(j, &reads) && j != server_sock && j != i) {
                                write(j, message, strlen(message));
                            }
                        }
                    }
                }
            }
        }
    }
    close(server_sock);
    return 0;
}