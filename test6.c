#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <IP> <Puerto> <Recurso>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *resource = argv[3];

    // Crear un socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la direcci  n del servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Error al convertir la direcci  n IP");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar al servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Enviar solicitud HTTP GET
    char http_request[1024];
    snprintf(http_request, sizeof(http_request), 
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n", resource, ip);
    send(sockfd, http_request, strlen(http_request), 0);

    // Recibir y mostrar la respuesta del servidor
    char buffer[BUFFER_SIZE];
    int bytes_received;
    printf("Respuesta del servidor:\n");
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Asegurarse de que el texto es una cadena terminada correctamente
        printf("%s", buffer);
    }
    if (bytes_received < 0) {
        perror("Error al recibir datos del servidor");
    }

    // Cerrar el socket
    close(sockfd);

    return 0;
}