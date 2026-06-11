# -------- pretty colors -----------------------------------
RESET   := \033[0m
BOLD    := \033[1m
RED     := \033[31m
GREEN   := \033[32m
YELLOW  := \033[33m
BLUE    := \033[34m
MAGENTA := \033[35m
CYAN    := \033[36m

# ==========================================================
.PHONY: format

# -------- utility targets ---------------------------------
format:
	@printf "$(BOLD)$(YELLOW)[ FORMAT  ]$(RESET) $(CYAN)running clang-format$(RESET)\n"
	@find . -type d -name runtime -prune -o -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
