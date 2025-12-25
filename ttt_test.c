#include "ttt_engine.h"
#include <stdio.h>
#include <stdbool.h>

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

int main(void) {
    return selftest();
}
