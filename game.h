#ifndef GAME_H
#define GAME_H

#include <stddef.h>

#define MAX_BOXES 20

/* The banker phones after this many boxes have been opened. */
#define BANKER_FIRST_OFFER  2
#define BANKER_SECOND_OFFER 4

/* Everything that changes while a game is being played. */
typedef struct {
    long prizes[MAX_BOXES];     /* hidden prize in each box (index = box - 1) */
    int  count;                 /* number of boxes in this game               */
    int  is_open[MAX_BOXES];    /* 1 = revealed and out of play               */
    int  is_peeked[MAX_BOXES];  /* 1 = player used Prize Peek on this box     */
    int  player_box;            /* box the player currently holds (1-based)   */
    int  initial_box;           /* box the player originally chose            */
    int  opened_count;          /* boxes opened/eliminated so far             */
    int  offer_made[2];         /* banker offers already handed out           */

    /* One-use features. */
    int  used_fifty_fifty;
    int  used_box_swap;
    int  used_prize_peek;
    int  used_mystery_box;
    int  used_lucky_spin;

    /* Bonuses collected on the way. */
    long bonus_cash;            /* net cash from Mystery Box + Lucky Spin     */
    int  multiplier_pct;        /* 100 = x1.00, 150 = x1.50, 200 = x2.00      */
} GameState;

/* Summary of a finished game, written to the results file. */
typedef struct {
    int  initial_box;
    int  final_box;             /* box held at the end                        */
    long box_prize;             /* prize that was inside that box             */
    int  swapped;               /* 1 = swapped with the last box at the end   */
    int  banker_deal;           /* 1 = player accepted a banker's offer       */
    long banker_offer;
    long bonus_cash;
    int  multiplier_pct;
    int  double_result;         /* 0 = not played, 1 = won, 2 = lost          */
    long total;                 /* final winnings                             */
    char features[96];          /* names of the features that were used       */
} GameResult;

/* ---- Setup ---------------------------------------------------------- */

/* Loads prize amounts from a text file. Returns the number loaded,
   or -1 if the file cannot be opened / contains invalid data. */
int load_prizes(const char *filename, long prizes[], int max_boxes);

/* Randomly rearranges the prize amounts. */
void shuffle_prizes(long prizes[], int count);

/* Starts a new game with the player holding chosen_box. */
void game_init(GameState *g, const long prizes[], int count, int chosen_box);

/* ---- Board ---------------------------------------------------------- */

/* Displays the numbered boxes that are still available. */
void show_available_boxes(const int available[], int count);

/* Prints every box (sealed / opened / yours) and the prizes still in play. */
void show_board(const GameState *g);

/* Fills out[] with closed boxes. Returns how many were written.
   include_player: 1 = also list the player's own box
   exclude_peeked: 1 = skip boxes already seen with Prize Peek */
int list_closed_boxes(const GameState *g, int out[],
                      int include_player, int exclude_peeked);

/* Number of closed boxes other than the player's. */
int others_closed(const GameState *g);

/* Opens a closed box (not the player's). Returns 0 and the prize, or -1. */
int open_box(GameState *g, int box, long *prize);

/* ---- Banker's Deal -------------------------------------------------- */

/* Returns 0 or 1 (first / second offer) if the banker is due to call now,
   otherwise -1. Each offer is only reported once. */
int banker_pending(GameState *g);

/* The banker's cash offer for the given offer round (0 or 1). */
long banker_offer(const GameState *g, int round);

/* ---- Lifelines ------------------------------------------------------ */

/* 50:50 - removes the two lowest-value closed boxes (never the player's).
   Needs at least 4 other closed boxes. Returns 2 on success, -1 otherwise. */
int use_fifty_fifty(GameState *g, int eliminated[2]);

/* Box Swap - exchange your box for another closed box. Returns 0 or -1. */
int use_box_swap(GameState *g, int new_box);

/* Prize Peek - reveal what is in another closed box without opening it.
   Returns 0 and the prize, or -1. */
int use_prize_peek(GameState *g, int box, long *prize);

/* ---- Bonus features ------------------------------------------------- */

/* Mystery Box - random bonus or penalty. Returns 0 on success, -1 if
   already used. */
int use_mystery_box(GameState *g, long *delta, const char **message);

/* Lucky Spin - cash reward or bonus multiplier. Sets *cash and/or
   *mult_pct (0 if that kind of prize was not won). Returns 0 or -1. */
int use_lucky_spin(GameState *g, long *cash, int *mult_pct,
                   const char **label);

/* Applies bonus cash and multiplier to a base prize (never below 0). */
long compute_winnings(const GameState *g, long base_prize);

/* Double or Nothing - the player guesses 1 or 2. Doubles *winnings on a
   correct guess, sets it to 0 otherwise. Returns 1 if won, 0 if lost. */
int double_or_nothing(long *winnings, int guess, int *winning_number);

/* ---- Results -------------------------------------------------------- */

/* Writes the names of the used features (e.g. "50:50, Prize Peek"). */
void describe_features(const GameState *g, char *buf, size_t size);

/* Appends the completed game's result to a file. Returns 0 on success. */
int save_result(const char *filename, const GameResult *r);

#endif
