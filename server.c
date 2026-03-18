#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "server.h"

static GameState gameStates[MAX_GAMES];
static ClientState clients[MAX_CLIENTS];
static int nextGameId = 1;

void initServerState() {
    for (int i = 0; i < MAX_GAMES; i++) {
        gameStates[i].gameId = 0;
        gameStates[i].isActive = 0;
        gameStates[i].result = IN_CORSO;
        gameStates[i].players[0].clientSocket = -1;
        gameStates[i].players[0].symbol = 'X';
        gameStates[i].players[0].countMoves = 0;
        gameStates[i].players[0].isReady = 0;
        memset(gameStates[i].players[0].username, 0, sizeof(gameStates[i].players[0].username));
        gameStates[i].players[1].clientSocket = -1;
        gameStates[i].players[1].symbol = 'O';
        gameStates[i].players[1].countMoves = 0;
        gameStates[i].players[1].isReady = 0;
        memset(gameStates[i].players[1].username, 0, sizeof(gameStates[i].players[1].username));
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        clients[i].gameId = -1;
        clients[i].playerSymbol = '\0';
        clients[i].isConnected = 0;
    }
}

static void safeSend(int client_sock, const char *msg) {
    if (!msg) return;
    send(client_sock, msg, strlen(msg), 0);
}

void sendMessage(int client_sock, const char *message) {
    char buf[BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s\n", message);
    safeSend(client_sock, buf);
}

void sendError(int client_sock, const char *errorMessage) {
    char buf[BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "Errore: %s\n", errorMessage);
    safeSend(client_sock, buf);
}

void sendCurrentPlayer(int client_sock, char currentPlayer) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Turno di: %c\n", currentPlayer);
    safeSend(client_sock, buf);
}

void sendMoveCount(int client_sock, int moveCount) {
    char buf[64];
    snprintf(buf, sizeof(buf), "Mosse effettuate: %d\n", moveCount);
    safeSend(client_sock, buf);
}

void sendBoardState(int client_sock, TrisGame *game) {
    if (!game) return;
    char buffer[256];
    stampaTabellone(game, buffer, sizeof(buffer));
    safeSend(client_sock, buffer);
}

void sendFullGameState(int client_sock, TrisGame *game) {
    sendBoardState(client_sock, game);
    sendCurrentPlayer(client_sock, game->currentPlayer);
    sendMoveCount(client_sock, game->movesCount);
}

void sendGameResult(int client_sock, Risultato result) {
    if (result == VITTORIA_X) sendMessage(client_sock, "Vittoria di X!");
    else if (result == VITTORIA_O) sendMessage(client_sock, "Vittoria di O!");
    else if (result == PAREGGIO) sendMessage(client_sock, "Pareggio!");
    else sendMessage(client_sock, "Partita in corso...");
}

void sendInvalidMove(int client_sock) {
    sendMessage(client_sock, "Mossa non valida: riga/colonna fuori o già occupata.");
}

void sendGameOver(int client_sock, Risultato result) {
    sendMessage(client_sock, "Partita terminata.");
    sendGameResult(client_sock, result);
}

void sendWelcomeMessage(int client_sock) {
    sendMessage(client_sock, "Benvenuto al server Tris! Digita 'help' per i comandi disponibili.");
}

void sendGoodbyeMessage(int client_sock) {
    sendMessage(client_sock, "Arrivederci!");
}

void sendPrompt(int client_sock) {
    safeSend(client_sock, "> ");
}

/* ===== FUNZIONI DI RICERCA - CLIENT ===== */

ClientState* getClientState(int client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_sock) return &clients[i];
    }
    return NULL;
}

ClientState* getClientById(int client_sock) {
    return getClientState(client_sock);
}

/* ===== FUNZIONI DI RICERCA - GIOCATORE ===== */

PlayerStatus* getPlayerBySymbol(GameState *gs, char symbol) {
    if (!gs) return NULL;
    if (gs->players[0].symbol == symbol) return &gs->players[0];
    if (gs->players[1].symbol == symbol) return &gs->players[1];
    return NULL;
}

