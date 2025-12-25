#include "ttt_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>


static void show_board(Board b){
    uint16_t x = ttt_bits_x(b), o = ttt_bits_o(b);
    printf("    a   b   c\n"); // Column coordinates
    puts("  +---+---+---+");
    for (int r = 0; r < 3; ++r) {
        printf("%d |", r + 1); // Row coordinate
        for (int c = 0; c < 3; ++c) {
            int i = r*3 + c;
            char ch = (x & (1u<<i)) ? 'X' : (o & (1u<<i)) ? 'O' : ' ';
            printf(" %c |", ch);
        }
        puts(""); // Newline after each row
        puts("  +---+---+---+"); // Row separator
    }
}


static int read_move(void){
    char buf[64];
    if (!fgets(buf, sizeof buf, stdin)) return TTT_PARSE_EOF;
    return ttt_parse_move(buf);
}

static void usage(const char *prog){
    fprintf(stderr, "Usage: %s [--ai X|O|none]\n", prog);
    fprintf(stderr, "Enter moves as 0..8 or algebraic a1..c3 (a1=top-left)\n");
}

static int get_human_move(Board b) {
    while (true) {
        printf("\nPlayer %c, your move (0-8 or a1..c3): ", token(ttt_side_to_move(b)));
        fflush(stdout);
        int mv = read_move();
        if (mv == TTT_PARSE_EOF) { // EOF or read error
             return -1; // Internal EOF signal
        }
        if (mv == TTT_PARSE_INVALID_FORMAT) {
            fprintf(stderr, "Invalid format. Enter 0-8 or a1-c3.\n");
            continue;
        }
        if (mv == TTT_PARSE_OUT_OF_RANGE) {
            fprintf(stderr, "Move out of range. Enter 0-8 or a1-c3.\n");
            continue;
        }
        if (!ttt_is_legal(b, mv)) {
            fprintf(stderr, "Illegal move (square occupied or invalid).\n");
            continue;
        }
        return mv;
    }
}

static int parse_cli_arguments(int argc, const char *const *argv, ttt_side *ai_player) {
    *ai_player = (ttt_side)2; // Default to NONE
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--ai") == 0) {
            if (i+1 >= argc) return (usage(argv[0]), 1);
            const char *v = argv[++i];
            if (v[0]=='X' || v[0]=='x') { *ai_player = TTT_X; }
            else if (v[0]=='O' || v[0]=='o' || v[0]=='0') { *ai_player = TTT_O; }
            else if (v[0]=='n' || v[0]=='N') { /* human vs human, ai_player remains NONE */ }
            else return (usage(argv[0]), 1);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            return (usage(argv[0]), 0);
        } else {
            return (usage(argv[0]), 1);
        }
    }
    return 0; // Success
}

static int run_game(ttt_side ai_player) {
    ttt_reset_cache();
    Board b = ttt_initial();
    bool ai_active = (ai_player != (ttt_side)2); 

    printf("Welcome to Tic-Tac-Toe!\n\n");

    while (true) {
        show_board(b);

        ttt_score s;
        if (ttt_is_terminal(b, &s)) {
            printf("\n--- GAME OVER ---\n");
            if (s == TTT_DRAW) {
                printf("It's a draw!\n");
            } else {
                printf("Player %c wins!\n", token((ttt_side)((ttt_side_to_move(b)^1))));
            }
            printf("-----------------\n");
            break;
        }

        int mv = -1;
        if (ai_active && ttt_side_to_move(b) == ai_player) {
            printf("\nAI is playing...\n");
            mv = ttt_best_move(b);
            printf("AI played on square %d\n", mv);
        } else {
            mv = get_human_move(b);
            if (mv == -1) { // EOF or read error
                printf("\nExiting game.\n");
                break;
            }
        }
        
        b = ttt_apply(b, mv);
    }
    return 0;
}

int main(int argc, const char *const *argv){
    ttt_side ai = (ttt_side)2; // 2 == NONE here in CLI

    if (parse_cli_arguments(argc, argv, &ai) != 0) {
        return 1; // Error during argument parsing or help requested
    }
    
    return run_game(ai);
}
