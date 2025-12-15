#pragma once

#include "commontypes.hpp"
#include "datawriter.hpp"
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
    // base owned traversal
    std::pair<double, double> traverse(State const &state, double p1, double p2);

    // ensure vectors are sized and action ordering remembered
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
        // plain accumulate
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
        // CFR+: cumulative regrets are clamped at 0
        double &r = this->regret_sum_[is][a];
        r = std::max(0.0, r + delta);
    }

    void on_strategy(typename Game::InfoSet const &is, Strategy const &sigma, double reach) override
    {
        // linear weighting by iteration (t)
        double w = static_cast<double>(this->iteration());
        for (std::size_t a = 0; a < sigma.size(); ++a)
            this->strategy_sum_[is][a] += w * reach * sigma[a];
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

    // average strategy accumulation for the CURRENT player
    double reach = (player == PLAYER_1) ? p1 : p2;
    on_strategy(is, sigma, reach);

    // CFR update (opponent reach weights regrets)
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
StrategyProfile CFR<Game>::get_average_strategy() const
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
    // Determine how often to log
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

        if ((i + 1) % (num_iterations / VERBOSE_UPDATE_PERCENT) == 0)
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
