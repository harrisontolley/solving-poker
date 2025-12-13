#include "leducgame.hpp"
#include "cfr.hpp"

int main()
{
    LeducGame game;
    CFR<LeducGame> cfr{game};

    cfr.train(1'000'000);

    return 0;
}