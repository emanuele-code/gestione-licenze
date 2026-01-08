#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define FILE_LICENZE "../licenze.csv"
#define PORT         9000
#define MAX_CLIENT   10
#define BUF_SIZE     256


pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct {
    char id[50];
    char stato[10];
    char scadenza[20];
} Licenza;


void salva_licenza(const char *id, const char *stato, const char *scadenza) {
    pthread_mutex_lock(&file_mutex);

    // Apre il file licenze in lettura/scrittura; se non esiste, prova a crearlo
    FILE *fp = fopen(FILE_LICENZE, "r+");
    if (!fp)
        fp = fopen(FILE_LICENZE, "w");

    // Se anche la creazione fallisce, esce
    if (!fp) {
        perror("Errore apertura file licenze");
        pthread_mutex_unlock(&file_mutex);
        return;
    }

    char line[BUF_SIZE];

    // Flag per capire se l'ID è stato trovato
    int found = 0;

    // File temporaneo dove scrivere i dati aggiornati
    FILE *temp = fopen("temp.csv", "w");
    if (!temp) {
        perror("Errore creazione file temporaneo");
        fclose(fp);
        pthread_mutex_unlock(&file_mutex);
        return;
    }

    // Legge il file originale riga per riga
    while (fgets(line, sizeof(line), fp)) {
        char lid[50], lstato[10], lscad[20];

        // Estrae i campi da line: id, stato, scadenza
        sscanf(line, "%49[^,],%9[^,],%19[^\n]", lid, lstato, lscad);

        if (strcmp(lid, id) == 0) {
            // Scrive la riga aggiornata nel file temporaneo
            fprintf(temp, "%s,%s,%s\n", id, stato, scadenza);
            found = 1; 
        } else {
            // Altrimenti copia la riga originale
            fprintf(temp, "%s", line);
        }
    }

    // Se l'ID non era presente nel file, aggiunge una nuova riga
    if (!found)
        fprintf(temp, "%s,%s,%s\n", id, stato, scadenza);

    fclose(fp);
    fclose(temp);

    if (remove(FILE_LICENZE) != 0)
        perror("Errore rimozione file licenze originale");

    if (rename("temp.csv", FILE_LICENZE) != 0)
        perror("Errore rinomina file temporaneo");

    pthread_mutex_unlock(&file_mutex);
}


