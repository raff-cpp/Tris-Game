#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trisGame.h"

void startGame(TrisGame *game) {
    memset(game->cella, 0, sizeof(game->cella)); // Inizializza la griglia con celle vuote (VUOTA==0)
    game->currentPlayer = 'X'; // Il giocatore X inizia per primo
    game->movesCount = 0; // Contatore delle mosse
}


Risultato checkResult(TrisGame *game) {
    // Controlla righe, colonne e diagonali per un vincitore
    for (int i = 0; i < 3; i++) {
        // Controlla la riga i
        if (game->cella[i][0] == game->cella[i][1] && game->cella[i][1] == game->cella[i][2] && game->cella[i][0] != VUOTA) {
            return (game->cella[i][0] == X) ? VITTORIA_X : VITTORIA_O;
        }
        // Controlla la colonna i
        if (game->cella[0][i] == game->cella[1][i] && game->cella[1][i] == game->cella[2][i] && game->cella[0][i] != VUOTA) {
            return (game->cella[0][i] == X) ? VITTORIA_X : VITTORIA_O;
        }
    }
    // Controlla la diagonale principale e quella secondaria
    if (game->cella[0][0] == game->cella[1][1] && game->cella[1][1] == game->cella[2][2] && game->cella[0][0] != VUOTA) {
        return (game->cella[0][0] == X) ? VITTORIA_X : VITTORIA_O;
    }
    
    if (game->cella[0][2] == game->cella[1][1] && game->cella[1][1] == game->cella[2][0] && game->cella[0][2] != VUOTA) {
        return (game->cella[0][2] == X) ? VITTORIA_X : VITTORIA_O;
    }
    // Controlla se la partita è in corso o è un pareggio
    return (game->movesCount < 9) ? IN_CORSO : PAREGGIO;
}



int makeMove(TrisGame *game, int row, int col) {
    // Controlla se la mossa è valida (dentro i limiti e cella vuota)
    if (row < 0 || row >= 3 || col < 0 || col >= 3 || game->cella[row][col] != VUOTA) {
        return -1; // Mossa non valida
    }
    // Aggiorna la griglia con il simbolo del giocatore corrente (converti char -> Cella)
    game->cella[row][col] = (game->currentPlayer == 'X') ? X : O;
    game->movesCount++; // Incrementa il contatore delle mosse
    // Cambia turno
    if (game->currentPlayer == 'X') game->currentPlayer = 'O';
    else game->currentPlayer = 'X';
    return 0; // Mossa effettuata con successo
}



void printBoard(TrisGame *game, char *buffer, size_t bufsize) {
    size_t used = 0;
    int n = 0;
    if (bufsize == 0) return;
    buffer[0] = '\0';

    n = snprintf(buffer + used, (used < bufsize) ? (bufsize - used) : 0,
         " %c | %c | %c \n",
         (game->cella[0][0] == X) ? 'X' : (game->cella[0][0] == O) ? 'O' : ' ',
         (game->cella[0][1] == X) ? 'X' : (game->cella[0][1] == O) ? 'O' : ' ',
         (game->cella[0][2] == X) ? 'X' : (game->cella[0][2] == O) ? 'O' : ' ');
    if (n > 0) used += (size_t)n < (bufsize - used) ? (size_t)n : (bufsize - used);

    n = snprintf(buffer + used, (used < bufsize) ? (bufsize - used) : 0, "---|---|---\n");
    if (n > 0) used += (size_t)n < (bufsize - used) ? (size_t)n : (bufsize - used);

    n = snprintf(buffer + used, (used < bufsize) ? (bufsize - used) : 0,
         " %c | %c | %c \n",
         (game->cella[1][0] == X) ? 'X' : (game->cella[1][0] == O) ? 'O' : ' ',
         (game->cella[1][1] == X) ? 'X' : (game->cella[1][1] == O) ? 'O' : ' ',
         (game->cella[1][2] == X) ? 'X' : (game->cella[1][2] == O) ? 'O' : ' ');
    if (n > 0) used += (size_t)n < (bufsize - used) ? (size_t)n : (bufsize - used);

    n = snprintf(buffer + used, (used < bufsize) ? (bufsize - used) : 0, "---|---|---\n");
    if (n > 0) used += (size_t)n < (bufsize - used) ? (size_t)n : (bufsize - used);

    n = snprintf(buffer + used, (used < bufsize) ? (bufsize - used) : 0,
         " %c | %c | %c \n",
         (game->cella[2][0] == X) ? 'X' : (game->cella[2][0] == O) ? 'O' : ' ',
         (game->cella[2][1] == X) ? 'X' : (game->cella[2][1] == O) ? 'O' : ' ',
         (game->cella[2][2] == X) ? 'X' : (game->cella[2][2] == O) ? 'O' : ' ');
    if (n > 0) used += (size_t)n < (bufsize - used) ? (size_t)n : (bufsize - used);
}

