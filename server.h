#ifndef SERVER_H
#define SERVER_H

#include "trisGame.h"

#define BUFFER_SIZE 4096
#define DEFAULT_PORT 8080
#define MAX_CLIENTS 8
#define MAX_GAMES 4

/* Struttura per tracciare lo stato di un client */
typedef struct {
    int socket;
    int gameId;
    char playerSymbol;
    int isConnected;
} ClientState;

/* Struttura per tracciare lo stato di un giocatore in partita */
typedef struct {
    int clientSocket;
    char username[32];
    char symbol;
    int countMoves;
    int isReady;
} PlayerStatus;

/* Struttura per tracciare lo stato di una partita */
typedef struct {
    int gameId;
    TrisGame gameData;
    PlayerStatus players[2];
    Risultato result;
    int isActive;
} GameState;

/* ===== FUNZIONI DI INVIO ===== */
void sendMessage(int client_sock, const char *message);
void sendError(int client_sock, const char *errorMessage);
void sendPrompt(int client_sock);
void sendWelcomeMessage(int client_sock);
void sendGoodbyeMessage(int client_sock);
void sendBoardState(int client_sock, TrisGame *game);
void sendFullGameState(int client_sock, TrisGame *game);
void sendCurrentPlayer(int client_sock, char currentPlayer);
void sendMoveCount(int client_sock, int moveCount);
void sendGameResult(int client_sock, Risultato result);
void sendInvalidMove(int client_sock);
void sendGameOver(int client_sock, Risultato result);

/* ===== FUNZIONI DI RICERCA - CLIENT ===== */
ClientState* getClientState(int client_sock);
ClientState* getClientById(int client_sock);

/* ===== FUNZIONI DI RICERCA - GIOCATORE ===== */
PlayerStatus* getPlayerBySymbol(GameState *gs, char symbol);
PlayerStatus* getPlayerBySocket(GameState *gs, int client_sock);
PlayerStatus* getPlayerByUsername(GameState *gs, const char *username);
PlayerStatus* getPlayerStatusByUsername(const char *username);

/* ===== FUNZIONI DI RICERCA - PARTITA ===== */
GameState* getGameStateById(int gameId);
GameState* findGameStateByClient(int client_sock);
GameState* getAvailableGameState(void);
int getGameIndexById(int gameId);

/* ===== FUNZIONI DI GESTIONE - PARTITE/CLIENT ===== */
void initServerState(void);
void registerClient(int client_sock);
void removeClient(int client_sock);
void printGameList(int client_sock);

/* ===== FUNZIONI DI CREAZIONE/MODIFICA PARTITE ===== */
GameState* createGame(int client_sock, const char *username);
void joinGame(int client_sock, int gameId, const char *username);

/* ===== HANDLER COMANDI ===== */
void handleCmdNew(int client_sock);
void handleCmdJoin(int client_sock, int gameId);
void handleCmdMove(int client_sock, int row, int col);
void handleCmdList(int client_sock);
void handleCmdHelp(int client_sock);
void handleCmdQuit(int client_sock);
void handleCmdGameState(int client_sock);
void handleCmdCurrentPlayer(int client_sock);
void handleCmdMoveCount(int client_sock);
void handleCmdBoard(int client_sock);
void handleCmdResult(int client_sock);
void handleCmdInvalid(int client_sock);
void handleCmdGameOver(int client_sock);
void handleClient(int client_sock);

#endif // SERVER_H



