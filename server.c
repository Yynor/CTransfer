#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    char name[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(const char *message) {
    // Логирование сообщений
    printf("[LOG] %s\n", message);
}

void broadcast_message(const char *sender, const char *recipient, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].name, recipient) == 0) {
            char full_message[BUFFER_SIZE];
            snprintf(full_message, BUFFER_SIZE, "%s: %s", sender, message);
            send(clients[i].socket, full_message, strlen(full_message), 0);

            // Логируем сообщение
            char log_msg[BUFFER_SIZE + 100];
            snprintf(log_msg, sizeof(log_msg), "Message from %s to %s: %s", sender, recipient, message);
            log_message(log_msg);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    char client_name[50];

    // Получаем имя клиента
    recv(client_sock, client_name, sizeof(client_name), 0);
    client_name[strcspn(client_name, "\n")] = '\0'; // Убираем символ новой строки

    // Добавляем клиента в массив
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        clients[client_count].socket = client_sock;
        strcpy(clients[client_count].name, client_name);
        client_count++;
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "Client connected: %s", client_name);
        log_message(log_msg);
    }
    pthread_mutex_unlock(&clients_mutex);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (read_size > 0) {
            // Ожидаем формат: имя_получателя:сообщение
            char *recipient = strtok(buffer, ":");
            char *message = strtok(NULL, ":");
            if (recipient && message) {
                // Убираем символ новой строки из сообщения
                message[strcspn(message, "\n")] = '\0';
                broadcast_message(client_name, recipient, message);
            }
        } else {
            break; // Клиент отключился
        }
    }

    // Удаляем клиента из массива
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client_sock) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            char log_msg[100];
            snprintf(log_msg, sizeof(log_msg), "Client disconnected: %s", client_name);
            log_message(log_msg);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client_sock);
    free(socket_desc);
    return NULL;
}

int main() {
    int server_fd, client_sock, c;
    struct sockaddr_in server, client;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 3);
    log_message("Server running and waiting for connections...");

    c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(server_fd, (struct sockaddr *)&client, (socklen_t *)&c))) {
        pthread_t client_thread;
        int *new_sock = malloc(1);
        *new_sock = client_sock;
        pthread_create(&client_thread, NULL, handle_client, (void *)new_sock);
    }

    if (client_sock < 0) {
        perror("Connection acceptance error");
        close(server_fd);
    }

    return 0;
}
