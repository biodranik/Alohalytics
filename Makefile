.PHONY: indent

indent:
	(find src src/cpp src/posix src/windows src/android/jni examples/server examples/cpp tests -maxdepth 1 -name "*.cc" -o -name "*.h") | xargs clang-format -i
