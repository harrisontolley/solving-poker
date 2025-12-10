#include "kuhngame.hpp"
#include "cfr.hpp"

int main()
{
    KuhnGame game;
    CFR<KuhnGame> cfr{game};

    cfr.train(10000);

    return 0;
}