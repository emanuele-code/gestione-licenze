#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT_PORTALE  9002          
#define SERVER_L_PORT 9000
#define SERVER_L_IP   "127.0.0.1"
#define BUF_SIZE      256
#define MAX_CLIENT    10

// Funzione che inoltra la richiesta al serverL
void chiedi_serverL(const char *msg, char *response) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Errore socket serverL");
        strcpy(response, "ERR\n");
        return;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_L_PORT);
    inet_pton(AF_INET, SERVER_L_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione a serverL fallita");
        strcpy(response, "ERR\n");
        close(sock);
        return;
    }

    send(sock, msg, strlen(msg), 0);

    int n = recv(sock, response, BUF_SIZE-1, 0);
    if (n < 0) n = 0;
    response[n] = '\0';
    close(sock);
}

// Funzione per gestire ogni client in un thread
void* client_handler(void *arg) {
    int sock = *(int*)arg;
    free(arg);

    char buffer[BUF_SIZE];
    int n = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (n <= 0) {
        close(sock);
        return NULL;
    }
    buffer[n] = '\0';

    char response[BUF_SIZE * 10]; // buffer più grande per LISTA
    chiedi_serverL(buffer, response);

    send(sock, response, strlen(response), 0);
    close(sock);
    return NULL;
}


int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creazione socket server portale
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore socket portale");
        exit(EXIT_FAILURE);
    }
    
    // permette di riusare subito una porta TCP anche se in TIME_WAIT
    // SOL_SOCKET   -> indica modifiche generiche
    // SO_REUSEADDR -> indica ip/port riutilizzabile anche se in time_wait
    int libera = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &libera, sizeof(libera)) == -1) {
		perror("setsockopt");
		exit(1);
	}

    // valorizzo struct per il server portale
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT_PORTALE);

    // assegno una socket a ip:port del portale
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Errore bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // metti in ascolto il portale
    if (listen(server_fd, MAX_CLIENT) < 0) {
        perror("Errore listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Portale in ascolto sulla porta %d...\n", PORT_PORTALE);

    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Errore accept");
            continue;
        }

        int *pclient = malloc(sizeof(int));
        if (!pclient) {
            perror("Errore malloc");
            close(new_socket);
            continue;
        }
        *pclient = new_socket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, pclient) != 0) {
            perror("Errore pthread_create");
            close(new_socket);
            free(pclient);
            continue;
        }
        pthread_detach(tid); // rilascio automatico risorse thread
    }

    close(server_fd);
    return 0;
}

