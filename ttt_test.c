#include "ttt_engine.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// A simple assertion macro
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #condition, __FILE__, __LINE__); \
            return false; \
        } \
    } while (0)

// Test function signature
typedef bool (*test_func)(void);

static bool test_draw(void) {
    printf("Running test: %s\n", __func__);
    ttt_reset_cache();
    Board b = ttt_initial();
    for (int ply = 0; ply < 9; ++ply) {
        if (ttt_is_win_bits(ttt_bits_x(b)) || ttt_is_win_bits(ttt_bits_o(b))) break;
        int mv = ttt_best_move(b);
        if (mv < 0) break;
        b = ttt_apply(b, mv);
    }
    ASSERT(!(ttt_bits_x(b) | ttt_bits_o(b)) == 0);

    ttt_score s;
    ASSERT(ttt_is_terminal(b, &s));
    ASSERT(s == TTT_DRAW);
    return true;
}

static bool test_forced_win(void) {
    printf("Running test: %s\n", __func__);
    ttt_reset_cache();
    Board t = ttt_initial();
    t = ttt_apply(t, 0); // X
    t = ttt_apply(t, 4); // O
    t = ttt_apply(t, 1); // X
    int bm = ttt_best_move(t);
    ASSERT(bm == 2);
    return true;
}

static bool test_win_conditions(void) {
    printf("Running test: %s\n", __func__);
    const uint16_t WINS[8] = {
        0007u, 0070u, 0700u, // rows
        0111u, 0222u, 0444u, // cols
        0421u, 0124u        // diags
    };

    for (size_t i = 0; i < 8; ++i) {
        ASSERT(ttt_is_win_bits(WINS[i]));
    }
    return true;
}

static bool test_move_parser(void) {
    printf("Running test: %s\n", __func__);
    ASSERT(ttt_parse_move("0") == 0);
    ASSERT(ttt_parse_move(" 8 ") == 8);
    ASSERT(ttt_parse_move("a1") == 0);
    ASSERT(ttt_parse_move("c3") == 8);
    ASSERT(ttt_parse_move("b2") == 4);
    ASSERT(ttt_parse_move(" a2 ") == 3);
    ASSERT(ttt_parse_move("9") == TTT_PARSE_OUT_OF_RANGE);
    ASSERT(ttt_parse_move("-1") == TTT_PARSE_OUT_OF_RANGE);
    ASSERT(ttt_parse_move("d1") == TTT_PARSE_INVALID_FORMAT);
    ASSERT(ttt_parse_move("a4") == TTT_PARSE_INVALID_FORMAT);
    ASSERT(ttt_parse_move("hello") == TTT_PARSE_INVALID_FORMAT);
    ASSERT(ttt_parse_move("") == TTT_PARSE_INVALID_FORMAT);
    return true;
}

// Array of tests to run
static test_func tests[] = {
    test_draw,
    test_forced_win,
    test_win_conditions,
    test_move_parser,
};

int main(void) {
    int passed = 0;
    int failed = 0;

    size_t num_tests = sizeof(tests) / sizeof(tests[0]);
    printf("Running %zu tests...\n", num_tests);

    for (size_t i = 0; i < num_tests; ++i) {
        if (tests[i]()) {
            passed++;
        } else {
            failed++;
            printf("Test failed.\n");
        }
    }

    printf("\nTests summary:\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);

    return failed > 0 ? 1 : 0;
}