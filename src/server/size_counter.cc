#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <exception>

using namespace std;

int main(int argc, char ** argv) {
  

  uint64_t kbcount = 0, kbsize = 0, count = 0, size = 0;
  ifstream file(argv[1]);
  while (file.good()) {
    string line;
    getline(file, line);
    try {
      int num = stoi(line);
      if (num < 1024) {
        ++kbcount; kbsize += num;
      } else {
        ++count; size += num;
      }
    } catch (std::exception & ex) {
      cerr << "Exception: " << ex.what() << endl;
    }
  }

  cout << kbcount << " files < 1kb with total size " << kbsize << endl;
  cout << count << "files > 1kb with total size " << size << endl;

  return 0;
}
