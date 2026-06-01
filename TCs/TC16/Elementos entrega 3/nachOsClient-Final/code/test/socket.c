#include "syscall.h"

#define BUFSIZE     512
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080


int mystrlen(const char *s) {
    int i = 0; while (s[i]) i++; return i;
}
void mystrcpy(char *d, const char *s) {
    int i = 0; while (s[i]) { d[i]=s[i]; i++; } d[i]='\0';
}
void mystrcat(char *d, const char *s) {
    int i=mystrlen(d), j=0;
    while (s[j]) d[i++]=s[j++]; d[i]='\0';
}
void limpiar(char *b, int n) {
    int i; for(i=0;i<n;i++) b[i]='\0';
}
void trimNL(char *s) {
    int i=0;
    while (s[i]) {
        if (s[i]=='\n'||s[i]=='\r') { s[i]='\0'; return; }
        i++;
    }
}
void intToStr(int n, char *dst) {
    char rev[12]; int i=0, j=0;
    if (n==0) { dst[0]='0'; dst[1]='\0'; return; }
    while (n>0) { rev[i++]='0'+(n%10); n/=10; }
    while (i>0) dst[j++]=rev[--i];
    dst[j]='\0';
}


void readContent(char *dst, int dstSize) {
    limpiar(dst, dstSize);
    Write("(Escribi el contenido, linea por linea.\n", 40, ConsoleOutput);
    Write(" Cuando termines, escribe . en una linea sola)\n", 47, ConsoleOutput);

    char line[64];
    int total = 0;

    while (1) {
        limpiar(line, 64);
        Read(line, 63, ConsoleInput);
        trimNL(line);

        if (line[0] == '.' && line[1] == '\0') break;

        if (total > 0 && total < dstSize - 1) {
            dst[total++] = '|';   
        }

        int j = 0;
        while (line[j] && total < dstSize - 1) {
            dst[total++] = line[j++];
        }
        dst[total] = '\0';
    }
}


void skipHeader(const char *src, char *dst, int sz) {
    int i = 0;
    while (src[i]) {
        if (src[i]=='\r'&&src[i+1]=='\n'&&src[i+2]=='\r'&&src[i+3]=='\n') {
            int j=0, k=i+4;
            while (src[k]&&j<sz-1) dst[j++]=src[k++];
            dst[j]='\0'; return;
        }
        i++;
    }
    mystrcpy(dst, src);
}

void extractResult(const char *src, char *dst, int sz) {
    int i = 0;
    while (src[i]) {
        if (src[i]=='<'&&src[i+1]=='r'&&src[i+2]=='e'&&src[i+3]=='s'&&
            src[i+4]=='u'&&src[i+5]=='l'&&src[i+6]=='t'&&src[i+7]=='>') {
            int j=0, k=i+8;
            while (src[k]&&j<sz-1) {
                if (src[k]=='<'&&src[k+1]=='/') break;
                dst[j++]=src[k++];
            }
            dst[j]='\0'; return;
        }
        i++;
    }
    int j=0; while(src[j]&&j<sz-1){dst[j]=src[j];j++;} dst[j]='\0';
}


int main() {

    while (1) {

        Write("\n", 1, ConsoleOutput);
        Write("1. GET figura\n",  14, ConsoleOutput);
        Write("2. POST figura\n", 15, ConsoleOutput);
        Write("3. LISTAR\n",      10, ConsoleOutput);
        Write("4. Salir\n",        9, ConsoleOutput);
        Write("> ",                2, ConsoleOutput);

        char opcion[4];
        limpiar(opcion, 4);
        Read(opcion, 3, ConsoleInput);
        trimNL(opcion);

        if (opcion[0] == '4') break;

        Socket_t sock = Socket(AF_INET_NachOS, SOCK_STREAM_NachOS);
        if (sock < 0) { Write("Error socket\n", 13, ConsoleOutput); continue; }
        if (Connect(sock, SERVER_IP, SERVER_PORT) < 0) {
            Write("Error conectando\n", 17, ConsoleOutput);
            Close(sock); continue;
        }

        char method[8];  limpiar(method, 8);
        char path[128];  limpiar(path, 128);
        char body[256];  limpiar(body, 256);

        if (opcion[0] == '1') {

            char figura[64]; limpiar(figura, 64);
            Write("Nombre figura: ", 15, ConsoleOutput);
            Read(figura, 63, ConsoleInput);
            trimNL(figura);
            mystrcpy(method, "GET");
            mystrcpy(path, "/figura/");
            mystrcat(path, figura);

        } else if (opcion[0] == '2') {

            char figura[48]; limpiar(figura, 48);
            Write("Nombre figura: ", 15, ConsoleOutput);
            Read(figura, 47, ConsoleInput);
            trimNL(figura);

            readContent(body, 256);

            mystrcpy(method, "POST");
            mystrcpy(path, "/figura/");
            mystrcat(path, figura);

        } else if (opcion[0] == '3') {

            mystrcpy(method, "GET");
            mystrcpy(path, "/figuras");

        } else {
            Write("Opcion invalida\n", 16, ConsoleOutput);
            Close(sock); continue;
        }

        char request[BUFSIZE]; limpiar(request, BUFSIZE);
        mystrcpy(request, method);
        mystrcat(request, " ");
        mystrcat(request, path);
        mystrcat(request, " HTTP/1.0\r\nHost: 127.0.0.1\r\nUser-Agent: NachOS\r\n");

        if (body[0] != '\0') {
            char lenStr[12]; intToStr(mystrlen(body), lenStr);
            mystrcat(request, "Content-Type: text/plain\r\nContent-Length: ");
            mystrcat(request, lenStr);
            mystrcat(request, "\r\n\r\n");
            mystrcat(request, body);
        } else {
            mystrcat(request, "\r\n");
        }

        Write(request, mystrlen(request), sock);

        char raw[BUFSIZE]; limpiar(raw, BUFSIZE);
        int bytes = Read(raw, BUFSIZE - 1, sock);

        if (bytes > 0) {
            raw[bytes] = '\0';
            char bodyOnly[BUFSIZE]; limpiar(bodyOnly, BUFSIZE);
            skipHeader(raw, bodyOnly, BUFSIZE);
            char result[256]; limpiar(result, 256);
            extractResult(bodyOnly, result, 256);
            Write("\nRespuesta: ", 12, ConsoleOutput);
            Write(result, mystrlen(result), ConsoleOutput);
            Write("\n", 1, ConsoleOutput);
        } else {
            Write("Sin respuesta\n", 14, ConsoleOutput);
        }

        Close(sock);
    }

    Exit(0);
    return 0;
}

