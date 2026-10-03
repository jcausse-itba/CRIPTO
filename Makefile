BUILD_DIR ?= build

.PHONY: all clean test

all:
	@cmake -B $(BUILD_DIR) -S .
	@cmake --build $(BUILD_DIR) --target stegobmp
	@cp -f $(BUILD_DIR)/stegobmp .

test:
	@cmake -B $(BUILD_DIR) -S .
	@cmake --build $(BUILD_DIR)
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	@rm -rf $(BUILD_DIR) stegobmp *.o
	@$(MAKE) -C test clean > /dev/null 2>&1 || true
