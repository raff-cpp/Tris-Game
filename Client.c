#include <stdio.h>
#include <string.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

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
        snprintf(server_port_str, sizeof(server_port_str), "%d", PORT); // Porta di default
    }




    // Configura l'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // Famiglia di indirizzi IPv4
    server_addr.sin_port = htons(atoi(server_port_str)); // Converte la porta in numero e poi in network byte order
    if (inet_pton(AF_INET, server_host, &server_addr.sin_addr) <= 0) // Converte l'indirizzo IP in formato binario e lo memorizza in server_addr.sin_addr
    {
        perror("Indirizzo del server non valido");
        close(sockfd);
        exit(EXIT_FAILURE);
    }



    // Connessione al server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nella connessione al server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }



    printf("Connesso al server %s:%s\n", server_host, server_port_str);
        // Ciclo di comunicazione con il server
    while (1) {
        // Legge la risposta del server
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("Errore nella ricezione dei dati");
            break;
        } else if (bytes_received == 0) {
            printf("Connessione chiusa dal server\n");
            break;
        }

        buffer[bytes_received] = '\0'; // Assicura che il buffer sia null-terminated
        printf("Messaggio ricevuto dal server: %s\n", buffer);
        // Invia una risposta al server (ad esempio, un semplice messaggio di conferma)
        const char *response = "Messaggio ricevuto";
        ssize_t bytes_sent = send(sockfd, response, strlen(response), 0);
        if (bytes_sent < 0) {
            perror("Errore nell'invio dei dati");
            break;
        }
}
}