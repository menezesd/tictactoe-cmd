// ttt_engine.c — C23 tic-tac-toe engine implementation (pure logic)
// Implements the API in ttt_engine.h

#include "ttt_engine.h"

#include <assert.h>
#include <limits.h>
#include <stdalign.h>

static_assert(sizeof(uint16_t) * 8 >= 9, "bitfield needs at least 9 bits");

#define FULL9  ((uint16_t)((1u << 9) - 1u))

// ------------------------- Bit utilities -------------------------

static inline int popcount16(uint16_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount((unsigned)x);
#else
    int c = 0;
    while (x) { x &= (uint16_t)(x - 1u); ++c; }
    return c;
#endif
}

static inline int ctz32(uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(x);
#else
    int i = 0;
    while ((x & 1u) == 0u) { x >>= 1; ++i; }
    return i;
#endif
}

// ------------------------- Winning masks -------------------------

static const uint16_t WINS[8] = {
    0007u, 0070u, 0700u,   // rows
    0111u, 0222u, 0444u,   // cols
    0421u, 0124u           // diags
};

bool ttt_is_win_bits(uint16_t b) {
    for (size_t i = 0; i < sizeof WINS / sizeof WINS[0]; ++i)
        if ((b & WINS[i]) == WINS[i]) return true;
    return false;
}

// ------------------------- Symmetry transforms (3x3) -------------------------
/*
   Index layout:
     0 1 2
     3 4 5
     6 7 8

   rotate90 map (i -> R90[i]) and horizontal reflection RH.
*/
static const uint8_t R90[9] = { 2,5,8, 1,4,7, 0,3,6 };
static const uint8_t RH [9] = { 2,1,0, 5,4,3, 8,7,6 };

static inline uint16_t remap9(uint16_t bits, const uint8_t map[9]) {
    uint16_t out = 0;
    for (int i = 0; i < 9; ++i)
        if (bits & (1u << i)) out |= (uint16_t)(1u << map[i]);
    return out;
}

static inline Board remap_board(Board b, const uint8_t map[9]) {
    uint16_t x = remap9(ttt_bits_x(b), map);
    uint16_t o = remap9(ttt_bits_o(b), map);
    return (Board)((Board)x | ((Board)o << 9) | ((Board)ttt_side_to_move(b) << 18));
}

static inline Board rotate90(Board b)  { return remap_board(b, R90); }
static inline Board reflect_h(Board b) { return remap_board(b, RH ); }

// Canonical representative under D4 (rotations + reflection).
static inline Board canonical(Board b) {
    Board best = b;
    Board t = b;
    for (int r = 0; r < 4; ++r) {
        if (t < best) best = t;
        Board tr = reflect_h(t);
        if (tr < best) best = tr;
        t = rotate90(t);
    }
    return best;
}

// ------------------------- Quick tactics (win/block) -------------------------

// Return a square index for immediate win, otherwise immediate block, else -1.
static int find_immediate(uint16_t me, uint16_t opp) {
    uint16_t empt = (uint16_t)(~(me | opp) & FULL9);
    // Win now
    for (size_t i = 0; i < sizeof WINS / sizeof WINS[0]; ++i) {
        uint16_t w = WINS[i];
        uint16_t need = (uint16_t)(w & ~me);
        if (need && (need & (uint16_t)(need - 1u)) == 0u && (need & empt))
            return ctz32(need);
    }
    // Block opponent
    for (size_t i = 0; i < sizeof WINS / sizeof WINS[0]; ++i) {
        uint16_t w = WINS[i];
        uint16_t need = (uint16_t)(w & ~opp);
        if (need && (need & (uint16_t)(need - 1u)) == 0u && (need & empt))
            return ctz32(need);
    }
    return -1;
}

// ------------------------- Transposition table -------------------------

typedef struct { uint8_t seen; int8_t score; } TTEntry;

// 19-bit key space; canonicalization reduces distinct states substantially.
#define TT_SIZE (1u << 19)
static alignas(64) TTEntry TT[TT_SIZE];

static inline uint32_t key_from(Board b) {
    // Use canonical board, mask into table size (power of two)
    return (uint32_t)canonical(b) & (TT_SIZE - 1u);
}

void ttt_reset_cache(void) {
    for (size_t i = 0; i < TT_SIZE; ++i) TT[i].seen = 0;
}

// ------------------------- Move ordering -------------------------

// Center, corners, edges
static const int ORDER[9] = { 4, 0, 2, 6, 8, 1, 3, 5, 7 };

// ------------------------- Search (negamax αβ) -------------------------

