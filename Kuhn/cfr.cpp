#include "cfr.hpp"
#include "types.hpp"
#include <iostream>

void CFR::train(int num_iterations)
{
    return;
}

void CFR::print_metrics(int num_iterations) const
{
    float total_pos = 0;
    float max_pos = 0;

    for (auto const &[info_set, strategy] : strategy_sum)
    {
        for (float val : strategy)
        {
            total_pos += std::max(0.0f, val);
            max_pos = std::max(max_pos, val);
        }
    }

    std::cout << "Avg pos regret / iter = " << (total_pos / num_iterations) << "\n";
    std::cout << "Max pos regret / iter = " << (max_pos / num_iterations) << "\n";
}

Strategy CFR::regret_match(InfoSet const &info_set)
{
    Strategy regrets = regret_sum[info_set];
    Strategy positive_regrets(regrets.size(), 0.0f);
    float total_regret = 0.0f;

    for (size_t i = 0; i < regrets.size(); ++i)
    {
        positive_regrets[i] = std::max(0.0f, regrets[i]);
        total_regret += positive_regrets[i];
    }

    if (total_regret > 0.0f)
    {
        Strategy strategy(regrets.size(), 0.0f);

        for (size_t i = 0; i < regrets.size(); ++i)
            strategy[i] = positive_regrets[i] / total_regret;

        return strategy;
    }

    return Strategy(regrets.size(), 1.0f / regrets.size());
}

// std::unordered_map<InfoSet, Strategy> CFR::get_average_strategy() const
// {
//     std::unordered_map<InfoSet, Strategy> average_strategy;

//     for (auto const &[info_set, strategy_sum] : strategy_sum)
//     {
//         float total_regret = 0.0f;
//         for (float val : strategy_sum)
//             total_regret += val;

//         int n = num_actions.at(info_set);

//         if (total_regret > 0)
//         {
//             Strategy average_strategy(n, 0.0f);
//             for (int i = 0; i < n; ++i)
//             {
//                 average_strategy.at(info_set)[i] = strategy_sum[i] / total_regret;
//             }
//         }
//     }
// }