# Makefile for ttt (tic-tac-toe) — C23
# Files: ttt_engine.c / ttt_cli.c -> binaries: ttt, ttt_debug, ttt_san

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
BIN      := ttt
BIN_DBG  := ttt_debug
BIN_SAN  := ttt_san
OBJS     := ttt_engine.o ttt_cli.o
DEPS     := $(OBJS:.o=.d)

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

# Self-test: builds release binary then runs its --selftest
test: $(BIN)
	./$(BIN) --selftest

# Pattern rule with auto-deps
%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include dependency files if present
-include $(DEPS)

clean:
	$(RM) $(OBJS) $(DEPS)

clobber: clean
	$(RM) $(BIN) $(BIN_DBG) $(BIN_SAN)

