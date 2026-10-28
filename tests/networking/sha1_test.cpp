#include "sha1.h"
#include <cassert>
int main() {
    assert(Sha1Hex("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    assert(Sha1Hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    assert(Sha1Hex(std::string(1000000, 'a')) == "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}
