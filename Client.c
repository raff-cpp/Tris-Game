#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096
#define DEFAULT_PORT 8080

int main() {

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Crea il socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }



    // Lettura delle variabili d'ambiente per host e porta del server
    char *server_host_env = getenv("SERVER_HOST");
    char *server_port_str_env = getenv("SERVER_PORT");
    char *server_host;
    char server_port_str[6]; // porta come stringa (max 5 cifre + terminatore)

    if (server_host_env != NULL) {
        server_host = server_host_env;
    } else {
        server_host = "127.0.0.1"; //Che è quello di default
    }

    if (server_port_str_env != NULL) {
        snprintf(server_port_str, sizeof(server_port_str), "%s", server_port_str_env); // Copia la porta dall'ambiente
    } else {
        snprintf(server_port_str, sizeof(server_port_str), "%d", DEFAULT_PORT); // Porta di default
    }




    // Risolve l'hostname del server usando getaddrinfo
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(server_host, server_port_str, &hints, &res) != 0) {
        perror("Errore nella risoluzione del server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Copia l'indirizzo del server risolto
    memcpy(&server_addr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);



    // Connessione al server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nella connessione al server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }



    printf("Connesso al server %s:%s\n", server_host, server_port_str);
    
    // Ciclo di comunicazione con il server
    while (1) {
        // Legge la risposta del server (se disponibile)
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms timeout
        
        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity < 0) {
            perror("Errore in select");
            break;
        }
        
        // Se il server ha inviato dati
        if (FD_ISSET(sockfd, &readfds)) {
            ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received < 0) {
                perror("Errore nella ricezione dei dati");
                break;
            } else if (bytes_received == 0) {
                printf("Connessione chiusa dal server\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("%s", buffer);  // Stampa il messaggio del server così com'è
        }
        
        // Se l'utente ha digita qualcosa da stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char user_input[BUFFER_SIZE];
            if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
                ssize_t bytes_sent = send(sockfd, user_input, strlen(user_input), 0);
                if (bytes_sent < 0) {
                    perror("Errore nell'invio dei dati");
                    break;
                }
            }
        }
    }
}