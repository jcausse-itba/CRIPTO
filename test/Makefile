CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c23 -g -fsanitize=address,undefined -I../include -I.
SRCDIR := ../src
BUILDDIR := build

# Automatically discover all test_*.c and *_test.c files
TEST_SRCS := $(wildcard test_*.c *_test.c)
TEST_BINS := $(patsubst %.c,$(BUILDDIR)/%,$(TEST_SRCS))

# All project sources (excluding main.c)
PROJ_SRCS := $(filter-out $(SRCDIR)/main.c,$(wildcard $(SRCDIR)/*.c))

.PHONY: all clean

all: $(TEST_BINS)
	@printf "\n\n"
	@for bin in $(TEST_BINS); do \
		echo "========== Running $$bin =========="; \
		./$$bin; \
		status=$$?; \
		if [ $$status -ne 0 ]; then \
			echo "FAILED (exit code $$status)"; \
			printf "\n\n"; \
			exit $$status; \
		fi; \
		echo ""; \
	done
	@echo "All tests passed."
	@printf "\n\n"

$(BUILDDIR)/%: %.c $(PROJ_SRCS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $< $(PROJ_SRCS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)
