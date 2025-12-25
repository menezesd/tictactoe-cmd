#include "ttt_engine.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void show_board(Board board)
{
    uint16_t x_bits = ttt_bits_x(board), o_bits = ttt_bits_o(board);
    printf("    a   b   c\n"); // Column coordinates
    puts("  +---+---+---+");
    for (int r = 0; r < 3; ++r) {
        printf("%d |", r + 1); // Row coordinate
        for (int c = 0; c < 3; ++c) {
            int i = r * 3 + c;
            char ch = (x_bits & (1u << i)) ? 'X' : (o_bits & (1u << i)) ? 'O'
                                                                        : ' ';
            printf(" %c |", ch);
        }
        puts(""); // Newline after each row
        puts("  +---+---+---+"); // Row separator
    }
}

static int read_move(void)
{
    char buf[64];
    if (!fgets(buf, sizeof buf, stdin))
        return TTT_PARSE_EOF;
    return ttt_parse_move(buf);
}

static void usage(const char* program_name)
{
    fprintf(stderr, "Usage: %s [--ai X|O|none]\n", program_name);
    fprintf(stderr, "Enter moves as 0..8 or algebraic a1..c3 (a1=top-left)\n");
}

static int get_human_move(Board board)
{
    while (true) {
        printf("\nPlayer %c, your move (0-8 or a1..c3): ", token(ttt_side_to_move(board)));
        fflush(stdout);
        int move = read_move();
        if (move == TTT_PARSE_EOF) { // EOF or read error
            return -1; // Internal EOF signal
        }
        if (move == TTT_PARSE_INVALID_FORMAT) {
            fprintf(stderr, "Invalid format. Enter 0-8 or a1-c3.\n");
            continue;
        }
        if (move == TTT_PARSE_OUT_OF_RANGE) {
            fprintf(stderr, "Move out of range. Enter 0-8 or a1-c3.\n");
            continue;
        }
        if (!ttt_is_legal(board, move)) {
            fprintf(stderr, "Illegal move (square occupied or invalid).\n");
            continue;
        }
        return move;
    }
}

static int parse_cli_arguments(int argc, const char* const* argv, ttt_side* ai_player)
{
    *ai_player = (ttt_side)2; // Default to NONE

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--ai") == 0) {
            if (i + 1 >= argc)
                return (usage(argv[0]), 1);
            const char* value = argv[++i];
            if (value[0] == 'X' || value[0] == 'x') {
                *ai_player = TTT_X;
            } else if (value[0] == 'O' || value[0] == 'o' || value[0] == '0') {
                *ai_player = TTT_O;
            } else if (value[0] == 'n' || value[0] == 'N') { /* human vs human, ai_player remains NONE */
            } else
                return (usage(argv[0]), 1);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            return (usage(argv[0]), 0);
        } else {
            return (usage(argv[0]), 1);
        }
    }
    return 0; // Success
}

static int run_game(ttt_side ai_player)
{
    ttt_reset_cache();
    Board board = ttt_initial();
    bool ai_active = (ai_player != (ttt_side)2);

    printf("Welcome to Tic-Tac-Toe!\n\n");

    while (true) {
        show_board(board);

        ttt_score score;
        if (ttt_is_terminal(board, &score)) {
            printf("\n--- GAME OVER ---\n");
            if (score == TTT_DRAW) {
                printf("It's a draw!\n");
            } else {
                printf("Player %c wins!\n", token((ttt_side)((ttt_side_to_move(board) ^ 1))));
            }
            printf("-----------------\n");
            break;
        }

        int move = -1;
        if (ai_active && ttt_side_to_move(board) == ai_player) {
            printf("\nAI is playing...\n");
            move = ttt_best_move(board);
            printf("AI played on square %d\n", move);
        } else {
            move = get_human_move(board);
            if (move == -1) { // EOF or read error
                printf("\nExiting game.\n");
                break;
            }
        }

        board = ttt_apply(board, move);
    }
    return 0;
}

int main(int argc, const char* const* argv)
{
    ttt_side ai = (ttt_side)2; // 2 == NONE here in CLI

    if (parse_cli_arguments(argc, argv, &ai) != 0) {
        return 1; // Error during argument parsing or help requested
    }

    return run_game(ai);
}