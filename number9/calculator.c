#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // for parsing

// 상태 관리를 위한 전역 변수
int has_operator = 0; // 0: 연산자 없음, 1: 연산자 이미 입력됨 (중복 방지용)

GtkWidget *display_entry; // 결과를 보여줄 화면

void quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

// 숫자 버튼 클릭: 화면에 숫자 이어 붙이기
void on_number_clicked(GtkWidget *widget, gpointer data) {
    const char *number_str = (const char *)data;
    const char *current_text = gtk_entry_get_text(GTK_ENTRY(display_entry));
    char new_text[100];

    // 초기 상태가 "0"이면 지우고 입력, 아니면 뒤에 붙임
    if (strcmp(current_text, "0") == 0) {
        gtk_entry_set_text(GTK_ENTRY(display_entry), number_str);
    } else {
        sprintf(new_text, "%s%s", current_text, number_str);
        gtk_entry_set_text(GTK_ENTRY(display_entry), new_text);
    }
}

// 연산자 버튼 클릭: 연산자가 없을 때만 화면에 이어 붙이기
void on_operator_clicked(GtkWidget *widget, gpointer data) {
    // 요구사항: 사칙연산 수가 1개 이상이면 버튼 클릭 안 되게 함
    if (has_operator) {
        return; // 이미 연산자가 있으면 무시 (함수 종료)
    }

    const char *op_str = (const char *)data;
    const char *current_text = gtk_entry_get_text(GTK_ENTRY(display_entry));
    char new_text[100];

    // 현재 화면 뒤에 연산자 붙임 (예: "12" -> "12+")
    sprintf(new_text, "%s%s", current_text, op_str);
    gtk_entry_set_text(GTK_ENTRY(display_entry), new_text);

    has_operator = 1; // 연산자가 입력되었음을 표시
}

// 초기화(C) 버튼: 모든 상태 초기화
void on_clear_clicked(GtkWidget *widget, gpointer data) {
    has_operator = 0;
    gtk_entry_set_text(GTK_ENTRY(display_entry), "0");
}

// 결과(=) 버튼: 문자열을 파싱하여 계산
void on_equals_clicked(GtkWidget *widget, gpointer data) {
    const char *current_text = gtk_entry_get_text(GTK_ENTRY(display_entry));
    double num1 = 0, num2 = 0, result = 0;
    char op = 0;
    char result_str[100];

    // sscanf를 사용하여 "숫자 연산자 숫자" 형식 파싱
    // 예: "12+3" -> num1=12, op='+', num2=3
    // %lf: double, %c: char
    if (sscanf(current_text, "%lf%c%lf", &num1, &op, &num2) < 3) {
        // 파싱 실패 (연산자가 없거나 숫자가 부족한 경우)
        return; 
    }

    switch (op) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/': 
            if (num2 != 0) result = num1 / num2;
            else result = 0; // 0 나누기 방지
            break;
        default: return;
    }

    // 결과 출력 (정수/실수 구분)
    if (result == (int)result)
        sprintf(result_str, "%d", (int)result);
    else
        sprintf(result_str, "%.2f", result);

    gtk_entry_set_text(GTK_ENTRY(display_entry), result_str);

    // 계산이 끝났으므로 연산자 상태 초기화 (결과값에 다시 연산 가능)
    has_operator = 0; 
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    
    // 버튼 배열
    const char *buttons[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "String Calculator");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 250, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(quit), NULL);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // 입력창 설정
    display_entry = gtk_entry_new();
    gtk_entry_set_alignment(GTK_ENTRY(display_entry), 1.0);
    gtk_editable_set_editable(GTK_EDITABLE(display_entry), FALSE);
    gtk_entry_set_text(GTK_ENTRY(display_entry), "0");
    gtk_grid_attach(GTK_GRID(grid), display_entry, 0, 0, 4, 1);

    // 버튼 생성 및 배치
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            button = gtk_button_new_with_label(buttons[y][x]);
            gtk_widget_set_hexpand(button, TRUE);
            gtk_widget_set_vexpand(button, TRUE);

            const char *label = buttons[y][x];

            // 시그널 연결 분기
            if (strcmp(label, "C") == 0) {
                g_signal_connect(button, "clicked", G_CALLBACK(on_clear_clicked), NULL);
            } else if (strcmp(label, "=") == 0) {
                g_signal_connect(button, "clicked", G_CALLBACK(on_equals_clicked), NULL);
            } else if (label[0] >= '0' && label[0] <= '9') {
                // 숫자 버튼
                g_signal_connect(button, "clicked", G_CALLBACK(on_number_clicked), (gpointer)label);
            } else {
                // 연산자 버튼 (+, -, *, /)
                g_signal_connect(button, "clicked", G_CALLBACK(on_operator_clicked), (gpointer)label);
            }

            gtk_grid_attach(GTK_GRID(grid), button, x, y + 1, 1, 1);
        }
    }

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}