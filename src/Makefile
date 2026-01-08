# Compilatore e flag
CC = gcc
CFLAGS = -Wall -Wextra -g

# Eseguibili
SERVERS = serverL serverG portale
CLIENTS = client clientAdmin clientCheck

# Target predefinito: solo compilazione
all: $(SERVERS) $(CLIENTS)

# Regole generiche per compilare i file .c
%: %.c
	$(CC) $(CFLAGS) $< -o $@

# Avvia i server in background
run-servers:
	@echo "Avvio dei server..."
	@./serverL & echo $$! > serverL.pid
	@sleep 1
	@./serverG & echo $$! > serverG.pid
	@sleep 1
	@./portale & echo $$! > portale.pid
	@sleep 1
	@echo "Tutti i server avviati."

# Ferma i server
stop-servers:
	@echo "Chiusura server..."
	@-pkill -f serverL || true
	@-pkill -f serverG || true
	@-pkill -f portale || true
	@rm -f *.pid
	@echo "Tutti i server chiusi. Attendo rilascio porte..."
	@sleep 2


# Menu per scegliere quale client avviare
menu-client:
	@while true; do \
		echo ""; \
		echo "Scegli un client:"; \
		echo "1) clientAdmin"; \
		echo "2) client"; \
		echo "3) clientCheck"; \
		echo "0) Esci"; \
		read -p "Numero: " scelta; \
		case $$scelta in \
			1) ./clientAdmin ;; \
			2) ./client ;; \
			3) ./clientCheck ;; \
			0) break ;; \
			*) echo "Scelta non valida." ;; \
		esac; \
	done


# resetta la compilazione
clean:
	rm -f $(SERVERS) $(CLIENTS) *.o *.pid


# Compila e avvia tutti i server
run-all: all run-servers
	@echo "Compilati e avviati tutti i server."

