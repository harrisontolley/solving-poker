#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include "kuhngame.hpp"
#include "types.hpp"
#pragma once

class CFR
{
public:
    CFR() = default;

    KuhnGame game{};

    void train(int num_iterations);

    std::unordered_map<InfoSet, Strategy> get_average_strategy() const;

    void print_metrics(int num_iterations) const;

private:
    std::unordered_map<InfoSet, Strategy> regret_sum;
    std::unordered_map<InfoSet, Strategy> strategy_sum;
    std::unordered_map<InfoSet, int> num_actions;

    std::pair<float, float> cfr_iterate(KuhnState &state, float p1, float p2);

    Strategy regret_match(InfoSet const &info_set);
};