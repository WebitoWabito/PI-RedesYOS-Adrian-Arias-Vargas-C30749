#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define SIZE 100
#define MAX 10

struct mensaje {
    int requestID;
    int clienteID;
    char texto[SIZE];
};

struct Cola {
    struct mensaje buffer[MAX];
    int inicio, fin;
    pthread_mutex_t mutex;
};

void initCola(struct Cola *c) {
    c->inicio = c->fin = 0;
    pthread_mutex_init(&c->mutex, NULL);
}

void enqueue(struct Cola *c, struct mensaje m) {
    pthread_mutex_lock(&c->mutex);
    c->buffer[c->fin % MAX] = m;
    c->fin++;
    pthread_mutex_unlock(&c->mutex);
}

int dequeue(struct Cola *c, struct mensaje *m) {
    pthread_mutex_lock(&c->mutex);

    if (c->inicio == c->fin) {
        pthread_mutex_unlock(&c->mutex);
        return 0;
    }

    *m = c->buffer[c->inicio % MAX];
    c->inicio++;

    pthread_mutex_unlock(&c->mutex);
    return 1;
}

struct Cola colaRequest;
struct Cola colaToServer;
struct Cola colaFromServer;
struct Cola colaResponse;

void *servidor(void *arg) {
    struct mensaje msg;
    FILE *file;
    int id;
    char nombre[SIZE];

    while (1) {

        if (!dequeue(&colaToServer, &msg)) {
            usleep(1000);
            continue;
        }

        printf("[Servidor] Recibio %s del cliente %d\n", msg.texto, msg.clienteID);

        //parse
        char proto, cmd;
        char data[SIZE];
        sscanf(msg.texto, "%c/%c/%[^\n]", &proto, &cmd, data);

        if (cmd == 'R' && strcmp(data, "dir") == 0) {
            file = fopen("db.txt", "r");
        
            if (!file) {
                strcpy(msg.texto, "P/E/Error abriendo db");
            } else {
                msg.texto[0] = '\0'; 
        
                char linea[SIZE];
                while (fgets(linea, SIZE, file)) {
                    strcat(msg.texto, linea);
                }
                fclose(file);
                //se formatea como P/D/datos
                char temp[SIZE];
                strcpy(temp, msg.texto);
                snprintf(msg.texto, SIZE, "P/D/%s", temp);
            }
        }

        else if (cmd == 'G') {
            int req_id = atoi(data);
            int encontrado = 0;
            file = fopen("db.txt", "r");

            if (file) {
                while (fscanf(file, "%d %[^\n]", &id, nombre) != EOF) {
                    if (id == req_id) {
                        snprintf(msg.texto, SIZE, "P/D/%d %s", id, nombre);
                        encontrado = 1;
                        break;
                    }
                }
                fclose(file);
            }

            if (!encontrado)
                strcpy(msg.texto, "P/E/No encontrado");
        }

        else {
            strcpy(msg.texto, "P/E/Comando no soportado");
        }

        printf("[Servidor] Procesado\n");

        enqueue(&colaFromServer, msg);
    }
}

void *intermediario(void *arg) {
    struct mensaje msg;

    while (1) {

        if (!dequeue(&colaRequest, &msg)) {
            usleep(1000);
            continue;
        }

        printf("[Intermediario] Recibio %s del cliente %d\n", msg.texto, msg.clienteID);

        // valida el formato del mensaje
        char proto, cmd;
        char data[SIZE];
        if (sscanf(msg.texto, "%c/%c/%s", &proto, &cmd, data) < 2 || proto != 'P') {
            strcpy(msg.texto, "P/E/Formato invalido");
            enqueue(&colaResponse, msg);
            continue;
        }

        //Request y get son los unicos comandos validos
        if (cmd != 'R' && cmd != 'G') {
            strcpy(msg.texto, "P/E/Comando invalido");
            enqueue(&colaResponse, msg);
            continue;
        }

        printf("[Intermediario] Enviando al servidor\n");
        enqueue(&colaToServer, msg);

        while (!dequeue(&colaFromServer, &msg)) {
            usleep(1000);
        }

        printf("[Intermediario] Recibio respuesta del servidor\n");

        enqueue(&colaResponse, msg);

        printf("[Intermediario] Envio respuesta al cliente %d\n", msg.clienteID);
    }
}

void *cliente(void *arg) {
    struct mensaje msg;
    memset(&msg, 0, sizeof(msg));

    int idCliente = (int)(size_t)arg;

    printf("\nCliente %d\n", idCliente);
    printf("Opciones:\n");
    printf("Ver lista: P/R/dir\n");
    printf("Buscar por ID: P/G/<id>\n");
    printf("Comando: ");

    scanf("%s", msg.texto);

    msg.clienteID = idCliente;
    msg.requestID = idCliente;

    printf("[Cliente %d] Enviando %s\n", idCliente, msg.texto);

    enqueue(&colaRequest, msg);

    while (!dequeue(&colaResponse, &msg)) {
        usleep(1000);
    }

    printf("[Cliente %d] Recibio respuesta:\n%s\n", idCliente, msg.texto);

    pthread_exit(NULL);
}

int main() {
    pthread_t servidorT, interT, clientes[3];

    initCola(&colaRequest);
    initCola(&colaToServer);
    initCola(&colaFromServer);
    initCola(&colaResponse);

    pthread_create(&servidorT, NULL, servidor, NULL);
    pthread_create(&interT, NULL, intermediario, NULL);

    sleep(1); 

    for (int i = 0; i < 3; i++) {
        pthread_create(&clientes[i], NULL, cliente, (void*)(size_t)(i+1));
        pthread_join(clientes[i], NULL);
    }

    pthread_cancel(servidorT);
    pthread_cancel(interT);

    return 0;
}