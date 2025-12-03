#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 3490
#define QLEN 10
#define BUF_SIZE 1024

void handle_clnt(int sockfd);
void send_err(int sockfd, char *msg);
void send_static(int sockfd, char *filename);
void execute_cgi(int sockfd, char *path, char *method, char *query_string, char *body);

int main(int argc, char *argv[])
{
    int sockfd, new_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t alen;
    fd_set readfds, activefds;
    int i, maxfd = 0;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        exit(1);
    }

    int opt = 1; // 재실행 시 bind 에러 방지
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(1);
    }

    if (listen(sockfd, QLEN) < 0) {
        fprintf(stderr, "listen failed\n");
        exit(1);
    }

    alen = sizeof(client_addr);
    FD_ZERO(&activefds);
    FD_SET(sockfd, &activefds);
    maxfd = sockfd;

    fprintf(stderr, "Server up and running on port %d.\n", PORT);

    while (1) {
        readfds = activefds;
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        for (i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == sockfd) {
                    if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &alen)) < 0) {
                        fprintf(stderr, "accept failed\n");
                    } else {
                        FD_SET(new_fd, &activefds);
                        if (new_fd > maxfd) maxfd = new_fd;
                        printf("New connection: %d\n", new_fd);
                    }
                } else {
                    handle_clnt(i);
                    close(i); // HTTP 1.0: 요청 처리 후 연결 종료
                    FD_CLR(i, &activefds);
                }
            }
        }
    }
    close(sockfd);
    return 0;
}

void handle_clnt(int client_sock)
{
    char buf[BUF_SIZE];
    char method[10], url[100], protocol[10];
    char *query_string = NULL;
    char *body = NULL;
    char path[100];
    int str_len;

    // 1. 요청 읽기
    if ((str_len = read(client_sock, buf, BUF_SIZE - 1)) <= 0) {
        return; 
    }
    buf[str_len] = 0;

    // 2. Body 분리 (Double CRLF 찾기)
    char *body_ptr = strstr(buf, "\r\n\r\n");
    if (body_ptr) {
        *body_ptr = 0; // 헤더와 바디 분리
        body = body_ptr + 4; // 바디 시작점
    }

    // 3. 헤더 파싱 (첫 줄만)
    sscanf(buf, "%s %s %s", method, url, protocol);
    printf("[%s] Request: %s %s\n", method, url);

    // 4. URL에서 파일 경로와 Query String 분리
    // 예: /test.cgi?name=kim
    strcpy(path, url);
    char *q_ptr = strchr(path, '?');
    if (q_ptr) {
        *q_ptr = 0; // '?'를 NULL로 변경하여 path 분리
        query_string = q_ptr + 1;
    }

    // 파일 이름이 없으면 index.html로 가정
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }

    // 5. CGI 실행 여부 판단 (확장자가 .cgi 이거나 bin 디렉토리 등)
    // 여기서는 단순하게 파일명에 ".cgi"가 포함되어 있으면 CGI로 처리
    if (strstr(path, ".cgi") != NULL) {
        execute_cgi(client_sock, path + 1, method, query_string, body); // path+1은 '/' 제거
    } 
    else if (strcmp(method, "GET") == 0) {
        // 일반 정적 파일 처리
        send_static(client_sock, path + 1);
    } 
    else {
        send_err(client_sock, "Bad Request or Method Not Allowed");
    }
}

void execute_cgi(int sockfd, char *path, char *method, char *query_string, char *body) {
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    int pid;
    int stdin_pipe[2];

    // 실행 파일 존재 확인
    if (access(path, X_OK) != 0) {
        send_err(sockfd, "CGI Script Not Found or Not Executable");
        return;
    }

    // HTTP 헤더 전송 (CGI가 Content-Type은 출력한다고 가정)
    write(sockfd, protocol, strlen(protocol));

    // POST 데이터를 위한 파이프 생성
    if (pipe(stdin_pipe) < 0) {
        perror("pipe error");
        return;
    }

    if ((pid = fork()) < 0) {
        perror("fork error");
        return;
    }

    if (pid == 0) { // 자식 프로세스 (CGI 실행)
        char meth_env[20], query_env[200], len_env[50];
        
        // 1. 소켓을 표준 출력으로 연결
        dup2(sockfd, STDOUT_FILENO);

        // 2. 파이프를 표준 입력으로 연결 (POST 데이터 수신용)
        close(stdin_pipe[1]); // 쓰기용 닫기
        dup2(stdin_pipe[0], STDIN_FILENO);

        // 3. 환경 변수 설정
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(strdup(meth_env));

        if (query_string) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(strdup(query_env));
        }

        if (body) {
            sprintf(len_env, "CONTENT_LENGTH=%ld", strlen(body));
            putenv(strdup(len_env));
        }

        // 4. CGI 실행
        execl(path, path, NULL);
        exit(1); // execl 실패 시
    } 
    else { // 부모 프로세스
        close(stdin_pipe[0]); // 읽기용 닫기
        
        // POST 데이터가 있다면 자식의 stdin으로 전송
        if (body && strcmp(method, "POST") == 0) {
            write(stdin_pipe[1], body, strlen(body));
        }
        close(stdin_pipe[1]); // EOF 전송

        wait(NULL); // 자식 종료 대기
    }
}

void send_static(int sockfd, char *filename) {
    FILE *fp = fopen(filename, "rb");
    char buf[BUF_SIZE];
    int n;

    if (!fp) {
        send_err(sockfd, "404 Not Found");
        return;
    }

    char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    write(sockfd, header, strlen(header));

    while ((n = fread(buf, 1, BUF_SIZE, fp)) > 0) {
        write(sockfd, buf, n);
    }
    fclose(fp);
}

void send_err(int client_sock, char *msg)
{
    char buf[BUF_SIZE];
    sprintf(buf, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"
                 "<html><body><h1>%s</h1></body></html>", msg);
    write(client_sock, buf, strlen(buf));
}