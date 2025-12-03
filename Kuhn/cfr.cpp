#include "cfr.hpp"
#include "types.hpp"
#include <iostream>

void CFR::print_metrics(int num_iterations) const
{
    float total_pos = 0;
    float max_pos = 0;

    for (auto const &[info_set, strategy] : regret_sum)
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

std::unordered_map<InfoSet, Strategy> CFR::get_average_strategy() const
{
    std::unordered_map<InfoSet, Strategy> average_strategy;

    for (auto const &[info_set, strategy_sum] : strategy_sum)
    {
        float total_regret = 0.0f;
        for (float val : strategy_sum)
            total_regret += val;

        int n = num_actions.at(info_set);

        average_strategy[info_set] = Strategy(n, 0.0f);
        if (total_regret > 0)
        {
            for (int i = 0; i < n; ++i)
            {
                average_strategy[info_set][i] = strategy_sum[i] / total_regret;
            }
        }
        else
        {
            average_strategy[info_set] = std::vector<float>(n, 1.0f / n);
        }
    }

    return average_strategy;
}

void CFR::train(int num_iterations)
{
    for (int i = 0; i < num_iterations; ++i)
    {
        KuhnState state{};
        cfr_iterate(state, 1.0f, 1.0f);

        if ((i + 1) % 1000 == 0)
            print_metrics(i + 1);
    }

    std::cout << "Training complete.\n";
    for (auto const &[info_set, strategy] : get_average_strategy())
    {
        std::cout << "InfoSet: " << info_set << " Strategy: ";
        for (float prob : strategy)
        {
            std::cout << prob << ", ";
        }
        std::cout << "\n";
    }
}

// Single CFR recursion iteration with chance-sampling
// Returns (counterfactual for p1, counterfactual for p2)
std::pair<float, float> CFR::cfr_iterate(KuhnState &state, float p1, float p2)
{
    if (game.is_terminal(state))
        return game.get_payoffs(state);

    int player = game.get_current_player(state);
    if (player == CHANCE_PLAYER)
    {
        auto [next_state, prob] = game.chance_transition(state);
        return cfr_iterate(next_state, p1, p2);
    }

    InfoSet current_infoset = game.get_information_set(state, player);
    ActionSet legal_actions = game.get_legal_actions(state);
    int n = legal_actions.size();

    // Initialise if first visit
    if (!(regret_sum.contains(current_infoset)))
    {
        regret_sum[current_infoset] = Strategy(n, 0.0f);
        strategy_sum[current_infoset] = Strategy(n, 0.0f);
        num_actions[current_infoset] = n;
    }

    // Accumulate strategy sums
    Strategy sigma = regret_match(current_infoset);
    float reach = (player == PLAYER_1) ? p1 : p2;
    for (int i = 0; i < n; ++i)
    {
        strategy_sum[current_infoset][i] += reach * sigma[i];
    }

    // recursive
    std::vector<std::pair<float, float>> children_values;
    std::pair<float, float> node_value{0.0f, 0.0f};

    for (int i = 0; i < n; ++i)
    {
        KuhnState next_state = game.transition(state, legal_actions[i]);
        std::pair<float, float> child_value;
        if (player == PLAYER_1)
            child_value = cfr_iterate(next_state, p1 * sigma[i], p2);
        else
            child_value = cfr_iterate(next_state, p1, p2 * sigma[i]);

        children_values.push_back(child_value);
        node_value.first += sigma[i] * child_value.first;
        node_value.second += sigma[i] * child_value.second;
    }

    // immediate counterfactual regret update
    double opponent_reach = (player == PLAYER_1) ? p2 : p1;
    int util_idx = (player == PLAYER_1) ? 0 : 1;

    for (int i = 0; i < n; ++i)
    {
        float child_util = (util_idx == 0) ? children_values[i].first : children_values[i].second;
        float node_util = (util_idx == 0) ? node_value.first : node_value.second;
        float regret = child_util - node_util;

        regret_sum[current_infoset][i] += opponent_reach * regret;
    }

    return node_value;
}
