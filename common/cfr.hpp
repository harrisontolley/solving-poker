// ===== common/cfr.hpp =====
#pragma once

#include "commontypes.hpp"
#include "datawriter.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace solver_detail
{
    inline Strategy regret_matching(const std::vector<double> &regrets)
    {
        Strategy sigma(regrets.size(), 0.0);
        if (regrets.empty())
            return sigma;

        double sum_pos = 0.0;
        for (double r : regrets)
            sum_pos += std::max(0.0, r);

        if (sum_pos > 0.0)
        {
            for (std::size_t i = 0; i < regrets.size(); ++i)
                sigma[i] = std::max(0.0, regrets[i]) / sum_pos;
        }
        else
        {
            double uniform = 1.0 / static_cast<double>(regrets.size());
            std::fill(sigma.begin(), sigma.end(), uniform);
        }
        return sigma;
    }

    inline Strategy normalise_weights(const std::vector<double> &w)
    {
        Strategy p(w.size(), 0.0);
        if (w.empty())
            return p;

        double total = 0.0;
        for (double x : w)
            total += x;

        if (total > 1e-12)
        {
            for (std::size_t i = 0; i < w.size(); ++i)
                p[i] = w[i] / total;
        }
        else
        {
            double u = 1.0 / static_cast<double>(w.size());
            std::fill(p.begin(), p.end(), u);
        }
        return p;
    }
} // namespace solver_detail

// ============================================================
// CFR (full tree traversal via enumerate_chance_transitions)
// ============================================================

template <class Game>
class CFR
{
public:
    using State = typename Game::State;
    using Action = typename Game::Action;
    using InfoSet = typename Game::InfoSet;

    explicit CFR(Game game)
        : game_{std::move(game)}
    {
        // no op
    }

    void train(int num_iterations);

    StrategyProfile get_average_strategy() const;

    void print_metrics(int num_iterations) const;

    void print_strategies() const;

protected:
    StrategyProfile regret_sum_;
    StrategyProfile strategy_sum_;
    std::unordered_map<InfoSet, int> num_actions_;
    std::unordered_map<InfoSet, std::vector<Action>> actions_by_infoset_;

    virtual void on_regret(InfoSet const &info_set, std::size_t a, double delta) = 0;
    virtual void on_strategy(InfoSet const &info_set, Strategy const &sigma, double reach) = 0;

    int iteration() const noexcept { return iteration_; };

private:
    std::pair<double, double> traverse(State const &state, double p1, double p2);

    void ensure_infoset(InfoSet const &info_set, std::vector<Action> const &actions);

    Strategy regret_match(InfoSet const &info_set);

private:
    Game game_;
    int iteration_{0};

    bool write_log_file_ = WRITE_LOG_FILE;
    DataWriter data_writer_{LOG_FILE_NAME};
};

template <class Game>
class CFRVanilla : public CFR<Game>
{
public:
    using CFR<Game>::CFR;

protected:
    void on_regret(typename Game::InfoSet const &is, std::size_t a, double delta) override
    {
        this->regret_sum_[is][a] += delta;
    }

    void on_strategy(typename Game::InfoSet const &is, Strategy const &sigma, double reach) override
    {
        for (std::size_t a = 0; a < sigma.size(); ++a)
            this->strategy_sum_[is][a] += reach * sigma[a];
    }
};

template <class Game>
class CFRPlus : public CFR<Game>
{
public:
    using CFR<Game>::CFR;

protected:
    void on_regret(typename Game::InfoSet const &is, std::size_t a, double delta) override
    {
        double &r = this->regret_sum_[is][a];
        r = std::max(0.0, r + delta);
    }

    void on_strategy(typename Game::InfoSet const &is, Strategy const &sigma, double /*reach*/) override
    {
        // CFR+ convention in your code: iteration-weighted averaging
        double w = static_cast<double>(this->iteration());
        for (std::size_t a = 0; a < sigma.size(); ++a)
            this->strategy_sum_[is][a] += w * sigma[a];
    }
};

template <class Game>
std::pair<double, double> CFR<Game>::traverse(State const &state, double p1, double p2)
{
    if (game_.is_terminal(state))
        return game_.get_payoffs(state);

    int player = game_.get_current_player(state);

    if (player == CHANCE_PLAYER)
    {
        std::pair<double, double> v{0.0, 0.0};
        for (auto const &[next_state, prob] : game_.enumerate_chance_transitions(state))
        {
            auto child = traverse(next_state, p1, p2);
            v.first += prob * child.first;
            v.second += prob * child.second;
        }
        return v;
    }

    std::vector<Action> actions = game_.get_legal_actions(state);
    InfoSet is = game_.get_information_set(state, player);

    ensure_infoset(is, actions);

    Strategy sigma = regret_match(is);

    std::vector<std::pair<double, double>> util(actions.size());
    std::pair<double, double> node{0.0, 0.0};

    for (std::size_t a = 0; a < actions.size(); ++a)
    {
        State next = game_.transition(state, actions[a]);

        util[a] = (player == PLAYER_1)
                      ? traverse(next, p1 * sigma[a], p2)
                      : traverse(next, p1, p2 * sigma[a]);

        node.first += sigma[a] * util[a].first;
        node.second += sigma[a] * util[a].second;
    }

    double reach = (player == PLAYER_1) ? p1 : p2;
    on_strategy(is, sigma, reach);

    if (player == PLAYER_1)
    {
        for (std::size_t a = 0; a < actions.size(); ++a)
            on_regret(is, a, p2 * (util[a].first - node.first));
    }
    else
    {
        for (std::size_t a = 0; a < actions.size(); ++a)
            on_regret(is, a, p1 * (util[a].second - node.second));
    }

    return node;
}

