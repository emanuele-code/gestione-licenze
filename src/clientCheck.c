#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_G_PORT 9001
#define SERVER_G_IP   "127.0.0.1"
#define BUF_SIZE      256
#define MAX_CLIENT    10


void controlla_licenza() {
    char id[50];
    char buffer[BUF_SIZE];

    printf("=== Controllo Licenza ===\n");
    printf("Inserisci ID licenza da verificare: ");
    scanf("%s", id);

    // prepara il messaggio da inviare al server G
    snprintf(buffer, sizeof(buffer), "CHECK,%s", id);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Errore socket");
        return;
    }

    // valorizzo ip:Port del server G a cui fare richiesta
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_G_PORT);
    inet_pton(AF_INET, SERVER_G_IP, &serv_addr.sin_addr);

    // connetto il client al server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione fallita");
        close(sock);
        return;
    }

    send(sock, buffer, strlen(buffer), 0);

    char response[BUF_SIZE];
    int n = recv(sock, response, sizeof(response)-1, 0);
    if (n < 0) {
        perror("Errore ricezione");
    } else {
        response[n] = '\0';
        printf("Stato licenza: %s\n", response);
    }

    close(sock);
}





void mostra_licenze() {
    char buffer[BUF_SIZE*MAX_CLIENT]; // buffer più grande per molte licenze
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Errore socket");
        return;
    }

    // valorizzo struct ip:port del server
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_G_PORT);
    inet_pton(AF_INET, SERVER_G_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione fallita");
        close(sock);
        return;
    }

    // invia il comando LISTA
    if (send(sock, "LISTA", strlen("LISTA"), 0) < 0) {
        perror("Errore invio dati");
        close(sock);
        return;
    }

    // riceve le licenze
    size_t tot = 0;
    int n;
    while ((n = recv(sock, buffer + tot, sizeof(buffer) - tot - 1, 0)) > 0) {
        tot += n;
        if (tot >= sizeof(buffer)-1) break; // evita overflow
    }
    if (n < 0) perror("Errore ricezione");

    buffer[tot] = '\0';

    // stampa in formato tabella
    printf("+----------------+----------+------------+\n");
    printf("| ID             | Validità | Scadenza   |\n");
    printf("+----------------+----------+------------+\n");

    char copia[BUF_SIZE*MAX_CLIENT];
    strncpy(copia, buffer, sizeof(copia)-1);
    copia[sizeof(copia)-1] = '\0';

    char *riga = strtok(copia, "\n");
    while (riga != NULL) {
        char id[50], validita[10], scadenza[20];
        sscanf(riga, "%49[^,],%9[^,],%19[^\n]", id, validita, scadenza);
        printf("| %-14s | %-8s | %-10s |\n", id, validita, scadenza);
        riga = strtok(NULL, "\n");
    }

    printf("+----------------+----------+------------+\n");

    close(sock);
}





int main() {
    int scelta;

    do {
        printf("\n=== Menu Client Check ===\n");
        printf("1. Controlla licenza\n");
        printf("2. Stampa licenze\n");
        printf("0. Esci\n");
        printf("Scelta: ");
        scanf("%d", &scelta);

        switch(scelta) {
            case 1:
                controlla_licenza();
                break;
            case 2:
                mostra_licenze();
                break;
            case 0:
                printf("Uscita dal clientCheck.\n");
                break;
            default:
                printf("Scelta non valida.\n");
        }
    } while (scelta != 0);

    return 0;
}

