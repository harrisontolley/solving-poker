#pragma once
#include "commontypes.hpp"
#include "datawriter.hpp
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <random>
#include <stdexcept>

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

            State root = game_.get_initial_state();
            update(root, PLAYER_1);

            root = game_.get_initial_state();
            update(root, PLAYER_2);

            if (iteration_ % 1000 == 0)
                std::cout << "Iteration " << iteration_ << " complete." << std::endl;
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

            Strategy s(sum.size(), 0.0);
            if (sum.empty())
            {
                avg_strat[is] = s;
                continue;
            }

            if (total > 1e-9)
            {
                for (size_t i = 0; i < sum.size(); ++i)
                    s[i] = sum[i] / total;
            }
            else
            {
                double uniform = 1.0 / static_cast<double>(sum.size());
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

private:
    void ensure_infoset(InfoSet const &is, std::vector<Action> const &actions)
    {
        if (actions.empty())
            throw std::runtime_error("MCCFR: reached non-terminal node with 0 legal actions for infoset: " + is);

        auto &r = regret_sum_[is];
        auto &s = strategy_sum_[is];
        auto &a = action_map_[is];

        bool mismatch = (r.size() != actions.size()) || (a.size() != actions.size());
        if (!mismatch)
        {
            // requires Action to have operator==
            if (!std::equal(a.begin(), a.end(), actions.begin()))
                mismatch = true;
        }

        if (mismatch)
        {
            // this indicates imperfect recall merging of states with different legal actions.
            // We reset to avoid OOB / corruption.
            r.assign(actions.size(), 0.0);
            s.assign(actions.size(), 0.0);
            a = actions;
        }
    }

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

        ensure_infoset(is, actions);

        Strategy sigma = get_strategy(regret_sum_[is]);

        if (player == traversing_player)
        {
            double node_util = 0.0;
            std::vector<double> action_utils(actions.size(), 0.0);

            for (size_t i = 0; i < actions.size(); ++i)
            {
                State next = game_.transition(s, actions[i]);
                action_utils[i] = update(next, traversing_player);
                node_util += sigma[i] * action_utils[i];
            }

            for (size_t i = 0; i < actions.size(); ++i)
                regret_sum_[is][i] += (action_utils[i] - node_util);

            return node_util;
        }
        else
        {
            std::discrete_distribution<int> dist(sigma.begin(), sigma.end());
            int action_idx = dist(rng_);

            // ! averaging is not theoretically correct but fine for now
            // todo fix
            for (size_t i = 0; i < sigma.size(); ++i)
                strategy_sum_[is][i] += sigma[i];

            State next = game_.transition(s, actions[action_idx]);
            return update(next, traversing_player);
        }
    }

    Strategy get_strategy(const std::vector<double> &regrets)
    {
        Strategy sigma(regrets.size(), 0.0);
        if (regrets.empty())
            return sigma;

        double sum_pos = 0.0;
        for (double r : regrets)
            sum_pos += std::max(0.0, r);

        if (sum_pos > 0.0)
        {
            for (size_t i = 0; i < regrets.size(); ++i)
                sigma[i] = std::max(0.0, regrets[i]) / sum_pos;
        }
        else
        {
            double uniform = 1.0 / static_cast<double>(regrets.size());
            std::fill(sigma.begin(), sigma.end(), uniform);
        }
        return sigma;
    }
};
