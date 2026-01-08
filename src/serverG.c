#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>


#define SERVER_L_IP   "127.0.0.1"
#define PORT_G_PORT   9001           
#define SERVER_L_PORT 9000  
#define MAX_CLIENT    10
#define BUF_SIZE      256

// Funzione che contatta Server L per controllare una licenza
int chiedi_serverL(const char *id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // configura indirizzo del Server L
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_L_PORT);           // conversione Big endian porta SERVER_L
    inet_pton(AF_INET, SERVER_L_IP, &serv_addr.sin_addr);  // IP Server L -> conversione da testuale a binario

    // Connessione al Server L
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Errore connessione serverL");
        close(sock);
        return 0; // fallimento
    }

    // Invio comando CHECK al Server L
    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "CHECK,%s,,,", id); // check per overflow
    send(sock, msg, strlen(msg), 0);

    // Ricezione risposta dal Server L
    char buffer[BUF_SIZE];
    int n = recv(sock, buffer, sizeof(buffer)-1, 0);
    buffer[n] = '\0';
    close(sock);

    // Restituisce 1 se licenza valida, 0 altrimenti
    if (strncmp(buffer, "VALIDA", 6) == 0) return 1;
    return 0;
}

// Funzione eseguita dal thread per ogni client
void* client_handler(void *arg) {
    int sock = *(int*)arg; 
    free(arg);             

    char buffer[BUF_SIZE];
    int n = recv(sock, buffer, sizeof(buffer)-1, 0); // riceve richiesta client
    if (n <= 0) { 
        if (n < 0) perror("Errore recv"); 
        close(sock); 
        return NULL; 
    }
    buffer[n] = '\0';

    // Legge comando e ID dalla richiesta
    char comando[20], id[50];
    sscanf(buffer, "%19[^,],%49[^\n]", comando, id); //legge linea già esistente dal buffer

    // Crea socket verso Server L
    int sockL = socket(AF_INET, SOCK_STREAM, 0);   
    if (sockL < 0) {
        perror("Errore socket verso Server L");
        send(sock, "ERR\n", 4, 0);
        close(sock);
        return NULL;
    }

    // ip:port dell'indirizzo server L a cui rivolgersi
    struct sockaddr_in serv_addrL;
    serv_addrL.sin_family = AF_INET;
    serv_addrL.sin_port   = htons(SERVER_L_PORT);
    inet_pton(AF_INET, SERVER_L_IP, &serv_addrL.sin_addr);

    // Connessione a Server L tramite i dati di sopra
    if (connect(sockL, (struct sockaddr*)&serv_addrL, sizeof(serv_addrL)) < 0) {
        perror("Connessione a Server L fallita");
        send(sock, "ERR\n", 4, 0);
        close(sockL);
        close(sock);
        return NULL;
    }

    // Prepara il messaggio da inoltrare a Server L
    char msgL[BUF_SIZE];
    if (strcmp(comando, "CHECK") == 0) {
        snprintf(msgL, sizeof(msgL), "CHECK,%s,,,", id);
    } else if (strcmp(comando, "LISTA") == 0) {
        snprintf(msgL, sizeof(msgL), "LISTA,,,,");
    } else {
        send(sock, "ERR\n", 4, 0);
        close(sockL);
        close(sock);
        return NULL;
    }

    // Invia il messaggio a Server L
    send(sockL, msgL, strlen(msgL), 0);

    // Riceve la risposta da Server L
    char risposta[BUF_SIZE*MAX_CLIENT] = ""; //più grande perchè ad esempio le richieste sulla lista possono eccedere il buffer
    ssize_t ricevuti;
    while ((ricevuti = recv(sockL, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[ricevuti] = '\0';
        strcat(risposta, buffer);
        if ((size_t)ricevuti < sizeof(buffer)-1) break; // fine dati
    }

    // Invia la risposta al client
    send(sock, risposta, strlen(risposta), 0);

    close(sockL); // chiude socket verso Server L
    close(sock);  // chiude socket verso client
    return NULL;
}

// Funzione principale del server G
int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creazione socket TCP per il server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore socket");
        exit(EXIT_FAILURE);
    }
    
    // Permette di riusare subito la porta anche se in TIME_WAIT
    int libera = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &libera, sizeof(libera)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    // Impostazione indirizzo e porta del server
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // ascolta tutte le interfacce
    address.sin_port        = htons(PORT_G_PORT);

    // Collega socket a porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Errore bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Mette il socket in ascolto
    if (listen(server_fd, MAX_CLIENT) < 0) {
        perror("Errore listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server G in ascolto sulla porta %d...\n", PORT_G_PORT);

    // Loop infinito per accettare client
    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Errore accept");
            continue;
        }

        // Alloca memoria per passare socket al thread
        int *pclient = malloc(sizeof(int));
        if (!pclient) {
            perror("Errore malloc");
            close(new_socket);
            continue;
        }
        *pclient = new_socket;

        // Crea un thread per gestire il client
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, pclient) != 0) {
            perror("Errore pthread_create");
            close(new_socket);
            free(pclient);
            continue;
        }

        pthread_detach(tid); // thread staccato: il sistema libera automaticamente le risorse
    }

    close(server_fd); // chiude socket del server (mai raggiunto in questo loop infinito)
    return 0;
}