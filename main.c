#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "game.h"

#define DATA_FILE "data.txt"
#define RESULTS_FILE "results.txt"

/* ====================================================================== */
/*  Input helpers                                                         */
/* ====================================================================== */

/* Reads one line, strips the newline and discards any overflow.
   Returns 0 on end of input. */
static int read_line(char *buf, size_t size)
{
    size_t len;

    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* discard the rest of an over-long line */
        }
    }
    if (len > 0 && buf[len - 1] == '\r') {
        buf[len - 1] = '\0';
    }
    return 1;
}

/* Asks for a number in [min, max]; quits the game if input ends. */
static int read_int(const char *prompt, int min, int max)
{
    char line[100];
    char *end;
    long value;

    for (;;) {
        printf("%s", prompt);
        if (!read_line(line, sizeof(line))) {
            printf("\nInput ended. Game cancelled.\n");
            exit(1);
        }

        errno = 0;
        value = strtol(line, &end, 10);
        while (isspace((unsigned char)*end)) {
            ++end;
        }

        if (errno == 0 && end != line && *end == '\0' &&
            value >= min && value <= max) {
            return (int)value;
        }

        printf("Invalid input. Enter a number from %d to %d.\n", min, max);
    }
}

/* Returns 1 for yes, 0 for no (or if input ends). */
static int ask_yes_no(const char *prompt)
{
    char line[40];
    size_t i;

    for (;;) {
        printf("%s", prompt);
        if (!read_line(line, sizeof(line))) {
            printf("\nInput ended. Answering no.\n");
            return 0;
        }

        for (i = 0; line[i] != '\0'; ++i) {
            line[i] = (char)tolower((unsigned char)line[i]);
        }

        if (strcmp(line, "yes") == 0 || strcmp(line, "y") == 0) {
            return 1;
        }
        if (strcmp(line, "no") == 0 || strcmp(line, "n") == 0) {
            return 0;
        }
        printf("Please answer yes or no.\n");
    }
}

/* Lets the player pick one of the other closed boxes. */
static int pick_box(const GameState *g, const char *prompt, int exclude_peeked)
{
    int boxes[MAX_BOXES];
    int n, choice, i;

    n = list_closed_boxes(g, boxes, 0, exclude_peeked);
    for (;;) {
        show_available_boxes(boxes, n);
        choice = read_int(prompt, 1, g->count);
        for (i = 0; i < n; ++i) {
            if (boxes[i] == choice) {
                return choice;
            }
        }
        printf("Box %d is not available.\n", choice);
    }
}

/* ====================================================================== */
/*  Game actions                                                          */
/* ====================================================================== */

static void print_intro(int count)
{
    printf("\n====================================\n");
    printf("LUCKY BOX GAME SHOW SPONSORED BY SRM AP\n");
    printf("====================================\n");
    printf("There are %d sealed boxes. Each box contains a different hidden prize.\n",
           count);
    printf("Choose one box and keep it safe, then open the other boxes one by one.\n");
    printf("\n");
    printf("  * Banker's Deal - the banker calls after %d and %d boxes are opened.\n",
           BANKER_FIRST_OFFER, BANKER_SECOND_OFFER);
    printf("  * 50:50         - removes the two lowest-value boxes.\n");
    printf("  * Box Swap      - exchange your box for another sealed box.\n");
    printf("  * Prize Peek    - see what is inside another sealed box.\n");
    printf("  * Mystery Box   - a random bonus or penalty.\n");
    printf("  * Lucky Spin    - cash rewards and a bonus multiplier.\n");
    printf("  * Double or Nothing - guess 1 or 2 at the end to double your winnings.\n");
    printf("Each feature can be used once per game. When two boxes remain\n");
    printf("you can keep your box or swap to the last one.\n");
}

static void show_menu(const GameState *g)
{
    printf("\nWhat would you like to do?\n");
    printf("  1) Open a box\n");
    printf("  2) 50:50       %s\n", g->used_fifty_fifty ? "[used]" : "[ready]");
    printf("  3) Box Swap    %s\n", g->used_box_swap    ? "[used]" : "[ready]");
    printf("  4) Prize Peek  %s\n", g->used_prize_peek  ? "[used]" : "[ready]");
    printf("  5) Mystery Box %s\n", g->used_mystery_box ? "[used]" : "[ready]");
    printf("  6) Lucky Spin  %s\n", g->used_lucky_spin  ? "[used]" : "[ready]");
}

