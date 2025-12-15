#include "kuhngame.hpp"
#include "cfr.hpp"

int main()
{
    KuhnGame game;
    CFRVanilla<KuhnGame> cfr{game};

    cfr.train(10000);

    return 0;
}