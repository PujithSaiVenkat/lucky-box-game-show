#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game.h"

/* ====================================================================== */
/*  Setup                                                                 */
/* ====================================================================== */

int load_prizes(const char *filename, long prizes[], int max_boxes)
{
    FILE *fp;
    int count = 0;
    long amount;

    if (filename == NULL || prizes == NULL || max_boxes <= 0) {
        return -1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return -1;
    }

    while (count < max_boxes && fscanf(fp, "%ld", &amount) == 1) {
        if (amount < 0) {
            fprintf(stderr, "Invalid prize amount in %s: amounts cannot be negative.\n",
                    filename);
            fclose(fp);
            return -1;
        }
        prizes[count++] = amount;
    }

    if (ferror(fp)) {
        perror("Error reading prize data");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    if (count == 0) {
        fprintf(stderr, "No prize amounts found in %s.\n", filename);
        return -1;
    }

    return count;
}

void shuffle_prizes(long prizes[], int count)
{
    int i;

    if (prizes == NULL || count <= 1) {
        return;
    }

    for (i = count - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        long temp = prizes[i];
        prizes[i] = prizes[j];
        prizes[j] = temp;
    }
}

void game_init(GameState *g, const long prizes[], int count, int chosen_box)
{
    memset(g, 0, sizeof(*g));
    memcpy(g->prizes, prizes, (size_t)count * sizeof(long));
    g->count = count;
    g->player_box = chosen_box;
    g->initial_box = chosen_box;
    g->multiplier_pct = 100;
}

/* ====================================================================== */
/*  Board                                                                 */
/* ====================================================================== */

void show_available_boxes(const int available[], int count)
{
    int i;

    printf("\nAvailable boxes: ");
    for (i = 0; i < count; ++i) {
        printf("%d", available[i]);
        if (i < count - 1) {
            printf("  ");
        }
    }
    printf("\n");
}

int list_closed_boxes(const GameState *g, int out[],
                      int include_player, int exclude_peeked)
{
    int box;
    int n = 0;

    for (box = 1; box <= g->count; ++box) {
        if (g->is_open[box - 1]) {
            continue;
        }
        if (box == g->player_box && !include_player) {
            continue;
        }
        if (exclude_peeked && g->is_peeked[box - 1]) {
            continue;
        }
        out[n++] = box;
    }
    return n;
}

int others_closed(const GameState *g)
{
    int tmp[MAX_BOXES];
    return list_closed_boxes(g, tmp, 0, 0);
}

void show_board(const GameState *g)
{
    long remaining[MAX_BOXES];
    int n = 0;
    int box, i, j;

    printf("\n------------------------------------\n");
    for (box = 1; box <= g->count; ++box) {
        printf("Box %2d : ", box);
        if (g->is_open[box - 1]) {
            printf("OPENED - Rs. %ld\n", g->prizes[box - 1]);
            continue;
        }

        remaining[n++] = g->prizes[box - 1];
        if (box == g->player_box) {
            printf("sealed  << YOUR BOX");
        } else {
            printf("sealed");
        }
        if (g->is_peeked[box - 1]) {
            printf("  (peeked: Rs. %ld)", g->prizes[box - 1]);
        }
        printf("\n");
    }

    /* Sort remaining prizes so the board does not reveal box positions. */
    for (i = 1; i < n; ++i) {
        long key = remaining[i];
        j = i - 1;
        while (j >= 0 && remaining[j] > key) {
            remaining[j + 1] = remaining[j];
            --j;
        }
        remaining[j + 1] = key;
    }

    printf("Prizes still in play: ");
    for (i = 0; i < n; ++i) {
        printf("Rs. %ld%s", remaining[i], i < n - 1 ? ", " : "");
    }
    printf("\n------------------------------------\n");
}

int open_box(GameState *g, int box, long *prize)
{
    if (g == NULL || box < 1 || box > g->count ||
        box == g->player_box || g->is_open[box - 1]) {
        return -1;
    }

    g->is_open[box - 1] = 1;
    g->opened_count++;
    if (prize != NULL) {
        *prize = g->prizes[box - 1];
    }
    return 0;
}

/* ====================================================================== */
/*  Banker's Deal                                                         */
/* ====================================================================== */

int banker_pending(GameState *g)
{
    static const int thresholds[2] = { BANKER_FIRST_OFFER, BANKER_SECOND_OFFER };
    int round = -1;
    int i;

    /* With one alternative left the final keep/swap choice comes next. */
    if (others_closed(g) < 2) {
        return -1;
    }

    /* If several thresholds were crossed at once (e.g. by 50:50) the
       banker only calls once, using the latest round. */
    for (i = 0; i < 2; ++i) {
        if (!g->offer_made[i] && g->opened_count >= thresholds[i]) {
            g->offer_made[i] = 1;
            round = i;
        }
    }
    return round;
}

long banker_offer(const GameState *g, int round)
{
    int boxes[MAX_BOXES];
    int n = list_closed_boxes(g, boxes, 1, 0);
    long sum = 0;
    long average, offer;
    int pct, i;

    if (n == 0) {
        return 0;
    }

    for (i = 0; i < n; ++i) {
        sum += g->prizes[boxes[i] - 1];
    }
    average = sum / n;

    /* The banker offers less than the average early on and gets more
       generous on the second call, with a little randomness. */
    pct = (round == 0 ? 60 : 80) + rand() % 11;
    offer = average * pct / 100;

    /* Round to the nearest Rs. 10 so offers look tidy. */
    return (offer + 5) / 10 * 10;
}

/* ====================================================================== */
/*  Lifelines                                                             */
/* ====================================================================== */

int use_fifty_fifty(GameState *g, int eliminated[2])
{
    int boxes[MAX_BOXES];
    int n, i, k;

    if (g->used_fifty_fifty) {
        return -1;
    }

    n = list_closed_boxes(g, boxes, 0, 0);
    if (n < 4) {
        return -1;
    }

    /* Pick the two lowest-value closed boxes. */
    for (k = 0; k < 2; ++k) {
        int best = -1;
        for (i = 0; i < n; ++i) {
            if (g->is_open[boxes[i] - 1]) {
                continue;
            }
            if (best == -1 ||
                g->prizes[boxes[i] - 1] < g->prizes[best - 1]) {
                best = boxes[i];
            }
        }
        eliminated[k] = best;
        g->is_open[best - 1] = 1;
        g->opened_count++;
    }

    g->used_fifty_fifty = 1;
    return 2;
}

int use_box_swap(GameState *g, int new_box)
{
    if (g->used_box_swap || new_box < 1 || new_box > g->count ||
        new_box == g->player_box || g->is_open[new_box - 1]) {
        return -1;
    }

    g->player_box = new_box;
    g->used_box_swap = 1;
    return 0;
}

int use_prize_peek(GameState *g, int box, long *prize)
{
    if (g->used_prize_peek || box < 1 || box > g->count ||
        box == g->player_box || g->is_open[box - 1] ||
        g->is_peeked[box - 1]) {
        return -1;
    }

    g->is_peeked[box - 1] = 1;
    g->used_prize_peek = 1;
    if (prize != NULL) {
        *prize = g->prizes[box - 1];
    }
    return 0;
}

/* ====================================================================== */
/*  Mystery Box, Lucky Spin, Double or Nothing                            */
/* ====================================================================== */

typedef struct {
    const char *message;
    long delta;
} MysteryOutcome;

static const MysteryOutcome mystery_outcomes[] = {
    { "JACKPOT! A bonus of Rs. 5000!",            5000 },
    { "Lucky you! A bonus of Rs. 2500.",          2500 },
    { "Nice! A bonus of Rs. 1000.",               1000 },
    { "A small bonus of Rs. 500.",                 500 },
    { "The box is empty. Nothing happens.",          0 },
    { "Oops! A penalty of Rs. 500.",              -500 },
    { "Ouch! A penalty of Rs. 1000.",            -1000 },
    { "Disaster! A penalty of Rs. 2500.",        -2500 }
};

int use_mystery_box(GameState *g, long *delta, const char **message)
{
    int count = (int)(sizeof(mystery_outcomes) / sizeof(mystery_outcomes[0]));
    const MysteryOutcome *o;

    if (g->used_mystery_box) {
        return -1;
    }

    o = &mystery_outcomes[rand() % count];
    g->bonus_cash += o->delta;
    g->used_mystery_box = 1;

    if (delta != NULL) {
        *delta = o->delta;
    }
    if (message != NULL) {
        *message = o->message;
    }
    return 0;
}

typedef struct {
    const char *label;
    long cash;          /* cash reward, 0 if none            */
    int  mult_pct;      /* bonus multiplier, 0 if none       */
} SpinSlice;

static const SpinSlice spin_slices[] = {
    { "Rs. 500 cash",            500,   0 },
    { "Rs. 1000 cash",          1000,   0 },
    { "Rs. 2500 cash",          2500,   0 },
    { "Rs. 5000 cash",          5000,   0 },
    { "1.5x BONUS MULTIPLIER",     0, 150 },
    { "2x BONUS MULTIPLIER",       0, 200 },
    { "Better luck next time",     0,   0 }
};

int use_lucky_spin(GameState *g, long *cash, int *mult_pct,
                   const char **label)
{
    int count = (int)(sizeof(spin_slices) / sizeof(spin_slices[0]));
    const SpinSlice *s;

    if (g->used_lucky_spin) {
        return -1;
    }

    s = &spin_slices[rand() % count];
    g->bonus_cash += s->cash;
    if (s->mult_pct > 0) {
        g->multiplier_pct = s->mult_pct;
    }
    g->used_lucky_spin = 1;

    if (cash != NULL) {
        *cash = s->cash;
    }
    if (mult_pct != NULL) {
        *mult_pct = s->mult_pct;
    }
    if (label != NULL) {
        *label = s->label;
    }
    return 0;
}

long compute_winnings(const GameState *g, long base_prize)
{
    long subtotal = base_prize + g->bonus_cash;

    if (subtotal < 0) {
        subtotal = 0;
    }
    return subtotal * g->multiplier_pct / 100;
}

int double_or_nothing(long *winnings, int guess, int *winning_number)
{
    int secret = (rand() % 2) + 1;

    if (winning_number != NULL) {
        *winning_number = secret;
    }

    if (guess == secret) {
        *winnings *= 2;
        return 1;
    }

    *winnings = 0;
    return 0;
}

/* ====================================================================== */
/*  Results                                                               */
/* ====================================================================== */

void describe_features(const GameState *g, char *buf, size_t size)
{
    size_t len = 0;
    const struct { int used; const char *name; } items[] = {
        { g->used_fifty_fifty, "50:50"       },
        { g->used_box_swap,    "Box Swap"    },
        { g->used_prize_peek,  "Prize Peek"  },
        { g->used_mystery_box, "Mystery Box" },
        { g->used_lucky_spin,  "Lucky Spin"  }
    };
    size_t i;

    if (size == 0) {
        return;
    }
    buf[0] = '\0';

    for (i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
        int written;
        if (!items[i].used) {
            continue;
        }
        written = snprintf(buf + len, size - len, "%s%s",
                           len > 0 ? ", " : "", items[i].name);
        if (written < 0 || (size_t)written >= size - len) {
            return;
        }
        len += (size_t)written;
    }

    if (len == 0) {
        snprintf(buf, size, "None");
    }
}

int save_result(const char *filename, const GameResult *r)
{
    FILE *fp;
    time_t now;
    struct tm *local_time;
    char timestamp[32];
    char final_box[16];
    char multiplier[16];
    const char *double_text;

    if (filename == NULL || r == NULL) {
        return -1;
    }

    now = time(NULL);
    local_time = localtime(&now);
    if (local_time == NULL ||
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
                 local_time) == 0) {
        snprintf(timestamp, sizeof(timestamp), "unknown-time");
    }

    snprintf(final_box, sizeof(final_box), "%d", r->final_box);
    snprintf(multiplier, sizeof(multiplier), "x%d.%02d",
             r->multiplier_pct / 100, r->multiplier_pct % 100);
    double_text = r->double_result == 1 ? "Won" :
                  r->double_result == 2 ? "Lost" : "Not played";

    fp = fopen(filename, "a");
    if (fp == NULL) {
        perror(filename);
        return -1;
    }

    if (fprintf(fp, "%s | Initially selected box: %d | Final box: %s | "
                    "Prize: %ld | Swapped: %s | Banker deal: ",
                timestamp, r->initial_box, final_box,
                r->box_prize, r->swapped ? "Yes" : "No") < 0) {
        perror("Error writing game result");
        fclose(fp);
        return -1;
    }

    if (r->banker_deal) {
        fprintf(fp, "Yes (Rs. %ld)", r->banker_offer);
    } else {
        fprintf(fp, "No");
    }

    if (fprintf(fp, " | Features: %s | Bonus cash: %+ld | Multiplier: %s | "
                    "Double or Nothing: %s | Total won: %ld\n",
                r->features, r->bonus_cash, multiplier, double_text,
                r->total) < 0) {
        perror("Error writing game result");
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        perror("Error closing result file");
        return -1;
    }

    return 0;
}
