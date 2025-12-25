#include "ttt_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>



static void show_board(Board b){
    uint16_t x = ttt_bits_x(b), o = ttt_bits_o(b);
    for (int r = 0; r < 3; ++r) {
        puts("+---+---+---+");
        printf("|\n");
        for (int c = 0; c < 3; ++c) {
            int i = r*3 + c;
            char ch = (x & (1u<<i)) ? 'X' : (o & (1u<<i)) ? 'O' : ' ';
            printf(" %c |", ch);
        }
        puts("");
    }
    puts("+---+---+---+");
}

static int parse_move(const char *s){
    while (*s && isspace((unsigned char)*s)) ++s;
    if (isdigit((unsigned char)*s)) {
        char *e = NULL; long v = strtol(s, &e, 10);
        if (e == s) return -1; // Invalid format (no digits parsed)
        if (v < 0 || v > 8) return -2; // Out of range
        return (int)v;
    }
    // Algebraic input
    if (((s[0]|32) >= 'a' && (s[0]|32) <= 'c') && (s[1] >= '1' && s[1] <= '3')) {
        int col = (s[0] | 32) - 'a'; int row = s[1] - '1';
        return row*3 + col;
    }
    return -1; // Invalid format (neither digit nor algebraic)
}

static int read_move(void){
    char buf[64];
    if (!fgets(buf, sizeof buf, stdin)) return -3;
    return parse_move(buf);
}

static void usage(const char *prog){
    fprintf(stderr, "Usage: %s [--ai X|O|none]\n", prog);
    fprintf(stderr, "Enter moves as 0..8 or algebraic a1..c3 (a1=top-left)\n");
}

static int get_human_move(Board b) {
    while (true) {
        printf("Player %c, your move (0-8 or a1..c3): ", token(ttt_side_to_move(b)));
        fflush(stdout);
        int mv = read_move();
        if (mv == -3) { // EOF or read error
             return -1;
        }
        if (mv == -1) { fprintf(stderr, "Invalid format. Enter 0-8 or a1-c3.\n"); continue; }
        if (mv == -2) { fprintf(stderr, "Move out of range. Enter 0-8 or a1-c3.\n"); continue; }
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

    while (true) {
        ttt_score s;
        if (ttt_is_terminal(b, &s)) {
            show_board(b);
            if (s == TTT_DRAW) puts("Draw");
            else printf("%c wins\n", token((ttt_side)((ttt_side_to_move(b)^1))));
            break;
        }

        int mv = -1;
        if (ai_active && ttt_side_to_move(b) == ai_player) {
            mv = ttt_best_move(b);
        } else {
            show_board(b);
            printf("Player %c, your move (0-8 or a1..c3): ", token(ttt_side_to_move(b)));
            mv = read_move();
            if (mv == -1) { /* EOF or read error */ break; }
            if (mv == -1) { fprintf(stderr, "Invalid format. Enter 0-8 or a1-c3.\n"); continue; }
            if (mv == -2) { fprintf(stderr, "Move out of range. Enter 0-8 or a1-c3.\n"); continue; }
            if (!ttt_is_legal(b, mv)) {
                fprintf(stderr, "Illegal move (square occupied or invalid).\n");
                continue;
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
