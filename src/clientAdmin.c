#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define SERVER_G_PORT     9001
#define PORT_PORTALE      9002
#define SERVER_IP_PORTALE "127.0.0.1"
#define SERVER_G_IP       "127.0.0.1"
#define BUF_SIZE          256




void modifica_licenza() {
    int scelta;
    char id[50];
    char stato[10];
    char scadenza[20];
    char buffer[BUF_SIZE];

    printf("=== Modifica Licenza Admin ===\n");
    printf("Inserisci ID licenza da modificare: ");
    scanf("%s", id);

    printf("Scegli operazione:\n1. Invalida\n2. Ripristina\nScelta: ");
    scanf("%d", &scelta);

    if (scelta == 1) {
    	strcpy(stato, "invalida");
    } else if (scelta == 2){
    	strcpy(stato, "valida");
    	time_t t = time(NULL);
		struct tm scad_tm = *localtime(&t);
		scad_tm.tm_year += 1;
		strftime(scadenza, sizeof(scadenza), "%Y-%m-%d", &scad_tm);
    } else {
        printf("Scelta non valida.\n");
        return;
    }

    snprintf(buffer, sizeof(buffer), "UPDATE,%s,%s,%s", id, stato, scadenza);

    // creo la socket tcp per il client
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Errore socket");
        return;
    }

    // valorizzoi dati del server portale da contattare per la modifica
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT_PORTALE);
    inet_pton(AF_INET, SERVER_IP_PORTALE, &serv_addr.sin_addr);

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
        printf("Risposta portale: %s\n", response);
    }

    close(sock);
}




void mostra_licenze() {
    char buffer[BUF_SIZE*10]; // buffer più grande per molte licenze
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Errore socket");
        return;
    }

    // valorizzo ip:port del serverG
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_G_PORT);
    inet_pton(AF_INET, SERVER_G_IP, &serv_addr.sin_addr);

    // connetto il client al server G
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione fallita");
        close(sock);
        return;
    }

    // invia il comando LISTA
    if (send(sock, "LISTA", 5, 0) < 0) {
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

    char copia[BUF_SIZE*10];
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
        printf("\n=== Menu Client Admin ===\n");
        printf("1. Modifica licenza (invalida/ripristina)\n");
        printf("2. Mostra licenze\n");
        printf("0. Esci\n");
        printf("Scelta: ");
        scanf("%d", &scelta);

        switch(scelta) {
            case 1:
                modifica_licenza();
                break;
            case 2:
                mostra_licenze();
                break;
            case 0:
                printf("Uscita dal clientAdmin.\n");
                break;
            default:
                printf("Scelta non valida.\n");
        }
    } while (scelta != 0);

    return 0;
}

