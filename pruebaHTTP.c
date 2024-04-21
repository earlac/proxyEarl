#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
    char ip[50];
    char port[10];
} ServerConfig;

#include "cJSON.h"

void find_ip_for_hostname_and_path(cJSON *config_array, char *hostname, char *path, char *ip);

void handle_http_request(int client_socket, ServerConfig *configs, int num_configs) {
    char buffer[1024];
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Received request on socket %d:\n%s\n", client_socket, buffer);

        // Select a random configuration
        int random_index = rand() % num_configs;
        ServerConfig selected_config = configs[random_index];

        // Build command to execute test6
        char command[256];
        snprintf(command, sizeof(command), "./test6 %s %s /index.html", selected_config.ip, selected_config.port);

        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("Failed to run command");
            close(client_socket);
            return;
        }

        // Read the output of test6 and send as HTTP response
        char response_buffer[4096];
        char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        write(client_socket, response_header, strlen(response_header));

        while (fgets(response_buffer, sizeof(response_buffer), fp) != NULL) {
            write(client_socket, response_buffer, strlen(response_buffer));
        }

        pclose(fp);
    }

    close(client_socket);
}

void start_listening(int port, ServerConfig *configs, int num_configs) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", port);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        handle_http_request(client_socket, configs, num_configs);
    }
}

void start_http_listening(int port, cJSON *config_array) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("HTTP server listening on port %d\n", port);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        char buffer[4096];
        int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            char method[16], path[1024] = "";
            sscanf(buffer, "%s %s", method, path);

            char hostname[256] = "";
            char *hostHeaderStart = strstr(buffer, "Host: ");
            if (hostHeaderStart) {
                hostHeaderStart += 6; // Skip "Host: "
                char *hostHeaderEnd = strstr(hostHeaderStart, "\r\n");
                if (hostHeaderEnd) {
                    int hostnameLength = hostHeaderEnd - hostHeaderStart;
                    strncpy(hostname, hostHeaderStart, hostnameLength);
                    hostname[hostnameLength] = '\0';

                    char ip[50] = "";
                    find_ip_for_hostname_and_path(config_array, hostname, path, ip);
                    if (ip[0] != '\0') {
                        char command[256];
                        snprintf(command, sizeof(command), "./test6 %s 80 /index.html", ip);
                        FILE *fp = popen(command, "r");
                        if (fp == NULL) {
                            perror("Failed to run command");
                            close(client_socket);
                            continue;
                        }

                        // Read the output of test6 and send as HTTP response
                        char response_buffer[4096];
                        char *response_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                        write(client_socket, response_header, strlen(response_header));

                        while (fgets(response_buffer, sizeof(response_buffer), fp) != NULL) {
                            write(client_socket, response_buffer, strlen(response_buffer));
                        }

                        pclose(fp);
                    } else {
                        char *error_message = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNo matching IP found.";
                        write(client_socket, error_message, strlen(error_message));
                    }
                }
            }
        }
        close(client_socket);
    }
}

void find_ip_for_hostname_and_path(cJSON *config_array, char *hostname, char *path, char *ip) {
    cJSON *config_entry = cJSON_GetObjectItem(config_array, hostname);
    if (!config_entry) {
        printf("No configuration found for hostname: %s\n", hostname);
        ip[0] = '\0'; // Indicar que no se encontró configuración.
        return;
    }

    cJSON *default_config = NULL;
    int config_count = cJSON_GetArraySize(config_entry);
    for (int i = 0; i < config_count; i++) {
        cJSON *entry = cJSON_GetArrayItem(config_entry, i);
        cJSON *json_ip = cJSON_GetObjectItem(entry, "ip");
        cJSON *json_path = cJSON_GetObjectItem(entry, "path");

        if (!json_ip) {
            printf("Invalid entry: IP missing in configuration for %s.\n", hostname);
            continue;
        }

        // Considerar este como default si no se especifica un path.
        if (!json_path || strcmp(json_path->valuestring, "/") == 0) {
            default_config = entry;  // Usar esta entrada como default si no hay match.
        }

        // Verificar match exacto o comodín.
        if (json_path && (strcmp(json_path->valuestring, path) == 0 || strcmp(json_path->valuestring, "/*") == 0)) {
            strcpy(ip, json_ip->valuestring);
            printf("IP Match found: '%s' for hostname+path: %s%s\n", ip, hostname, path);
            return; // Retornar tan pronto como se encuentra una coincidencia.
        }
    }

    // Usar la configuración predeterminada si no se encontró una coincidencia exacta.
    if (default_config) {
        cJSON *default_ip = cJSON_GetObjectItem(default_config, "ip");
        strcpy(ip, default_ip->valuestring);
        printf("Default IP Match found: '%s' for hostname: %s with default path\n", ip, hostname);
    } else {
        printf("No matching IP found for %s%s\n", hostname, path);
        ip[0] = '\0'; // Asegurarse que la cadena de IP está vacía si no se encuentra configuración.
    }
}


void parse_json_and_start_servers(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Cannot open file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = malloc(len + 1);
    fread(data, 1, len, file);
    data[len] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(data);
    if (!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(data);
        return;
    }

    cJSON *mode = cJSON_GetObjectItemCaseSensitive(json, "mode");
    if (cJSON_IsString(mode)) {
        if (strcmp(mode->valuestring, "tcp") == 0) {
            cJSON *tcp = cJSON_GetObjectItemCaseSensitive(json, "tcp");
            cJSON *ports = cJSON_GetObjectItemCaseSensitive(tcp, "ports");
            cJSON *port_key;
            cJSON_ArrayForEach(port_key, ports) {
                int listening_port = atoi(port_key->string);
                cJSON *config_array = cJSON_GetObjectItemCaseSensitive(ports, port_key->string);
                int num_configs = cJSON_GetArraySize(config_array);
                ServerConfig *configs = malloc(num_configs * sizeof(ServerConfig));

                for (int i = 0; i < num_configs; i++) {
                    cJSON *config = cJSON_GetArrayItem(config_array, i);
                    strcpy(configs[i].ip, cJSON_GetObjectItem(config, "ip")->valuestring);
                    strcpy(configs[i].port, cJSON_GetObjectItem(config, "port")->valuestring);
                }

                if (fork() == 0) { // Create a child process for each listening port
                    start_listening(listening_port, configs, num_configs);
                    free(configs);
                    exit(0);
                } else {
                    free(configs); // Parent frees memory
                }
            }
        } else if (strcmp(mode->valuestring, "http") == 0) {
            cJSON *http = cJSON_GetObjectItemCaseSensitive(json, "http");
            cJSON *ports = cJSON_GetObjectItemCaseSensitive(http, "ports");
            cJSON *port_key;
            cJSON_ArrayForEach(port_key, ports) {
                int listening_port = atoi(port_key->string);
                cJSON *config_array = cJSON_GetObjectItemCaseSensitive(ports, port_key->string);

                if (fork() == 0) { // Create a child process for each listening HTTP port
                    start_http_listening(listening_port, config_array);
                    exit(0);
                }
            }
        }
    } else {
        printf("Mode is not recognized, no sockets will be opened.\n");
    }

    cJSON_Delete(json);
    free(data);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));  // Seed the random number generator

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    parse_json_and_start_servers(argv[1]);
    while (1) sleep(1); // Keep the main process running
    return 0;
}
