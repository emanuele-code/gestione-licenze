## Descrizione

Il progetto implementa un sistema distribuito per la gestione delle licenze software, basato su più server con ruoli distinti e client dedicati. L’architettura è pensata per separare responsabilità, migliorare la sicurezza e simulare un’infrastruttura client–server reale.

## Requisiti

- Sistema operativo Unix/Linux  
- make  
- bash  

## Compilazione

La compilazione del progetto è gestita tramite **Makefile**, al fine di automatizzare il processo ed evitare la compilazione manuale dei singoli file.

Per compilare il progetto:
```bash
make
```

Per compilare ed avviare automaticamente tutti i server:

```bash
make run-all
```

## Avvio e arresto dei server
Dopo la compilazione, i server possono essere avviati con:

```bash
make run-servers
```

Per arrestare l’esecuzione dei server:

```bash
make stop-servers
```

## Avvio dei client
Una volta avviati i server, è possibile selezionare il client da eseguire tramite il menu interattivo:
```bash
make menu-client
```