int licenza_esiste(const char *id) {
    pthread_mutex_lock(&file_mutex);
    FILE *fp = fopen(FILE_LICENZE, "r");
    if (!fp) {
        pthread_mutex_unlock(&file_mutex);
        return 0;
    }

    char line[BUF_SIZE];
    char lid[50];
    while (fgets(line, sizeof(line), fp)) {
        // legge solo l'id fino alla prima virgola
        sscanf(line, "%49[^,]", lid);
        if (strcmp(lid, id) == 0) {
            fclose(fp);
            pthread_mutex_unlock(&file_mutex);
            return 1;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&file_mutex);
    return 0;
}


int licenza_valida(const char *id) {
    pthread_mutex_lock(&file_mutex);
    FILE *fp = fopen(FILE_LICENZE, "r");
    if (!fp) {
        pthread_mutex_unlock(&file_mutex);
        return 0;
    }

    char line[BUF_SIZE];
    int valido = 0;
    while (fgets(line, sizeof(line), fp)) {
        char lid[50], lstato[10], lscad[20];
        sscanf(line, "%49[^,],%9[^,],%19[^\n]", lid, lstato, lscad);
        if (strcmp(lid, id) == 0) {
            if (strcmp(lstato, "valida") == 0) valido = 1;
            break;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&file_mutex);
    return valido;
}


// Funzione eseguita da un thread per gestire un singolo client
void* client_handler(void *arg) {
    // arg è un puntatore a un intero che contiene il socket del client
    int sock = *(int*)arg;   
    free(arg);            

    char buffer[BUF_SIZE];
    int n = recv(sock, buffer, sizeof(buffer)-1, 0); // riceve dati dal client

    if (n <= 0) {                         // errore <0 o connessione chiusa =0
        if (n < 0) perror("Errore recv"); // stampa errore se recv fallisce
        close(sock);                      // chiude socket
        return NULL;                      // termina thread
    }

    buffer[n] = '\0';         

    // Variabili per parsare il comando e i dati della licenza
    char comando[20] = {0}, id[50] = {0}, stato[10] = {0}, scadenza[20] = {0};
    sscanf(buffer, "%19[^,],%49[^,],%9[^,],%19[^\n]", comando, id, stato, scadenza);

    if (strcmp(comando, "REGISTRA") == 0) { // se l'id esiste già da errore altrimenti salva la licenza
        if (licenza_esiste(id)) {
            send(sock, "ERRORE: ID già registrato\n", 27, 0);
        } else {
            salva_licenza(id, stato, scadenza);
            send(sock, "OK\n", 3, 0);
        }
    }
    else if (strcmp(comando, "UPDATE") == 0) { // se la licenza non esiste da errore altrimenti aggiorna la licenza
        if (!licenza_esiste(id)) {
            send(sock, "ERRORE: ID non esistente\n", 26, 0);
        } else {
            salva_licenza(id, stato, scadenza);
            send(sock, "OK\n", 3, 0);
        }
    }
    else if (strcmp(comando, "CHECK") == 0) { // se la licenza non esiste da errore altrimenti controlla la licenza
        if (!licenza_esiste(id)) {
            send(sock, "ERRORE: ID non esistente\n", 26, 0);
        } else {
            int val = licenza_valida(id);
            if (send(sock, val ? "VALIDA\n" : "INVALIDA\n", val ? 7 : 9, 0) < 0)
                perror("Errore send");
        }        
    }
    else if (strcmp(comando, "LISTA") == 0) { // stampa tutte le licenze
        pthread_mutex_lock(&file_mutex);      
        FILE *fp = fopen(FILE_LICENZE, "r");  
        if (!fp) {                             
            pthread_mutex_unlock(&file_mutex);
            send(sock, "Errore apertura file\n", 21, 0);
        } else {
            char line[BUF_SIZE];
            char risposta[BUF_SIZE * MAX_CLIENT] = ""; // buffer per tutta la lista
            while (fgets(line, sizeof(line), fp)) {
                strcat(risposta, line); 
            }
            fclose(fp);
            pthread_mutex_unlock(&file_mutex); 
            send(sock, risposta, strlen(risposta), 0); 
        }
    }
    else {
        // Comando sbagliato
        if (send(sock, "ERR - comando errato\n", 22, 0) < 0)
            perror("Errore send");
    }

    close(sock); // chiude il socket del client
    return NULL; // termina il thread
}




int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore creazione socket");
        exit(EXIT_FAILURE);
    }
    
    // non fa fallire la bind sulla porta anche se in TIME_WAIT 
    // SO_REUSEADDR -> riusa subito l'indirizzo anche se in TIME_WAIT
    // SOL_SOCKET   -> indica una modifica generica del socket e non specifica tipo porta, ip ecc
    int libera = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &libera, sizeof(libera)) == -1) {
		perror("setsockopt");
		exit(1);
	}

    // valorizzazione campi per il server
    // INADDR_ANY accetta connessioni su qualsiasi indirizzo disponibile (loopback, ethernet)
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    // la bind collega il socket creato al IP:PORT definiti in modo che può ricevere dati
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Errore bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // trasformare il socket generico nel socket server pronto a ricevere connessioni TCP client
    if (listen(server_fd, MAX_CLIENT) < 0) {
        perror("Errore listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server L in ascolto sulla porta %d...\n", PORT);

    while (1) {
        // dopo 3-way, preleva una connessione dalla lista di backlog e crea una socket dedicata per il client
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
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
        *pclient = new_socket; // assegna il socket

        // crea il thread e poi fa il detach
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, pclient) != 0) {
            perror("Errore pthread_create");
            close(new_socket);
            free(pclient);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