static void do_open_box(GameState *g)
{
    long prize;
    int box = pick_box(g, "Open which box? ", 0);

    open_box(g, box, &prize);
    printf("\nBox %d contains: Rs. %ld\n", box, prize);
}

static void do_fifty_fifty(GameState *g)
{
    int eliminated[2];

    if (g->used_fifty_fifty) {
        printf("\nYou have already used 50:50.\n");
        return;
    }
    if (use_fifty_fifty(g, eliminated) < 0) {
        printf("\n50:50 needs at least 4 other sealed boxes.\n");
        return;
    }

    printf("\n50:50 activated! The host removes the two lowest-value boxes:\n");
    printf("Box %d contains: Rs. %ld\n", eliminated[0], g->prizes[eliminated[0] - 1]);
    printf("Box %d contains: Rs. %ld\n", eliminated[1], g->prizes[eliminated[1] - 1]);
}

static void do_box_swap(GameState *g)
{
    int old_box = g->player_box;
    int new_box;

    if (g->used_box_swap) {
        printf("\nYou have already used Box Swap.\n");
        return;
    }

    new_box = pick_box(g, "Swap your box for which box? ", 0);
    use_box_swap(g, new_box);

    printf("\nYou swapped box %d for box %d. Box %d is now your box.\n",
           old_box, new_box, new_box);
    if (g->is_peeked[new_box - 1]) {
        printf("You peeked at this box earlier - it holds Rs. %ld!\n",
               g->prizes[new_box - 1]);
    }
}

static void do_prize_peek(GameState *g)
{
    long prize;
    int boxes[MAX_BOXES];
    int box;

    if (g->used_prize_peek) {
        printf("\nYou have already used Prize Peek.\n");
        return;
    }
    if (list_closed_boxes(g, boxes, 0, 1) == 0) {
        printf("\nThere are no sealed boxes left to peek into.\n");
        return;
    }

    box = pick_box(g, "Peek into which box? ", 1);
    use_prize_peek(g, box, &prize);
    printf("\nPrize Peek: box %d contains Rs. %ld. It stays sealed.\n",
           box, prize);
}

static void do_mystery_box(GameState *g)
{
    long delta;
    const char *message;

    if (use_mystery_box(g, &delta, &message) < 0) {
        printf("\nYou have already opened the Mystery Box.\n");
        return;
    }

    printf("\nYou open the Mystery Box...\n");
    printf("%s\n", message);
    printf("Your running bonus is now Rs. %+ld.\n", g->bonus_cash);
}

static void do_lucky_spin(GameState *g)
{
    long cash;
    int mult_pct;
    const char *label;

    if (use_lucky_spin(g, &cash, &mult_pct, &label) < 0) {
        printf("\nYou have already used your Lucky Spin.\n");
        return;
    }

    printf("\nThe wheel spins... and lands on: %s!\n", label);
    if (cash > 0) {
        printf("Cash reward added. Your running bonus is now Rs. %+ld.\n",
               g->bonus_cash);
    } else if (mult_pct > 0) {
        printf("Your final winnings will be multiplied by x%d.%02d.\n",
               mult_pct / 100, mult_pct % 100);
    }
}

/* Returns 1 if the player accepted a deal (offer stored in *accepted). */
static int handle_banker(GameState *g, long *accepted)
{
    int round = banker_pending(g);
    long offer;

    if (round < 0) {
        return 0;
    }

    offer = banker_offer(g, round);

    printf("\n====================================\n");
    printf("          THE BANKER IS CALLING     \n");
    printf("====================================\n");
    show_board(g);
    printf("The banker offers you Rs. %ld for your box.\n", offer);

    if (ask_yes_no("Deal? (yes/no): ")) {
        *accepted = offer;
        return 1;
    }

    printf("\nNo deal! The game continues.\n");
    return 0;
}

static void print_multiplier_line(const GameState *g)
{
    if (g->multiplier_pct != 100) {
        printf("Bonus multiplier: x%d.%02d\n",
               g->multiplier_pct / 100, g->multiplier_pct % 100);
    }
}

/* ====================================================================== */
/*  Main                                                                  */
/* ====================================================================== */

