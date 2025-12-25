# Makefile for ttt (tic-tac-toe) — C23
# Files: ttt_engine.c / ttt_cli.c / ttt_test.c -> binaries: ttt, ttt_test

# ---- Toolchain & flags ----
CC      ?= cc
STD     := -std=c23
WARN    := -Wall -Wextra -Wconversion -Wsign-conversion -pedantic
OPT     ?= -O3
DBG     ?= -g
CFLAGS  ?= $(STD) $(WARN) $(OPT)
LDFLAGS ?=
SANFLAGS:= -fsanitize=address,undefined -fno-omit-frame-pointer

# ---- Targets ----
BIN       := ttt
BIN_DBG   := ttt_debug
BIN_SAN   := ttt_san
BIN_TEST  := ttt_test

OBJS      := ttt_engine.o ttt_cli.o
OBJS_TEST := ttt_engine.o ttt_test.o
ALL_OBJS  := $(sort $(OBJS) $(OBJS_TEST))
DEPS      := $(ALL_OBJS:.o=.d)

.PHONY: all debug san test clean clobber

all: $(BIN)

# Release build
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Debug build (no optimizations, symbols)
debug: CFLAGS := $(STD) $(WARN) -O0 $(DBG)
debug: $(BIN_DBG)

$(BIN_DBG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Sanitized build (ASan/UBSan)
san: CFLAGS := $(STD) $(WARN) -O0 $(DBG) $(SANFLAGS)
san: LDFLAGS += $(SANFLAGS)
san: $(BIN_SAN)

$(BIN_SAN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Self-test: builds and runs the test binary
test: $(BIN_TEST)
	./$(BIN_TEST)

$(BIN_TEST): $(OBJS_TEST)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Pattern rule with auto-deps
%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include dependency files if present
-include $(DEPS)

clean:
	$(RM) $(ALL_OBJS) $(DEPS)

clobber: clean
	$(RM) $(BIN) $(BIN_DBG) $(BIN_SAN) $(BIN_TEST)