# Copyright (c) 2025 ETH Zurich and University of Bologna
# SPDX-License-Identifier: Apache-2.0

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  format   - Format code using clang-format and yapf"
	@echo "  help     - Display this help message"

format:
	@echo "Formatting code..."
	@python3 ./scripts/run_clang_format.py -ri .
	@python3 -m yapf -rpi .

.PHONY: help format