int main(void)
{
    long prizes[MAX_BOXES];
    GameState g;
    GameResult r;
    long offer = 0;
    long base_prize;
    long total;
    int count;
    int chosen_box;
    int final_box;
    int swapped = 0;
    int deal = 0;
    int choice;

    srand((unsigned int)time(NULL));

    count = load_prizes(DATA_FILE, prizes, MAX_BOXES);
    if (count < 3) {
        fprintf(stderr, "The game needs at least 3 prize amounts in %s.\n",
                DATA_FILE);
        return 1;
    }

    shuffle_prizes(prizes, count);
    print_intro(count);

    /* Show the boxes and let the player pick one. */
    {
        int all[MAX_BOXES];
        int i;
        for (i = 0; i < count; ++i) {
            all[i] = i + 1;
        }
        show_available_boxes(all, count);
    }
    chosen_box = read_int("Choose your box: ", 1, count);
    game_init(&g, prizes, count, chosen_box);
    printf("\nYou selected box %d. Keep it safe!\n", chosen_box);

    /* Opening rounds: continue until one other box is left. */
    while (others_closed(&g) > 1) {
        if (handle_banker(&g, &offer)) {
            deal = 1;
            break;
        }

        show_board(&g);
        show_menu(&g);
        choice = read_int("Your choice: ", 1, 6);

        switch (choice) {
        case 1: do_open_box(&g);    break;
        case 2: do_fifty_fifty(&g); break;
        case 3: do_box_swap(&g);    break;
        case 4: do_prize_peek(&g);  break;
        case 5: do_mystery_box(&g); break;
        case 6: do_lucky_spin(&g);  break;
        }
    }

    memset(&r, 0, sizeof(r));
    r.initial_box = g.initial_box;

    if (deal) {
        final_box = g.player_box;
        base_prize = offer;
        printf("\nYou accepted the banker's deal of Rs. %ld!\n", offer);
        printf("Your box %d contained: Rs. %ld\n", final_box,
               g.prizes[final_box - 1]);
        r.banker_deal = 1;
        r.banker_offer = offer;
    } else {
        int boxes[MAX_BOXES];

        list_closed_boxes(&g, boxes, 0, 0);
        final_box = boxes[0];

        show_board(&g);
        printf("\nYour box %d is still sealed. The last unopened alternative is box %d.\n",
               g.player_box, final_box);
        if (g.is_peeked[final_box - 1]) {
            printf("(You peeked at box %d earlier: Rs. %ld.)\n", final_box,
                   g.prizes[final_box - 1]);
        }

        if (ask_yes_no("Do you want to swap to the last box? (yes/no): ")) {
            swapped = 1;
            printf("\nYou swapped from box %d to box %d.\n",
                   g.player_box, final_box);
        } else {
            final_box = g.player_box;
            printf("\nYou decided to keep box %d.\n", final_box);
        }
        base_prize = g.prizes[final_box - 1];
    }

    total = compute_winnings(&g, base_prize);

    printf("\n====================================\n");
    printf("               RESULT               \n");
    printf("====================================\n");
    printf("Your final box: %d\n", final_box);
    printf("%s: Rs. %ld\n", deal ? "Banker's deal" : "Prize in box", base_prize);
    if (g.bonus_cash != 0) {
        printf("Bonus cash (Mystery Box / Lucky Spin): Rs. %+ld\n", g.bonus_cash);
    }
    print_multiplier_line(&g);
    printf("You won: Rs. %ld\n", total);
    printf("====================================\n");

    r.double_result = 0;

    /* Double or Nothing */
    if (total > 0) {
        printf("\nDOUBLE OR NOTHING!\n");
        printf("Guess 1 or 2. Get it right and your winnings double to Rs. %ld.\n",
               total * 2);
        printf("Get it wrong and you lose everything.\n");

        if (ask_yes_no("Play Double or Nothing? (yes/no): ")) {
            int guess = read_int("Your guess (1 or 2): ", 1, 2);
            int secret;

            if (double_or_nothing(&total, guess, &secret)) {
                printf("\nThe number was %d - you guessed right!\n", secret);
                printf("Your winnings are doubled to Rs. %ld!\n", total);
                r.double_result = 1;
            } else {
                printf("\nThe number was %d - wrong guess.\n", secret);
                printf("You lose everything. Final winnings: Rs. 0.\n");
                r.double_result = 2;
            }
        }
    }

    printf("\n====================================\n");
    printf("Final winnings: Rs. %ld\n", total);
    printf("====================================\n");

    r.final_box = final_box;
    r.box_prize = base_prize;
    r.swapped = swapped;
    r.bonus_cash = g.bonus_cash;
    r.multiplier_pct = g.multiplier_pct;
    r.total = total;
    describe_features(&g, r.features, sizeof(r.features));

    if (save_result(RESULTS_FILE, &r) != 0) {
        fprintf(stderr, "Warning: the result could not be saved.\n");
    } else {
        printf("Game result saved to %s.\n", RESULTS_FILE);
    }

    return 0;
}
