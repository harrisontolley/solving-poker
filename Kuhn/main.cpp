#include "kuhngame.hpp"
#include "cfr.hpp"
#include "exploit.hpp"

int main()
{
    KuhnGame game;
    CFR<KuhnGame> cfr{game};

    cfr.train(100000);

    auto avg = cfr.get_average_strategy();

    double v_self = evaluate_policy<KuhnGame>(game, avg);
    double nc = nash_conv<KuhnGame>(game, avg);
    double expl = exploitability<KuhnGame>(game, avg);

    std::cout << "Self-play value (P1 vs P1): " << v_self << "\n";
    std::cout << "NashConv: " << nc << " chips\n";
    std::cout << "Exploitability: " << expl << " chips\n";

    return 0;
}