PlayerStatus* getPlayerBySocket(GameState *gs, int client_sock) {
    if (!gs) return NULL;
    if (gs->players[0].clientSocket == client_sock) return &gs->players[0];
    if (gs->players[1].clientSocket == client_sock) return &gs->players[1];
    return NULL;
}

PlayerStatus* getPlayerByUsername(GameState *gs, const char *username) {
    if (!gs || !username) return NULL;
    if (strcmp(gs->players[0].username, username) == 0) return &gs->players[0];
    if (strcmp(gs->players[1].username, username) == 0) return &gs->players[1];
    return NULL;
}

PlayerStatus* getPlayerStatusByUsername(const char *username) {
    if (!username) return NULL;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].isActive) {
            PlayerStatus *p = getPlayerByUsername(&gameStates[i], username);
            if (p) return p;
        }
    }
    return NULL;
}

/* ===== FUNZIONI DI RICERCA - PARTITA ===== */

GameState* getGameStateById(int gameId) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].gameId == gameId) return &gameStates[i];
    }
    return NULL;
}

GameState* findGameStateByClient(int client_sock) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].isActive && 
            (gameStates[i].players[0].clientSocket == client_sock || 
             gameStates[i].players[1].clientSocket == client_sock)) {
            return &gameStates[i];
        }
    }
    return NULL;
}

GameState* getAvailableGameState(void) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].isActive && gameStates[i].players[0].isReady && !gameStates[i].players[1].isReady) {
            return &gameStates[i];
        }
    }
    return NULL;
}

int getGameIndexById(int gameId) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].gameId == gameId) return i;
    }
    return -1;
}

/* ===== FUNZIONI DI GESTIONE - PARTITE/CLIENT ===== */

void registerClient(int client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == -1) {
            clients[i].socket = client_sock;
            clients[i].gameId = -1;
            clients[i].playerSymbol = '\0';
            clients[i].isConnected = 1;
            return;
        }
    }
}

void removeClient(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (gs) {
        if (gs->players[0].clientSocket == client_sock && gs->players[1].isReady) {
            sendMessage(gs->players[1].clientSocket, "L'avversario si è disconnesso. Partita terminata.");
        } else if (gs->players[1].clientSocket == client_sock && gs->players[0].isReady) {
            sendMessage(gs->players[0].clientSocket, "L'avversario si è disconnesso. Partita terminata.");
        }
        gs->gameId = 0;
        gs->isActive = 0;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_sock) {
            clients[i].socket = -1;
            clients[i].gameId = -1;
            clients[i].playerSymbol = '\0';
            clients[i].isConnected = 0;
        }
    }
}

void printGameList(int client_sock) {
    int found = 0;
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].isActive) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Game %d - X: %s, O: %s, Mosse: %d, Stato: %s",
                     gameStates[i].gameId,
                     gameStates[i].players[0].username,
                     gameStates[i].players[1].isReady ? gameStates[i].players[1].username : "In attesa",
                     gameStates[i].gameData.movesCount,
                     gameStates[i].result == IN_CORSO ? "In corso" : "Terminata");
            sendMessage(client_sock, buf);
            found = 1;
        }
    }
    if (!found) sendMessage(client_sock, "Nessuna partita attiva.");
}

/* ===== FUNZIONI DI CREAZIONE/MODIFICA PARTITE ===== */

GameState* createGame(int client_sock, const char *username) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (gameStates[i].gameId == 0) {
            gameStates[i].gameId = nextGameId++;
            gameStates[i].isActive = 1;
            gameStates[i].result = IN_CORSO;
            gameStates[i].players[0].clientSocket = client_sock;
            gameStates[i].players[0].isReady = 1;
            gameStates[i].players[0].countMoves = 0;
            strncpy(gameStates[i].players[0].username, username, sizeof(gameStates[i].players[0].username) - 1);
            gameStates[i].players[1].clientSocket = -1;
            gameStates[i].players[1].isReady = 0;
            gameStates[i].players[1].countMoves = 0;
            memset(gameStates[i].players[1].username, 0, sizeof(gameStates[i].players[1].username));
            iniziaPartita(&gameStates[i].gameData);
            gameStates[i].gameData.id = gameStates[i].gameId;
            return &gameStates[i];
        }
    }
    return NULL;
}

