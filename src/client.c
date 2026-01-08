#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>


#define SERVER_IP_PORTALE "127.0.0.1"
#define PORT_PORTALE      9002         
#define BUF_SIZE          256


void registra_licenza() {
    char id[50];
    char scadenza[20];
    char buffer[BUF_SIZE];
    char stato[] = "valida";

    printf("=== Registrazione Licenza ===\n");
    printf("Inserisci ID licenza: ");
    scanf("%s", id);
    
    time_t t = time(NULL);
    struct tm scadenza_tm = *localtime(&t);
    scadenza_tm.tm_year += 1;
	strftime(scadenza, sizeof(scadenza), "%Y-%m-%d", &scadenza_tm);
	
	// la uso per evitare di andare in buffer overflow, sarà la stringa inviata al server
    snprintf(buffer, sizeof(buffer), "REGISTRA,%s,%s,%s\n", id, stato, scadenza);

    // ritorno il fd della socket per il client
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { 
    	perror("Errore socket");
    	return;
    }

    // valorizzo ip:port per il server portale da contattare
    struct sockaddr_in serv_addr;   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT_PORTALE); // numero porta in formato binario big-endian
    inet_pton(AF_INET, SERVER_IP_PORTALE, &serv_addr.sin_addr); //ipv4 to address in formato binario per socket


	// connette il client a un server remoto grazie ip:port definiti nella struct sopra
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connessione fallita");
        close(sock);
        return;
    }

	// serve per inviare i dati ad un socket connesso, ritorna i byte effettivamente inviati
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
		perror("Errore invio dati");
		close(sock);
		return;
	}


    char response[BUF_SIZE]; // qui salvo i dati ricevuti dal server
    int n = recv(sock, response, sizeof(response)-1, 0); // ritorna i byte ricevuti da parte del server
    if (n < 0) {
		perror("Errore ricezione");
		close(sock);
		return;
	} else if (n == 0) {
		printf("Server chiuso la connessione.\n");
		close(sock);
		return;
	}
    response[n] = '\0';
    printf("Risposta portale: %s\n", response);

    close(sock);
}





int main() {
    int scelta;

    do {
        printf("\n=== Menu Client ===\n");
        printf("1. Registra licenza\n");
        printf("0. Esci\n");
        printf("Scelta: ");
        scanf("%d", &scelta);

        switch(scelta) {
            case 1:
                registra_licenza();
                break; 
            case 0:
                printf("Uscita dal client.\n");
                break;
            default:
                printf("Scelta non valida.\n");
        }
        
    } while (scelta != 0);

    return 0;
}

