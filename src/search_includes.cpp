#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <string>

using namespace std;

set<string> get_includes(string filename) {
  set<string> res;
  ifstream in(filename);
  string test;

  while (in >> test) {
    if (test == "#include") {
      string sample;
      in >> sample;
      int n = sample.length();
      if (n > 0 && sample[0] == '"') {
        res.insert(sample.substr(1, n - 2));
      }
    }
  }

  return res;
}

void search_at(string node, list<string> path) {
  for (auto x : path) {
    if (node == x) {
      path.push_back(node);
      cout << "Loop found: " << endl;
      for (auto y : path) cout << y << " -> ";
      cout << endl;
      exit(0);
    }
  }

  cout << "search at: ";
  for (auto x : path) cout << x << " -> ";
  cout << ": " << node << endl;

  path.push_back(node);
  for (auto x : get_includes(node)) search_at(x, path);
}

int main(int argc, char **argv) {
  list<string> path;
  search_at(argv[1], path);
}
