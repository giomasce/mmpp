
#include "uct.h"

#include "utils/utils.h"

#include <memory>

using namespace std;

class Test : public enable_create< Test > {
public:
protected:
    Test() : x(0) {}
    Test(int x) : x(x) {}

private:
    int x;
};

void test22() {
    auto t1 = Test::create();
    auto t2 = Test::create(22);
}
