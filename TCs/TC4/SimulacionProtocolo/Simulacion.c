#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define SIZE 100
#define MAX 10

#define GETLIST 1
#define GETID 2
#define POST 3
#define DELETE 4

struct mensaje {
    int requestID;
    int clienteID;
    int operacion;
    int id;
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

        printf("[Servidor] Recibio request %d del cliente %d\n", msg.requestID, msg.clienteID);

        if (msg.operacion == GETLIST) {
            file = fopen("db.txt", "r");
        
            if (!file) {
                strcpy(msg.texto, "Error abriendo db");
            } else {
                msg.texto[0] = '\0'; 
        
                char linea[SIZE];
                while (fgets(linea, SIZE, file)) {
                    strcat(msg.texto, linea);
                }
                fclose(file);
            }
        }

        else if (msg.operacion == GETID) {
            int encontrado = 0;
            file = fopen("db.txt", "r");

            if (file) {
                while (fscanf(file, "%d %[^\n]", &id, nombre) != EOF) {
                    if (id == msg.id) {
                        sprintf(msg.texto, "%d %s", id, nombre);
                        encontrado = 1;
                        break;
                    }
                }
                fclose(file);
            }

            if (!encontrado)
                strcpy(msg.texto, "No encontrado");
        }

        else if (msg.operacion == POST) {
            int existe = 0;
            file = fopen("db.txt", "r");

            if (file) {
                while (fscanf(file, "%d %[^\n]", &id, nombre) != EOF) {
                    if (id == msg.id) {
                        existe = 1;
                        break;
                    }
                }
                fclose(file);
            }

            if (existe) {
                strcpy(msg.texto, "ID ya existe");
            } else {
                file = fopen("db.txt", "a");
                fprintf(file, "%d %s\n", msg.id, msg.texto);
                fclose(file);
                strcpy(msg.texto, "Agregado");
            }
        }

        else if (msg.operacion == DELETE) {
            FILE *temporaryFile = fopen("temporaryFile.txt", "w");
            file = fopen("db.txt", "r");

            int encontrado = 0;

            if (file && temporaryFile) {
                while (fscanf(file, "%d %[^\n]", &id, nombre) != EOF) {
                    if (id == msg.id) {
                        encontrado = 1;
                        continue;
                    }
                    fprintf(temporaryFile, "%d %s\n", id, nombre);
                }

                fclose(file);
                fclose(temporaryFile);

                remove("db.txt");
                rename("temporaryFile.txt", "db.txt");

                if (encontrado)
                    strcpy(msg.texto, "Eliminado");
                else
                    strcpy(msg.texto, "No encontrado");
            }
        }

        printf("[Servidor] Procesado request %d\n", msg.requestID);

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

        printf("[Intermediario] Recibio request %d del cliente %d\n", msg.requestID, msg.clienteID);

        printf("[Intermediario] Enviando request %d al servidor\n", msg.requestID);
        enqueue(&colaToServer, msg);

        while (!dequeue(&colaFromServer, &msg)) {
            usleep(1000);
        }

        printf("[Intermediario] Recibio respuesta %d del servidor\n", msg.requestID);

        enqueue(&colaResponse, msg);

        printf("[Intermediario] Envio respuesta %d al cliente %d\n", msg.requestID, msg.clienteID);
    }
}

void *cliente(void *arg) {
    struct mensaje msg;
    memset(&msg, 0, sizeof(msg));

    int idCliente = (int)(size_t)arg;

    printf("\nCliente %d\n", idCliente);
    printf("1. Ver lista\n2. Buscar por ID\n3. Agregar figura\n4. Eliminar figura\nOpcion: ");

    int op;
    scanf("%d", &op);

    msg.clienteID = idCliente;
    msg.requestID = idCliente;

    if (op == 1) msg.operacion = GETLIST;

    else if (op == 2) {
        msg.operacion = GETID;
        printf("Ingrese ID: ");
        scanf("%d", &msg.id);
    }

    else if (op == 3) {
        msg.operacion = POST;
        printf("Ingrese ID: ");
        scanf("%d", &msg.id);
        printf("Nombre: ");
        // scanf("%s", msg.texto);
        scanf(" %[^\n]", msg.texto);
    }

    else if (op == 4) {
        msg.operacion = DELETE;
        printf("Ingrese ID: ");
        scanf("%d", &msg.id);
    }

    printf("[Cliente %d] Enviando request %d\n", idCliente, msg.requestID);

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