#include "leducgame.hpp"
#include "cfr.hpp"
#include "exploit.hpp"

int main()
{
    LeducGame game;
    CFR<LeducGame> cfr{game};

    cfr.train(1'000'000 / 1024);

    auto avg = cfr.get_average_strategy();

    double v_self = evaluate_policy<LeducGame>(game, avg);
    double nc = nash_conv<LeducGame>(game, avg);
    double expl = exploitability<LeducGame>(game, avg);

    std::cout << "Self-play value (P1 vs P1): " << v_self << "\n";
    std::cout << "NashConv: " << nc << " chips\n";
    std::cout << "Exploitability: " << expl << " chips\n";

    return 0;
}