void joinGame(int client_sock, int gameId, const char *username) {
    GameState *gs = getGameStateById(gameId);
    if (!gs) return;
    if (gs->players[1].isReady) return;
    if (gs->players[0].clientSocket == client_sock) return;

    gs->players[1].clientSocket = client_sock;
    gs->players[1].isReady = 1;
    strncpy(gs->players[1].username, username, sizeof(gs->players[1].username) - 1);

    ClientState *cs = getClientState(client_sock);
    if (cs) {
        cs->gameId = gameId;
        cs->playerSymbol = 'O';
    }

    sendMessage(client_sock, "Ti sei unito alla partita come giocatore O. Il gioco inizia!");
    sendMessage(gs->players[0].clientSocket, "Un avversario si è unito. Il gioco può iniziare!");
    sendFullGameState(gs->players[0].clientSocket, &gs->gameData);
    sendFullGameState(gs->players[1].clientSocket, &gs->gameData);
}

/* ===== HANDLER COMANDI ===== */

void handleCmdNew(int client_sock) {
    sendMessage(client_sock, "Inserisci il tuo username:");
    sendPrompt(client_sock);
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';
    
    char *end = buffer + strlen(buffer) - 1;
    while (end >= buffer && (*end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }

    if (strlen(buffer) == 0) {
        sendError(client_sock, "Username non valido.");
        return;
    }

    GameState *gs = createGame(client_sock, buffer);
    if (!gs) {
        sendError(client_sock, "Non ci sono slot liberi per nuove partite.");
        return;
    }

    ClientState *cs = getClientState(client_sock);
    if (cs) {
        cs->gameId = gs->gameId;
        cs->playerSymbol = 'X';
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Partita creata con ID %d. Sei il giocatore X (%s). In attesa di un avversario...", gs->gameId, buffer);
    sendMessage(client_sock, msg);
}

void handleCmdJoin(int client_sock, int gameId) {
    GameState *gs = getGameStateById(gameId);
    if (!gs) {
        sendError(client_sock, "Game ID non trovato.");
        return;
    }
    if (gs->players[1].isReady) {
        sendError(client_sock, "La partita è già completa.");
        return;
    }
    if (gs->players[0].clientSocket == client_sock) {
        sendError(client_sock, "Sei già in questa partita.");
        return;
    }

    sendMessage(client_sock, "Inserisci il tuo username:");
    sendPrompt(client_sock);
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';
    
    char *end = buffer + strlen(buffer) - 1;
    while (end >= buffer && (*end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }

    if (strlen(buffer) == 0) {
        sendError(client_sock, "Username non valido.");
        return;
    }

    joinGame(client_sock, gameId, buffer);
}

void handleCmdMove(int client_sock, int row, int col) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita attiva.");
        return;
    }

    ClientState *cs = getClientState(client_sock);
    if (!cs) {
        sendError(client_sock, "Errore interno: client non trovato.");
        return;
    }

    if (gs->gameData.currentPlayer != cs->playerSymbol) {
        sendError(client_sock, "Non è il tuo turno.");
        return;
    }

    if (faiMossa(&gs->gameData, row, col) != 0) {
        sendInvalidMove(client_sock);
        return;
    }

    PlayerStatus *player = getPlayerBySocket(gs, client_sock);
    if (player) player->countMoves++;

    gs->result = controllaVincitore(&gs->gameData);

    sendFullGameState(gs->players[0].clientSocket, &gs->gameData);
    sendFullGameState(gs->players[1].clientSocket, &gs->gameData);

    if (gs->result != IN_CORSO) {
        sendGameOver(gs->players[0].clientSocket, gs->result);
        sendGameOver(gs->players[1].clientSocket, gs->result);
        gs->gameId = 0;
        gs->isActive = 0;
    }
}

void handleCmdList(int client_sock) {
    printGameList(client_sock);
}

void handleCmdHelp(int client_sock) {
    sendMessage(client_sock, "===== COMANDI DISPONIBILI =====");
    sendMessage(client_sock, "new              - Crea una nuova partita");
    sendMessage(client_sock, "join <id>        - Unisciti a una partita");
    sendMessage(client_sock, "list             - Elenca le partite");
    sendMessage(client_sock, "move <r> <c>     - Effettua una mossa (r,c: 0-2)");
    sendMessage(client_sock, "board            - Mostra il tabellone");
    sendMessage(client_sock, "status           - Mostra lo stato della partita");
    sendMessage(client_sock, "result           - Mostra il risultato");
    sendMessage(client_sock, "quit             - Disconnettiti");
    sendMessage(client_sock, "================================");
}

void handleCmdQuit(int client_sock) {
    sendGoodbyeMessage(client_sock);
}

void handleCmdGameState(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita.");
        return;
    }
    sendFullGameState(client_sock, &gs->gameData);
}

