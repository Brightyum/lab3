// cal.c -> 컴파일해서 'cal.cgi'로 만드세요. (gcc cal.c -o cal.cgi)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char *method = getenv("REQUEST_METHOD");
    char *query = getenv("QUERY_STRING");
    char *len_str = getenv("CONTENT_LENGTH");
    char content[1024] = {0};

    // CGI는 Content-Type을 먼저 출력해야 함
    printf("Content-Type: text/html\n\n");
    printf("<html><body>");
    printf("<h1>CGI Result</h1>");
    printf("<p>Method: %s</p>", method);

    if (method && strcmp(method, "GET") == 0 && query) {
        printf("<p>Query String: %s</p>", query);
    } 
    else if (method && strcmp(method, "POST") == 0 && len_str) {
        int len = atoi(len_str);
        // 표준 입력(stdin)에서 Body 읽기
        fread(content, 1, len, stdin);
        printf("<p>Post Body: %s</p>", content);
    }
    
    printf("</body></html>");
    return 0;
}