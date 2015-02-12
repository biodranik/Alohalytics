.PHONY: indent

indent:
	(find src src/cpp src/posix src/android/jni -maxdepth 1 -name "*.cc" -o -name "*.h") | xargs clang-format -i