template <class Game>
void CFR<Game>::ensure_infoset(InfoSet const &is, std::vector<Action> const &actions)
{
    const int n = static_cast<int>(actions.size());

    num_actions_[is] = n;
    actions_by_infoset_[is] = actions;

    auto &r = regret_sum_[is];
    if (static_cast<int>(r.size()) != n)
        r.assign(n, 0.0);

    auto &s = strategy_sum_[is];
    if (static_cast<int>(s.size()) != n)
        s.assign(n, 0.0);
}

template <class Game>
Strategy CFR<Game>::regret_match(InfoSet const &info_set)
{
    Strategy &regrets = regret_sum_[info_set];
    return solver_detail::regret_matching(regrets);
}

template <class Game>
StrategyProfile CFR<Game>::get_average_strategy() const
{
    StrategyProfile avg;

    for (auto const &[info_set, strat_sum] : strategy_sum_)
    {
        avg.emplace(info_set, solver_detail::normalise_weights(strat_sum));
    }

    return avg;
}

template <class Game>
void CFR<Game>::print_metrics(int num_iterations) const
{
    double total_pos = 0.0;
    double max_pos = 0.0;

    for (auto const &[info_set, strategy] : regret_sum_)
    {
        (void)info_set;
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
void CFR<Game>::train(int num_iterations)
{
    int log_every = num_iterations;
    if (NUM_LOG_INTERVALS > 0)
        log_every = std::max(1, num_iterations / NUM_LOG_INTERVALS);

    for (int i = 0; i < num_iterations; ++i)
    {
        iteration_ = i + 1;
        State s = game_.get_initial_state();
        traverse(s, 1.0, 1.0);

        if (write_log_file_ && ((i + 1) % log_every == 0))
        {
            auto avg = get_average_strategy();
            data_writer_.log_metrics(game_, i + 1, avg);
        }

        if (!game_.cfr_verbose)
            continue;

        int denom = std::max(1, num_iterations / VERBOSE_UPDATE_PERCENT);
        if ((i + 1) % denom == 0)
        {
            std::cout << "==== CFR " << ((i + 1) * 100 / num_iterations)
                      << "% complete. ====" << std::endl;
            print_metrics(i + 1);
        }
    }

    std::cout << "Training complete.\n";
    print_strategies();
}

template <class Game>
void CFR<Game>::print_strategies() const
{
    auto avg = get_average_strategy();

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
            for (std::size_t i = 0; i < strat.size(); ++i)
            {
                std::cout << "  Action " << i
                          << " : " << std::fixed << std::setprecision(4)
                          << strat[i] << "\n";
            }
        }
        else
        {
            auto const &actions = it->second;
            for (std::size_t i = 0; i < strat.size() && i < actions.size(); ++i)
            {
                std::cout << "  "
                          << game_.action_to_string(actions[i])
                          << " : " << std::fixed << std::setprecision(4)
                          << strat[i] << "\n";
            }
        }

        std::cout << "\n";
    }
}

// ============================================================
// MCCFR (external sampling via game_.sample_chance)
// ============================================================

template <class Game>
class MCCFR
{
public:
    using State = typename Game::State;
    using Action = typename Game::Action;
    using InfoSet = typename Game::InfoSet;

    explicit MCCFR(Game game)
        : game_{std::move(game)}, rng_(std::random_device{}()) {}

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
        StrategyProfile avg;
        for (auto const &[is, sum] : strategy_sum_)
        {
            avg.emplace(is, solver_detail::normalise_weights(sum));
        }
        return avg;
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
            if (!std::equal(a.begin(), a.end(), actions.begin()))
                mismatch = true;
        }

        if (mismatch)
        {
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

        Strategy sigma = solver_detail::regret_matching(regret_sum_[is]);

        if (player == traversing_player)
        {
            double node_util = 0.0;
            std::vector<double> action_utils(actions.size(), 0.0);

            for (std::size_t i = 0; i < actions.size(); ++i)
            {
                State next = game_.transition(s, actions[i]);
                action_utils[i] = update(next, traversing_player);
                node_util += sigma[i] * action_utils[i];
            }

            for (std::size_t i = 0; i < actions.size(); ++i)
                regret_sum_[is][i] += (action_utils[i] - node_util);

            return node_util;
        }
        else
        {
            std::discrete_distribution<int> dist(sigma.begin(), sigma.end());
            int action_idx = dist(rng_);

            // NOTE: Your comment stands: this averaging is not theoretically correct for MCCFR,
            // but is kept unchanged here to avoid altering behavior during refactor.
            for (std::size_t i = 0; i < sigma.size(); ++i)
                strategy_sum_[is][i] += sigma[i];

            State next = game_.transition(s, actions[action_idx]);
            return update(next, traversing_player);
        }
    }
};
