// ttt_engine.h — C23 tic-tac-toe engine (pure logic, no I/O)
#ifndef TTT_ENGINE_H
#define TTT_ENGINE_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Packed board: bits 0..8 = X, 9..17 = O, bit 18 = side (0 = X to move, 1 = O)
typedef uint32_t Board;

/// Side to move.
typedef enum : int { TTT_X = 0,
    TTT_O = 1 } ttt_side;

/// Human-readable squares (top-left is A1).
typedef enum {
    A1 = 0,
    B1,
    C1,
    A2,
    B2,
    C2,
    A3,
    B3,
    C3
} ttt_sq;

/// Integer score type.
typedef int ttt_score;
enum { TTT_WIN = 100,
    TTT_LOSS = -100,
    TTT_DRAW = 0 };

/// @name Board helpers (pure)
/// @{

/// Return the character representation of a ttt_side.
inline char token(ttt_side side) { return side == TTT_X ? 'X' : 'O'; }

/// Return the side to move in @p board.
static inline ttt_side ttt_side_to_move(Board board) { return (ttt_side)((board >> 18) & 1u); }

/// Bitboard of X’s occupied squares (9 LSBits).
static inline uint16_t ttt_bits_x(Board board) { return (uint16_t)(board & 0x1FFu); }

/// Bitboard of O’s occupied squares (next 9 bits).
static inline uint16_t ttt_bits_o(Board board) { return (uint16_t)((board >> 9) & 0x1FFu); }

/// Bitboard of all occupied squares.
static inline uint16_t ttt_bits_occ(Board board) { return (uint16_t)(ttt_bits_x(board) | ttt_bits_o(board)); }

/// Flip side to move and return a new board.
static inline Board ttt_flip_side(Board board) { return (Board)(board ^ (1u << 18)); }

/// True if @p square is on board and empty in @p board.
static inline bool ttt_is_empty(Board board, int square) { return (0 <= square && square < 9) && ((ttt_bits_occ(board) & (1u << square)) == 0); }

/// True if @p square is a legal move in @p board.
static inline bool ttt_is_legal(Board board, int square) { return (0 <= square && square < 9) && ttt_is_empty(board, square); }

/// Apply legal move @p square and return the new board (asserts legality in debug).
static inline Board ttt_apply(Board board, int square)
{
    // Contract: caller must pass a legal square.
    assert(ttt_is_legal(board, square));
    unsigned bit_to_set = (ttt_side_to_move(board) == TTT_X) ? (unsigned)square : 9u + (unsigned)square;
    return ttt_flip_side(board | (1u << bit_to_set));
}

/// Construct the initial position: empty board, X to move.
static inline Board ttt_initial(void) { return (Board)0u; }

/// @}

/// @name Game logic (search/eval)
/// @{

/**
 * @brief Return true if @p board is terminal; if so, write @p out_score from the
 *        perspective of the side to move: TTT_WIN, TTT_LOSS, or TTT_DRAW.
 * @param board         Board position.
 * @param out_score Optional; receives terminal score if terminal.
 * @return true if terminal, false otherwise.
 */
[[nodiscard]] bool ttt_is_terminal(Board board, ttt_score* out_score);

/**
 * @brief Compute the best move for the side to move in @p board.
 * @param board   Board position.
 * @return    Square index 0..8 (use @ref ttt_sq for named constants), or -1 if no legal moves.
 */
[[nodiscard]] int ttt_best_move(Board board);

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
int ttt_parse_move(const char* str);

/// @}

#ifdef __cplusplus
}
#endif
#endif // TTT_ENGINE_H