void handleCmdCurrentPlayer(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita.");
        return;
    }
    sendCurrentPlayer(client_sock, gs->gameData.currentPlayer);
}

void handleCmdMoveCount(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita.");
        return;
    }
    sendMoveCount(client_sock, gs->gameData.movesCount);
}

void handleCmdBoard(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita.");
        return;
    }
    sendBoardState(client_sock, &gs->gameData);
}

void handleCmdResult(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita.");
        return;
    }
    sendGameResult(client_sock, gs->result);
}

void handleCmdInvalid(int client_sock) {
    sendError(client_sock, "Comando non riconosciuto. Digita 'help' per l'aiuto.");
}

void handleCmdGameOver(int client_sock) {
    GameState *gs = findGameStateByClient(client_sock);
    if (!gs) {
        sendError(client_sock, "Non sei in una partita.");
        return;
    }
    if (gs->result == IN_CORSO) {
        sendMessage(client_sock, "La partita è ancora in corso.");
    } else {
        sendGameOver(client_sock, gs->result);
    }
}

void handleClient(int client_sock) {
    char buffer[BUFFER_SIZE];

    registerClient(client_sock);
    sendWelcomeMessage(client_sock);
    sendPrompt(client_sock);

    while (1) {
        ssize_t bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';

        char *end = buffer + strlen(buffer) - 1;
        while (end >= buffer && (*end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }

        if (strlen(buffer) == 0) {
            sendPrompt(client_sock);
            continue;
        }

        char cmd[64];
        int r, c, gameId;
        sscanf(buffer, "%63s", cmd);

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            handleCmdQuit(client_sock);
            break;
        } else if (strcmp(cmd, "new") == 0) {
            handleCmdNew(client_sock);
        } else if (strcmp(cmd, "join") == 0) {
            if (sscanf(buffer, "%*s %d", &gameId) == 1) {
                handleCmdJoin(client_sock, gameId);
            } else {
                sendError(client_sock, "Uso: join <gameId>");
            }
        } else if (strcmp(cmd, "list") == 0) {
            handleCmdList(client_sock);
        } else if (strcmp(cmd, "move") == 0) {
            if (sscanf(buffer, "%*s %d %d", &r, &c) == 2) {
                handleCmdMove(client_sock, r, c);
            } else {
                sendError(client_sock, "Uso: move <riga> <colonna>");
            }
        } else if (strcmp(cmd, "help") == 0) {
            handleCmdHelp(client_sock);
        } else if (strcmp(cmd, "status") == 0) {
            handleCmdGameState(client_sock);
        } else if (strcmp(cmd, "board") == 0) {
            handleCmdBoard(client_sock);
        } else if (strcmp(cmd, "result") == 0) {
            handleCmdResult(client_sock);
        } else {
            handleCmdInvalid(client_sock);
        }

        sendPrompt(client_sock);
    }

    removeClient(client_sock);
    close(client_sock);
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_sock);
        return 1;
    }

    initServerState();
    printf("Server in ascolto sulla porta %d...\n", DEFAULT_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        printf("Client connesso: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (fork() == 0) {
            close(server_sock);
            handleClient(client_sock);
            exit(0);
        }
        close(client_sock);
    }

    close(server_sock);
    return 0;
}