static inline ttt_score win_in(int ply)  { return  TTT_WIN  - ply; }
static inline ttt_score lose_in(int ply) { return  TTT_LOSS + ply; }

static ttt_score search(Board b, ttt_score alpha, ttt_score beta, int ply) {
    TTEntry *e = &TT[key_from(b)];
    if (e->seen) return e->score;

    // Terminal check: score is from side-to-move POV
    ttt_score ts;
    if (ttt_is_terminal(b, &ts)) { e->seen = 1u; e->score = (int8_t)ts; return ts; }

    // Identify "me" and "opp" bitboards for current mover
    uint16_t me  = (ttt_side_to_move(b) == TTT_X) ? ttt_bits_x(b) : ttt_bits_o(b);
    uint16_t opp = (ttt_side_to_move(b) == TTT_X) ? ttt_bits_o(b) : ttt_bits_x(b);

    // Immediate tactic shortcut (win or block)
    int imm = find_immediate(me, opp);
    if (imm >= 0) {
        Board nb = ttt_apply(b, imm);
        // If this creates a win for the mover now, return quick mate score.
        ttt_score s = ttt_is_win_bits((ttt_side_to_move(b) == TTT_X) ? ttt_bits_x(nb) : ttt_bits_o(nb))
                      ? win_in(ply)
                      : -(search(nb, -beta, -alpha, ply + 1));
        e->seen = 1u; e->score = (int8_t)s;
        return s;
    }

    // Generate moves in a good order
    uint16_t empt = (uint16_t)(~ttt_bits_occ(b) & FULL9);
    ttt_score a = alpha;

    for (int k = 0; k < 9; ++k) {
        int sq = ORDER[k];
        if ((empt & (1u << sq)) == 0u) continue;

        Board nb = ttt_apply(b, sq);

        // Quick win check after the move
        if (ttt_is_win_bits((ttt_side_to_move(b) == TTT_X) ? ttt_bits_x(nb) : ttt_bits_o(nb))) {
            ttt_score s = win_in(ply);
            if (s > a) a = s;
            e->seen = 1u; e->score = (int8_t)a;
            return a;
        }

        ttt_score sc = -(search(nb, -beta, -a, ply + 1));
        if (sc > a) {
            a = sc;
            if (a >= beta) { e->seen = 1u; e->score = (int8_t)a; return a; } // cutoff
        }
    }

    e->seen = 1u; e->score = (int8_t)a;
    return a;
}

// ------------------------- Public API -------------------------

bool ttt_is_terminal(Board b, ttt_score *out_score) {
    uint16_t x = ttt_bits_x(b), o = ttt_bits_o(b);

    // If opponent (who just moved) has a 3-in-a-row, side-to-move is losing.
    if (ttt_is_win_bits(o)) {
        if (out_score) *out_score = TTT_LOSS;
        return true;
    }
    // Full board → draw
    if ((x | o) == FULL9) {
        if (out_score) *out_score = TTT_DRAW;
        return true;
    }
    return false;
}

int ttt_best_move(Board b) {
    // Fast-path guard: if terminal or no empties, no move to make
    if ((ttt_bits_occ(b) == FULL9) ||
        ttt_is_win_bits(ttt_bits_x(b)) ||
        ttt_is_win_bits(ttt_bits_o(b))) {
        return -1;
    }

    uint16_t empt = (uint16_t)(~ttt_bits_occ(b) & FULL9);

    // Immediate tactic: win now, otherwise block opponent
    {
        uint16_t me  = (ttt_side_to_move(b) == TTT_X) ? ttt_bits_x(b) : ttt_bits_o(b);
        uint16_t opp = (ttt_side_to_move(b) == TTT_X) ? ttt_bits_o(b) : ttt_bits_x(b);
        int imm = find_immediate(me, opp);
        if (imm >= 0) return imm;
    }

    // Explore all legal moves with αβ; pick the max score
    ttt_score best = INT_MIN / 2;
    int best_sq = -1;

    for (int k = 0; k < 9; ++k) {
        int sq = ORDER[k];
        if ((empt & (1u << sq)) == 0u) continue;

        Board nb = ttt_apply(b, sq);

        // If this wins immediately, prefer it
        ttt_score sc = ttt_is_win_bits((ttt_side_to_move(b) == TTT_X) ? ttt_bits_x(nb) : ttt_bits_o(nb))
                     ? win_in(0)
                     : -(search(nb, INT_MIN / 2, INT_MAX / 2, 1));

        if (sc > best) { best = sc; best_sq = sq; }
    }

    return best_sq; // Should be valid due to the fast-path guard
}

