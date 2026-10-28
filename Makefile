.DEFAULT_GOAL := help
BUILD_DIR ?= build

CMAKE ?= cmake
CTEST ?= ctest
PYTHON := $(shell command -v python3 || command -v python)


.PHONY: dev release test clean check-python inspect help


dev: ## Build a development executable that bundles Lib ASan
	@$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
	@$(CMAKE) --build $(BUILD_DIR) --target stegobmp
	@cp -f $(BUILD_DIR)/stegobmp .

release: ## Build a release executable that does not bundle Lib ASan in order to reduce memory footprint
	@$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF
	@$(CMAKE) --build $(BUILD_DIR) --target stegobmp
	@cp -f $(BUILD_DIR)/stegobmp .

test: ## Run all tests
	@$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
	@$(CMAKE) --build $(BUILD_DIR)
	@$(CTEST) --test-dir $(BUILD_DIR) --verbose

clean: ## Clean temporary build files
	@rm -rf $(BUILD_DIR) stegobmp *.o
	@rm -rf ./cmake-build-debug
	@$(MAKE) -C test clean > /dev/null 2>&1 || true

check-python:
	@if [ -z "$(PYTHON)" ]; then \
		echo "Error: Neither 'python3' nor 'python' was found in your PATH."; \
		exit 1; \
	fi

inspect: check-python ## Inspect a BMP image to determine whether it's OK to use with stegobmp (BMP v3) or not
	@$(PYTHON) external-utilities/bmp_inspector.py

help: ## Show this help message
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_-]+:.*?## / {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
