SRC = src/sync/lock.cc src/sync/guard.cc src/sync/epoch.cc src/list/list.cc

LDFLAGS = -lpthread

format:
	find . -regex '.*\.\(cpp\|hpp\|cc\|cxx\|cc\|h\)' -exec clang-format -style=file -i {} \;
	@echo "Done"

test:
	g++ -g test/test.cc $(SRC) -std=c++20 -Iinclude -march=native -o output $(LDFLAGS)
	./output --enable-mixed-units

.PHONY: style test
