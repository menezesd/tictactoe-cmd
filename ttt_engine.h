// ttt_engine.h — C23 tic-tac-toe engine (pure logic, no I/O)
#ifndef TTT_ENGINE_H
#define TTT_ENGINE_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Packed board: bits 0..8 = X, 9..17 = O, bit 18 = side (0 = X to move, 1 = O)
typedef uint32_t Board;

/// Side to move.
typedef enum : int { TTT_X = 0, TTT_O = 1 } ttt_side;

/// Human-readable squares (top-left is A1).
typedef enum {
    A1 = 0, B1, C1,
    A2,     B2, C2,
    A3, B3, C3
} ttt_sq;

/// Integer score type.
typedef int ttt_score;
enum { TTT_WIN = 100, TTT_LOSS = -100, TTT_DRAW = 0 };

/// @name Board helpers (pure)
/// @{

/// Return the side to move in @p b.
static inline ttt_side  ttt_side_to_move(Board b)        { return (ttt_side)((b >> 18) & 1u); }

/// Bitboard of X’s occupied squares (9 LSBits).
static inline uint16_t  ttt_bits_x(Board b)              { return (uint16_t)( b       & 0x1FFu); }

/// Bitboard of O’s occupied squares (next 9 bits).
static inline uint16_t  ttt_bits_o(Board b)              { return (uint16_t)((b >> 9) & 0x1FFu); }

/// Bitboard of all occupied squares.
static inline uint16_t  ttt_bits_occ(Board b)            { return (uint16_t)(ttt_bits_x(b) | ttt_bits_o(b)); }

/// Flip side to move and return a new board.
static inline Board     ttt_flip_side(Board b)           { return (Board)(b ^ (1u << 18)); }

/// True if @p sq is on board and empty in @p b.
static inline bool      ttt_is_empty(Board b, int sq)    { return (0 <= sq && sq < 9) && ((ttt_bits_occ(b) & (1u << sq)) == 0); }

/// True if @p sq is a legal move in @p b.
static inline bool      ttt_is_legal(Board b, int sq)    { return (0 <= sq && sq < 9) && ttt_is_empty(b, sq); }

/// Apply legal move @p sq and return the new board (asserts legality in debug).
static inline Board     ttt_apply(Board b, int sq) {
    // Contract: caller must pass a legal square.
    assert(ttt_is_legal(b, sq));
    unsigned bit_to_set = (ttt_side_to_move(b) == TTT_X) ? (unsigned)sq : 9u + (unsigned)sq;
    return ttt_flip_side(b | (1u << bit_to_set));
}

/// Construct the initial position: empty board, X to move.
static inline Board     ttt_initial(void)                { return (Board)0u; }

/// @}

/// @name Game logic (search/eval)
/// @{

/**
 * @brief Return true if @p b is terminal; if so, write @p out_score from the
 *        perspective of the side to move: TTT_WIN, TTT_LOSS, or TTT_DRAW.
 * @param b         Board position.
 * @param out_score Optional; receives terminal score if terminal.
 * @return true if terminal, false otherwise.
 */
[[nodiscard]] bool ttt_is_terminal(Board b, ttt_score *out_score);

/**
 * @brief Compute the best move for the side to move in @p b.
 * @param b   Board position.
 * @return    Square index 0..8 (use @ref ttt_sq for named constants), or -1 if no legal moves.
 */
[[nodiscard]] int  ttt_best_move(Board b);

/// Return true if the provided 9-bit bitboard has three in a row (utility/testing).
bool ttt_is_win_bits(uint16_t bits);

/// Clear engine caches (transposition table).
void ttt_reset_cache(void);

/// @}

/// @name Utilities
/// @{

/// Return values for ttt_parse_move
enum {
    TTT_PARSE_INVALID_FORMAT = -1,
    TTT_PARSE_OUT_OF_RANGE = -2,
    TTT_PARSE_EOF = -3,
};

/**
 * @brief Parse a move from a string.
 * @param str The string to parse.
 * @return The square index (0-8), or a TTT_PARSE_* error code.
 */
int ttt_parse_move(const char *str);

/// @}

#ifdef __cplusplus
}
#endif
#endif // TTT_ENGINE_H

