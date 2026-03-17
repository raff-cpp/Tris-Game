#ifndef TRIS_GAME_H
#define TRIS_GAME_H

#define SIZE 3


// Possibili risultati della partita
typedef enum {
    IN_CORSO,
    VITTORIA_X, // Vittoria del giocatore X
    VITTORIA_O, // Vittoria del giocatore O
    PAREGGIO
} Risultato;



// Stato di ogni cella del tabellone
typedef enum { VUOTA, X, O } Cella;



// Struttura dati che rappresenta lo stato di una partita di tris
typedef struct {
    Cella cella[SIZE][SIZE]; // Griglia di gioco
    char currentPlayer;     // X or O
    int movesCount;          // Numero di mosse effettuate
} TrisGame;


// Inizializza una nuova partita di tris
void iniziaPartita(TrisGame *game);

// Controlla se c'è un vincitore o se la partita è in corso; restituisce il risultato
Risultato controllaVincitore(TrisGame *game);

// Esegue una mossa nella cella (row, col); restituisce 0 se valida, -1 altrimenti
int faiMossa(TrisGame *game, int row, int col);

// Stampa il tabellone di gioco in un buffer di stringa
// `bufsize` è la dimensione di `buffer` per evitare overflow
void stampaTabellone(TrisGame *game, char *buffer, size_t bufsize);

#endif 