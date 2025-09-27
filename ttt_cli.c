// ttt_cli.c — C23 CLI wrapper (algebraic or numeric input, self-test)
#include "ttt_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static inline char token(ttt_side s){ return s == TTT_X ? 'X' : 'O'; }

static void show_board(Board b){
    uint16_t x = ttt_bits_x(b), o = ttt_bits_o(b);
    for (int r = 0; r < 3; ++r) {
        puts("+---+---+---+");
        printf("|");
        for (int c = 0; c < 3; ++c) {
            int i = r*3 + c;
            char ch = (x & (1u<<i)) ? 'X' : (o & (1u<<i)) ? 'O' : (char)('0' + i);
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
        return (e == s || v < 0 || v > 8) ? -1 : (int)v;
    }
    if (((s[0]|32) >= 'a' && (s[0]|32) <= 'c') && (s[1] >= '1' && s[1] <= '3')) {
        int col = (s[0] | 32) - 'a'; int row = s[1] - '1';
        return row*3 + col;
    }
    return -1;
}

static int read_move(void){
    char buf[64];
    if (!fgets(buf, sizeof buf, stdin)) return -1;
    return parse_move(buf);
}

static void usage(const char *prog){
    printf("Usage: %s [--ai X|O|none] [--selftest]\n", prog);
    printf("Enter moves as 0..8 or algebraic a1..c3 (a1=top-left)\n");
}

static int selftest(void){
    // Best vs best from start should draw.
    ttt_reset_cache();
    Board b = ttt_initial();
    for (int ply = 0; ply < 9; ++ply) {
        if (ttt_is_win_bits(ttt_bits_x(b)) || ttt_is_win_bits(ttt_bits_o(b))) break;
        int mv = ttt_best_move(b);
        if (mv < 0) break;
        b = ttt_apply(b, mv);
    }
    if (!(ttt_bits_x(b) | ttt_bits_o(b))) return 1; // sanity
    if (!(ttt_bits_x(b) == 0 && ttt_bits_o(b) == 0)) { /* noop */ }
    ttt_score s;
    if (!ttt_is_terminal(b, &s)) return (fprintf(stderr, "Selftest: not terminal\n"), 1);
    if (s != TTT_DRAW) return (fprintf(stderr, "Selftest: expected draw, got %d\n", s), 1);

    // A forced win test: X: 0,1 ; O: 4, find X=2 to win on row 0
    ttt_reset_cache();
    Board t = ttt_initial();
    t = ttt_apply(t, 0); // X
    t = ttt_apply(t, 4); // O
    t = ttt_apply(t, 1); // X
    int bm = ttt_best_move(t);
    if (bm != 2) return (fprintf(stderr, "Selftest: expected best move 2, got %d\n", bm), 1);

    puts("Selftest passed.");
    return 0;
}

int main(int argc, char **argv){
    int ai_set = 0; ttt_side ai = (ttt_side)2; // 2 == NONE here in CLI
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--ai") == 0) {
            if (i+1 >= argc) return (usage(argv[0]), 1);
            const char *v = argv[++i];
            if (v[0]=='X' || v[0]=='x') { ai = TTT_X; ai_set = 1; }
            else if (v[0]=='O' || v[0]=='o' || v[0]=='0') { ai = TTT_O; ai_set = 1; }
            else if (v[0]=='n' || v[0]=='N') { ai_set = 1; /* human vs human */ }
            else return (usage(argv[0]), 1);
        } else if (strcmp(argv[i], "--selftest") == 0) {
            return selftest();
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            return (usage(argv[0]), 0);
        } else {
            return (usage(argv[0]), 1);
        }
    }

    ttt_reset_cache();
    Board b = ttt_initial();

    while (true) {
        ttt_score s;
        if (ttt_is_terminal(b, &s)) {
            show_board(b);
            if (s == TTT_DRAW) puts("Draw");
            else printf("%c wins\n", token((ttt_side)((ttt_side_to_move(b)^1))));
            break;
        }

        int mv = -1;
        if (ai_set && ttt_side_to_move(b) == ai) {
            mv = ttt_best_move(b);
        } else {
            show_board(b);
            printf("Player %c, your move (0-8 or a1..c3): ", token(ttt_side_to_move(b)));
            mv = read_move();
            if (!ttt_is_legal(b, mv)) { puts("Invalid move."); continue; }
        }
        b = ttt_apply(b, mv);
    }
    return 0;
}

