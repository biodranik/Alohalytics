#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

int main(int argc, char ** argv) {
  ifstream file(argv[1]);
  int sum = 0;
  int i = 1;
  while (file.good()) {
    string s;
    getline(file, s);
    istringstream stream(s);
    int num;
    stream >> num;
    sum += num;
    cout << i++ << " " << sum << endl;
  }
  return 0;
}
