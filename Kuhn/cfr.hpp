#pragma once
#include "commontypes.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <iomanip>
template <class Game>
class CFR
{
public:
    using State = typename Game::State;
    using Action = typename Game::Action;
    using InfoSet = typename Game::InfoSet;

    explicit CFR(Game game)
        : game_{std::move(game)} {}

    void train(int num_iterations);

    std::unordered_map<InfoSet, Strategy> get_average_strategy() const;

    void print_metrics(int num_iterations) const;

    void print_strategies() const;

private:
    Game game_;

    std::unordered_map<InfoSet, Strategy> regret_sum_;
    std::unordered_map<InfoSet, Strategy> strategy_sum_;
    std::unordered_map<InfoSet, int> num_actions_;

    std::unordered_map<InfoSet, std::vector<Action>> actions_by_infoset_;

    Strategy regret_match(InfoSet const &info_set);
    std::pair<double, double> cfr_iterate(State const &state, double p1, double p2);
};

template <class Game>
void CFR<Game>::print_metrics(int num_iterations) const
{
    double total_pos = 0.0;
    double max_pos = 0.0;

    for (auto const &[info_set, strategy] : regret_sum_)
    {
        for (double val : strategy)
        {
            double pos = std::max(0.0, val);
            total_pos += pos;
            if (pos > max_pos)
                max_pos = pos;
        }
    }

    std::cout << "Avg pos regret / iter = " << (total_pos / num_iterations) << "\n";
    std::cout << "Max pos regret / iter = " << (max_pos / num_iterations) << "\n";
}

template <class Game>
Strategy CFR<Game>::regret_match(InfoSet const &info_set)
{
    Strategy &regrets = regret_sum_[info_set]; // creates if missing
    Strategy positive(regrets.size(), 0.0);
    double total = 0.0;

    for (size_t i = 0; i < regrets.size(); ++i)
    {
        positive[i] = std::max(0.0, regrets[i]);
        total += positive[i];
    }

    Strategy sigma(regrets.size(), 0.0);
    if (total > 0.0)
    {
        for (size_t i = 0; i < regrets.size(); ++i)
            sigma[i] = positive[i] / total;
    }
    else if (!sigma.empty())
    {
        double uniform = 1.0 / sigma.size();
        for (double &p : sigma)
            p = uniform;
    }

    return sigma;
}

template <class Game>
std::unordered_map<typename CFR<Game>::InfoSet, Strategy>
CFR<Game>::get_average_strategy() const
{
    std::unordered_map<InfoSet, Strategy> average_strategy;

    for (auto const &[info_set, strat_sum] : strategy_sum_)
    {
        double total = 0.0;
        for (double v : strat_sum)
            total += v;

        int n = num_actions_.at(info_set);
        Strategy strat(n, 0.0);

        if (total > 0.0)
        {
            for (int i = 0; i < n; ++i)
                strat[i] = strat_sum[i] / total;
        }
        else if (n > 0)
        {
            double uniform = 1.0 / n;
            for (int i = 0; i < n; ++i)
                strat[i] = uniform;
        }

        average_strategy.emplace(info_set, std::move(strat));
    }

    return average_strategy;
}

template <class Game>
void CFR<Game>::train(int num_iterations)
{
    for (int i = 0; i < num_iterations; ++i)
    {
        State s = game_.get_initial_state();
        cfr_iterate(s, 1.0, 1.0);

        if ((i + 1) % 1000 == 0)
            print_metrics(i + 1);
    }

    std::cout << "Training complete.\n";
    print_strategies();
}

// Single CFR recursion iteration with chance-sampling
// Returns (counterfactual for p1, counterfactual for p2)
template <class Game>
std::pair<double, double>
CFR<Game>::cfr_iterate(State const &state, double p1, double p2)
{
    if (game_.is_terminal(state))
        return game_.get_payoffs(state);

    int player = game_.get_current_player(state);
    if (player == CHANCE_PLAYER)
    {
        auto [next_state, prob] = game_.chance_transition(state);
        (void)prob; // unused
        return cfr_iterate(next_state, p1, p2);
    }

    InfoSet infoset = game_.get_information_set(state, player);
    std::vector<Action> actions = game_.get_legal_actions(state);
    int n = static_cast<int>(actions.size());

    // init storage on first visit
    if (!regret_sum_.contains(infoset))
    {
        regret_sum_[infoset] = Strategy(n, 0.0);
        strategy_sum_[infoset] = Strategy(n, 0.0);
        num_actions_[infoset] = n;
        actions_by_infoset_[infoset] = actions;
    }

    Strategy sigma = regret_match(infoset);

    double reach = (player == PLAYER_1) ? p1 : p2;
    for (int i = 0; i < n; ++i)
        strategy_sum_[infoset][i] += reach * sigma[i];

    // recurse on each action
    std::vector<std::pair<double, double>> child_values(n);
    std::pair<double, double> node_value{0.0, 0.0};

    for (int i = 0; i < n; ++i)
    {
        State next_state = game_.transition(state, actions[i]);
        auto child = (player == PLAYER_1)
                         ? cfr_iterate(next_state, p1 * sigma[i], p2)
                         : cfr_iterate(next_state, p1, p2 * sigma[i]);

        child_values[i] = child;
        node_value.first += sigma[i] * child.first;
        node_value.second += sigma[i] * child.second;
    }

    double opp_reach = (player == PLAYER_1) ? p2 : p1;
    int util_idx = (player == PLAYER_1) ? 0 : 1;

    for (int i = 0; i < n; ++i)
    {
        double child_util = (util_idx == 0)
                                ? child_values[i].first
                                : child_values[i].second;
        double node_util = (util_idx == 0)
                               ? node_value.first
                               : node_value.second;
        double regret = child_util - node_util;
        regret_sum_[infoset][i] += opp_reach * regret;
    }

    return node_value;
}

template <class Game>
void CFR<Game>::print_strategies() const
{
    auto avg = get_average_strategy();

    // Collect and sort infosets for deterministic output
    std::vector<InfoSet> keys;
    keys.reserve(avg.size());
    for (auto const &kv : avg)
        keys.push_back(kv.first);

    std::sort(keys.begin(), keys.end());

    std::cout << "Average strategy by information set:\n";

    for (auto const &infoset : keys)
    {
        auto const &strat = avg.at(infoset);
        std::cout << "InfoSet: " << infoset << "\n";

        auto it = actions_by_infoset_.find(infoset);

        if (it == actions_by_infoset_.end())
        {
            // Fallback
            for (size_t i = 0; i < strat.size(); ++i)
            {
                std::cout << "  Action " << i
                          << " : " << std::fixed << std::setprecision(4)
                          << strat[i] << "\n";
            }
        }
        else
        {
            auto const &actions = it->second;
            for (size_t i = 0; i < strat.size() && i < actions.size(); ++i)
            {
                std::cout << "  "
                          << game_.action_to_string(actions[i]) // Game-specific label
                          << " : " << std::fixed << std::setprecision(4)
                          << strat[i] << "\n";
            }
        }

        std::cout << "\n";
    }
}
