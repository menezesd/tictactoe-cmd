// ttt_engine.c — C23 tic-tac-toe engine implementation (pure logic)
// Implements the API in ttt_engine.h

#include "ttt_engine.h"

#include <assert.h>
#include <limits.h>
#include <stdalign.h>
#include <ctype.h>
#include <stdlib.h>

static_assert(sizeof(uint16_t) * 8 >= 9, "bitfield needs at least 9 bits");

#define FULL9  ((uint16_t)((1u << 9) - 1u))

// ------------------------- Bit utilities -------------------------

static inline int ctz32(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(value);
#else
    int i = 0;
    while ((value & 1u) == 0u) { value >>= 1; ++i; }
    return i;
#endif
}

// ------------------------- Winning masks -------------------------

static const uint16_t WINS[8] = {
    0007u, 0070u, 0700u,   // rows
    0111u, 0222u, 0444u,   // cols
    0421u, 0124u           // diags
};

bool ttt_is_win_bits(uint16_t bits) {
    for (size_t i = 0; i < sizeof WINS / sizeof WINS[0]; ++i)
        if ((bits & WINS[i]) == WINS[i]) return true;
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

static inline Board remap_board(Board board, const uint8_t map[9]) {
    uint16_t x = remap9(ttt_bits_x(board), map);
    uint16_t o = remap9(ttt_bits_o(board), map);
    return (Board)((Board)x | ((Board)o << 9) | ((Board)ttt_side_to_move(board) << 18));
}

static inline Board rotate90(Board board)  { return remap_board(board, R90); }
static inline Board reflect_h(Board board) { return remap_board(board, RH ); }

// Canonical representative under D4 (rotations + reflection).
static inline Board canonical(Board board) {
    Board best = board;
    Board t = board;
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
static int find_immediate(uint16_t me_bits, uint16_t opponent_bits) {
    uint16_t empty_squares = (uint16_t)(~(me_bits | opponent_bits) & FULL9);
    // Win now
    for (size_t i = 0; i < sizeof WINS / sizeof WINS[0]; ++i) {
        uint16_t w = WINS[i];
        uint16_t need = (uint16_t)(w & ~me_bits);
        if (need && (need & (uint16_t)(need - 1u)) == 0u && (need & empty_squares))
            return ctz32(need);
    }
    // Block opponent
    for (size_t i = 0; i < sizeof WINS / sizeof WINS[0]; ++i) {
        uint16_t w = WINS[i];
        uint16_t need = (uint16_t)(w & ~opponent_bits);
        if (need && (need & (uint16_t)(need - 1u)) == 0u && (need & empty_squares))
            return ctz32(need);
    }
    return -1;
}

// ------------------------- Transposition table -------------------------

typedef struct { uint8_t seen; int8_t score; } TTEntry;

// 19-bit key space; canonicalization reduces distinct states substantially.
#define TT_SIZE (1u << 19)
static alignas(64) TTEntry TT[TT_SIZE];

static inline uint32_t key_from(Board board) {
    // Use canonical board, mask into table size (power of two)
    return (uint32_t)canonical(board) & (TT_SIZE - 1u);
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

static ttt_score search(Board board, ttt_score alpha, ttt_score beta, int ply) {
    TTEntry *entry = &TT[key_from(board)];
    if (entry->seen) return entry->score;

    // Terminal check: score is from side-to-move POV
    ttt_score terminal_score;
    if (ttt_is_terminal(board, &terminal_score)) { entry->seen = 1u; entry->score = (int8_t)terminal_score; return terminal_score; }

    // Identify "me" and "opp" bitboards for current mover
    uint16_t me_bits  = (ttt_side_to_move(board) == TTT_X) ? ttt_bits_x(board) : ttt_bits_o(board);
    uint16_t opponent_bits = (ttt_side_to_move(board) == TTT_X) ? ttt_bits_o(board) : ttt_bits_x(board);

    // Immediate tactic shortcut (win or block)
    int immediate_move = find_immediate(me_bits, opponent_bits);
    if (immediate_move >= 0) {
        Board new_board = ttt_apply(board, immediate_move);
        // If this creates a win for the mover now, return quick mate score.
        ttt_score score = ttt_is_win_bits((ttt_side_to_move(board) == TTT_X) ? ttt_bits_x(new_board) : ttt_bits_o(new_board))
                      ? win_in(ply)
                      : -(search(new_board, -beta, -alpha, ply + 1));
        entry->seen = 1u; entry->score = (int8_t)score;
        return score;
    }

    // Generate moves in a good order
    uint16_t empty_squares = (uint16_t)(~ttt_bits_occ(board) & FULL9);
    ttt_score a = alpha;

    for (int k = 0; k < 9; ++k) {
        int square = ORDER[k];
        if ((empty_squares & (1u << square)) == 0u) continue;

        Board new_board = ttt_apply(board, square);

        // Quick win check after the move
        if (ttt_is_win_bits((ttt_side_to_move(board) == TTT_X) ? ttt_bits_x(new_board) : ttt_bits_o(new_board))) {
            ttt_score score = win_in(ply);
            if (score > a) a = score;
            entry->seen = 1u; entry->score = (int8_t)a;
            return a;
        }

        ttt_score score = -(search(new_board, -beta, -a, ply + 1));
        if (score > a) {
            a = score;
            if (a >= beta) { entry->seen = 1u; entry->score = (int8_t)a; return a; } // cutoff
        }
    }

    entry->seen = 1u; entry->score = (int8_t)a;
    return a;
}

// ------------------------- Public API -------------------------

bool ttt_is_terminal(Board board, ttt_score *out_score) {
    uint16_t x = ttt_bits_x(board), o = ttt_bits_o(board);

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

int ttt_best_move(Board board) {
    // Fast-path guard: if terminal or no empties, no move to make
    if ((ttt_bits_occ(board) == FULL9) ||
        ttt_is_win_bits(ttt_bits_x(board)) ||
        ttt_is_win_bits(ttt_bits_o(board))) {
        return -1;
    }

    uint16_t empty_squares = (uint16_t)(~ttt_bits_occ(board) & FULL9);

    // Immediate tactic: win now, otherwise block opponent
    {
        uint16_t me_bits  = (ttt_side_to_move(board) == TTT_X) ? ttt_bits_x(board) : ttt_bits_o(board);
        uint16_t opponent_bits = (ttt_side_to_move(board) == TTT_X) ? ttt_bits_o(board) : ttt_bits_x(board);
        int immediate_move = find_immediate(me_bits, opponent_bits);
        if (immediate_move >= 0) return immediate_move;
    }

    // Explore all legal moves with αβ; pick the max score
    ttt_score best_score = INT_MIN / 2;
    int best_square = -1;

    for (int k = 0; k < 9; ++k) {
        int square = ORDER[k];
        if ((empty_squares & (1u << square)) == 0u) continue;

        Board new_board = ttt_apply(board, square);

        // If this wins immediately, prefer it
        ttt_score score = ttt_is_win_bits((ttt_side_to_move(board) == TTT_X) ? ttt_bits_x(new_board) : ttt_bits_o(new_board))
                     ? win_in(0)
                     : -(search(new_board, INT_MIN / 2, INT_MAX / 2, 1));

        if (score > best_score) { best_score = score; best_square = square; }
    }

    return best_square; // Should be valid due to the fast-path guard
}

// ------------------------- Utilities -------------------------

int ttt_parse_move(const char *str){
    while (*str && isspace((unsigned char)*str)) ++str;
    char *end_ptr = NULL;
    long value = strtol(str, &end_ptr, 10);
    if (end_ptr != str && (*end_ptr == '\0' || isspace((unsigned char)*end_ptr))) { // It was a number
        if (value < 0 || value > 8) return TTT_PARSE_OUT_OF_RANGE;
        return (int)value;
    }

    // Algebraic input
    if (((str[0]|32) >= 'a' && (str[0]|32) <= 'c') && (str[1] >= '1' && str[1] <= '3')) {
        int col = (str[0] | 32) - 'a'; int row = str[1] - '1';
        return row*3 + col;
    }

    return TTT_PARSE_INVALID_FORMAT;
}