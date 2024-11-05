#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void log_message(const char *message) {
    // Логирование сообщений
    printf("[LOG] %s\n", message);
}

int main() {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], server_reply[BUFFER_SIZE];
    char name[50];

    // Создаем сокет
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        log_message("Socket creation error");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0) {
        log_message("Invalid address");
        close(sock);
        return -1;
    }

    // Подключаемся к серверу
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        log_message("Connection error");
        close(sock);
        return -1;
    }

    // Ввод имени
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0'; // Убираем символ новой строки
    send(sock, name, strlen(name), 0);
    log_message("Connection established.");

    printf("Enter message (format 'recipient_name:message'):\n");

    while (1) {
        // Выводим приглашение для ввода сообщения
        printf("You: ");
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0';

        if (strcmp(message, "exit") == 0) {
            log_message("Exiting client.");
            break; // Выход по команде
        }

        send(sock, message, strlen(message), 0);
        log_message("Message sent.");

        // Получаем ответ от сервера
        memset(server_reply, 0, BUFFER_SIZE);
        int read_size = recv(sock, server_reply, BUFFER_SIZE, 0);
        if (read_size > 0) {
            server_reply[read_size] = '\0'; // Завершаем строку
            printf("Server reply: %s\n", server_reply);
        } else {
            log_message("Failed to receive message from server.");
        }
    }

    close(sock);
    return 0;
}
