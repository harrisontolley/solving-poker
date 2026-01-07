#pragma once
#include "commontypes.hpp"
#include "headsupwriter.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <random>

template <class Game>
class MCCFR
{
public:
    using State = typename Game::State;
    using Action = typename Game::Action;
    using InfoSet = typename Game::InfoSet;

    explicit MCCFR(Game game) : game_{std::move(game)}, rng_(std::random_device{}()) {}

    void train(int num_iterations)
    {
        for (int i = 0; i < num_iterations; ++i)
        {
            iteration_++;
            // Update Player 1
            State root = game_.get_initial_state();
            update(root, PLAYER_1);

            // Update Player 2
            root = game_.get_initial_state();
            update(root, PLAYER_2);

            if (iteration_ % 1000 == 0)
            {
                std::cout << "Iteration " << iteration_ << " complete." << std::endl;
            }
        }

        if (WRITE_LOG_FILE)
        {
            DataWriter w(LOG_FILE_NAME);
            w.log_strategy(get_average_strategy());
        }
    }

    StrategyProfile get_average_strategy() const
    {
        StrategyProfile avg_strat;
        for (auto const &[is, sum] : strategy_sum_)
        {
            double total = 0.0;
            for (double v : sum)
                total += v;

            Strategy s(sum.size());
            if (total > 1e-9)
            {
                for (size_t i = 0; i < sum.size(); ++i)
                    s[i] = sum[i] / total;
            }
            else
            {
                double uniform = 1.0 / sum.size();
                std::fill(s.begin(), s.end(), uniform);
            }
            avg_strat[is] = s;
        }
        return avg_strat;
    }

private:
    Game game_;
    int iteration_{0};
    std::mt19937 rng_;

    StrategyProfile regret_sum_;
    StrategyProfile strategy_sum_;
    std::unordered_map<InfoSet, std::vector<Action>> action_map_;

    double update(State s, int traversing_player)
    {
        if (game_.is_terminal(s))
        {
            auto payoffs = game_.get_payoffs(s);
            return (traversing_player == PLAYER_1) ? payoffs.first : payoffs.second;
        }

        int player = game_.get_current_player(s);

        if (player == CHANCE_PLAYER)
        {
            State next = game_.sample_chance(s);
            return update(next, traversing_player);
        }

        InfoSet is = game_.get_information_set(s, player);
        std::vector<Action> actions = game_.get_legal_actions(s);

        if (regret_sum_.find(is) == regret_sum_.end())
        {
            regret_sum_[is].resize(actions.size(), 0.0);
            strategy_sum_[is].resize(actions.size(), 0.0);
            action_map_[is] = actions;
        }

        Strategy sigma = get_strategy(regret_sum_[is]);

        if (player == traversing_player)
        {
            double node_util = 0.0;
            std::vector<double> action_utils(actions.size());

            for (size_t i = 0; i < actions.size(); ++i)
            {
                State next = game_.transition(s, actions[i]);
                action_utils[i] = update(next, traversing_player);
                node_util += sigma[i] * action_utils[i];
            }

            for (size_t i = 0; i < actions.size(); ++i)
            {
                regret_sum_[is][i] += (action_utils[i] - node_util);
            }
            return node_util;
        }
        else
        {
            std::discrete_distribution<int> dist(sigma.begin(), sigma.end());
            int action_idx = dist(rng_);

            for (size_t i = 0; i < sigma.size(); ++i)
            {
                strategy_sum_[is][i] += sigma[i];
            }

            State next = game_.transition(s, actions[action_idx]);
            return update(next, traversing_player);
        }
    }

    Strategy get_strategy(const std::vector<double> &regrets)
    {
        double sum_pos = 0.0;
        Strategy sigma(regrets.size());
        for (double r : regrets)
            sum_pos += std::max(0.0, r);

        for (size_t i = 0; i < regrets.size(); ++i)
        {
            if (sum_pos > 0)
                sigma[i] = std::max(0.0, regrets[i]) / sum_pos;
            else
                sigma[i] = 1.0 / regrets.size();
        }
        return sigma;
    }
};