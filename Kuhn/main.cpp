#include "kuhngame.hpp"
#include "cfr.hpp"

int main()
{
    KuhnGame game;
    CFR<KuhnGame> cfr{game};

    cfr.train(100000);

    return 0;
}