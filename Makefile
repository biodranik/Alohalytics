.PHONY: indent

indent:
	(find src examples server tests -maxdepth 1 -name "*.cc" -o -name "*.h") | xargs clang-format -i
