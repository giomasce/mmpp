
#include <iostream>
#include <fstream>

#include "wff.h"
#include "parser.h"

using namespace std;

int main() {

  /*auto toks = tokenize("( ( ph -/\\ ps ) \\/_ ph )");
  for (auto tok : toks) {
    //cout << tok << endl;
  }

  auto ph = pwff(new Var("ph"));
  auto ps = pwff(new Var("ps"));
  auto w = pwff(new Nand(ph, ps));
  auto w2 = pwff(new Xor(w, ph));

  cout << w2->to_string() << endl;
  cout << w2->imp_not_form()->to_string() << endl;*/

    /*fstream in("/home/giovanni/progetti/metamath/demo0.mm");
    FileTokenizer ft(in);
    Parser p(ft);
    p.run();*/

    fstream in("/home/giovanni/progetti/metamath/set.mm/set.mm");
    FileTokenizer ft(in);
    Parser p(ft);
    p.run();

    return 0;

}
