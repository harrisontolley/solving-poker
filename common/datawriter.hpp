#pragma once

#include "commontypes.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>

template <class Game>
using Policy = std::unordered_map<typename Game::InfoSet, Strategy>;

class DataWriter
{
public:
    explicit DataWriter(std::string filename)
        : filename_(std::move(filename)),
          metrics_path_(make_metrics_path(filename_))
    {
        ensure_output_dir_exists();
        // We intentionally do NOT open the metrics file here.
        // It is opened lazily on first write_line() so the same class can be used
        // for both "metrics writer" and "strategy writer" use-cases.
    }

    ~DataWriter()
    {
        close_metrics_stream();
    }

    // HeadsUp MCCFR strategy dumping
    void log_strategy(const StrategyProfile &profile) const
    {
        std::ofstream out(filename_);
        if (!out.is_open())
        {
            std::cerr << "Failed to open strategy file for writing: " << filename_ << "\n";
            return;
        }

        out << "InfoSet,ActionIndex,Prob\n";
        for (auto const &[is, strat] : profile)
        {
            for (std::size_t i = 0; i < strat.size(); ++i)
                out << is << "," << i << "," << strat[i] << "\n";
        }

        std::cout << "Strategy saved to " << filename_ << "\n";
    }

    // Kuhn/Leduc CFR metrics logging
    void write_line(int iteration, double policy_evaluation, double nash_conv)
    {
        open_metrics_stream_if_needed();
        if (!metrics_.is_open())
        {
            std::cerr << "Logfile not open for writing: " << metrics_path_.string() << "\n";
            return;
        }

        metrics_ << iteration << "," << policy_evaluation << "," << nash_conv << "\n";
        metrics_.flush();
    }

    template <class Game>
    void log_metrics(const Game &game, int iteration, const Policy<Game> &policy)
    {
        double policy_eval = evaluate_policy<Game>(game, policy);
        double nc = nash_conv<Game>(game, policy);
        write_line(iteration, policy_eval, nc);
    }

private:
    std::string filename_;

    std::filesystem::path metrics_path_;
    std::ofstream metrics_;

private:
    static std::filesystem::path output_dir()
    {
        return std::filesystem::path{"output"};
    }

    static void ensure_output_dir_exists()
    {
        namespace fs = std::filesystem;

        std::error_code ec;
        fs::path out_dir = output_dir();

        if (!fs::exists(out_dir))
        {
            fs::create_directories(out_dir, ec);
            if (ec)
                std::cerr << "Failed to create output directory '" << out_dir.string()
                          << "': " << ec.message() << "\n";
        }
    }

    static std::filesystem::path make_metrics_path(const std::string &filename)
    {
        namespace fs = std::filesystem;
        fs::path p{filename};

        if (p.is_absolute() || p.has_parent_path())
            return p;

        return output_dir() / p;
    }

    void open_metrics_stream_if_needed()
    {
        if (metrics_.is_open())
            return;

        metrics_.open(metrics_path_, std::ios::out | std::ios::trunc);
        if (!metrics_.is_open())
            std::cerr << "Failed to open log file: " << metrics_path_.string() << "\n";
    }

    void close_metrics_stream()
    {
        if (metrics_.is_open())
            metrics_.close();
    }

private:
    template <class GameT>
    double evaluate_policy_rec(GameT const &game,
                               typename GameT::State const &state,
                               Policy<GameT> const &policy,
                               PlayerId hero)
    {
        using State = typename GameT::State;
        using Action = typename GameT::Action;
        using InfoSet = typename GameT::InfoSet;

        if (game.is_terminal(state))
        {
            auto [u1, u2] = game.get_payoffs(state);
            return (hero == PLAYER_1) ? u1 : u2;
        }

        int player = game.get_current_player(state);

        if (player == CHANCE_PLAYER)
        {
            double v = 0.0;
            for (auto const &entry : game.enumerate_chance_transitions(state))
            {
                State const &next_state = entry.first;
                double prob = entry.second;
                v += prob * evaluate_policy_rec<GameT>(game, next_state, policy, hero);
            }
            return v;
        }

        std::vector<Action> actions = game.get_legal_actions(state);
        if (actions.empty())
            throw std::runtime_error("DataWriter::evaluate_policy_rec: non-terminal node has 0 legal actions.");

        InfoSet infoset = game.get_information_set(state, player);

        Strategy const *sigma_ptr = nullptr;
        auto it = policy.find(infoset);
        if (it != policy.end())
            sigma_ptr = &it->second;

        double v = 0.0;

        if (sigma_ptr && sigma_ptr->size() == actions.size())
        {
            Strategy const &sigma = *sigma_ptr;
            for (std::size_t i = 0; i < actions.size(); ++i)
            {
                State next_state = game.transition(state, actions[i]);
                v += sigma[i] * evaluate_policy_rec<GameT>(game, next_state, policy, hero);
            }
        }
        else
        {
            // uniform random if no policy defined for this infoset
            double uniform_prob = 1.0 / static_cast<double>(actions.size());
            for (auto const &a : actions)
            {
                State next_state = game.transition(state, a);
                v += uniform_prob * evaluate_policy_rec<GameT>(game, next_state, policy, hero);
            }
        }

        return v;
    }

    template <class GameT>
    double evaluate_policy(GameT const &game, Policy<GameT> const &policy)
    {
        // by convention return player 1's value vs itself
        return evaluate_policy_rec<GameT>(game, game.get_initial_state(), policy, PLAYER_1);
    }

    template <class GameT>
    double best_response_rec(GameT const &game,
                             typename GameT::State const &state,
                             Policy<GameT> const &opp_policy,
                             PlayerId hero)
    {
        using State = typename GameT::State;
        using Action = typename GameT::Action;
        using InfoSet = typename GameT::InfoSet;

        if (game.is_terminal(state))
        {
            auto [u1, u2] = game.get_payoffs(state);
            return (hero == PLAYER_1) ? u1 : u2;
        }

        int player = game.get_current_player(state);

        if (player == CHANCE_PLAYER)
        {
            double v = 0.0;
            for (auto const &[next_state, prob] : game.enumerate_chance_transitions(state))
            {
                v += prob * best_response_rec<GameT>(game, next_state, opp_policy, hero);
            }
            return v;
        }

        std::vector<Action> actions = game.get_legal_actions(state);
        if (actions.empty())
            throw std::runtime_error("DataWriter::best_response_rec: non-terminal node has 0 legal actions.");

        if (player == hero)
        {
            // maximize over actions
            double best = -std::numeric_limits<double>::infinity();
            for (auto const &a : actions)
            {
                State next_state = game.transition(state, a);
                double v = best_response_rec<GameT>(game, next_state, opp_policy, hero);
                best = std::max(best, v);
            }
            return best;
        }

        // Opponent plays fixed strategy
        InfoSet infoset = game.get_information_set(state, player);

        Strategy const *sigma_ptr = nullptr;
        auto it = opp_policy.find(infoset);
        if (it != opp_policy.end())
            sigma_ptr = &it->second;

        double v = 0.0;

        if (sigma_ptr && sigma_ptr->size() == actions.size())
        {
            Strategy const &sigma = *sigma_ptr;
            for (std::size_t i = 0; i < actions.size(); ++i)
            {
                State next_state = game.transition(state, actions[i]);
                v += sigma[i] * best_response_rec<GameT>(game, next_state, opp_policy, hero);
            }
        }
        else
        {
            double uniform = 1.0 / static_cast<double>(actions.size());
            for (auto const &a : actions)
            {
                State next_state = game.transition(state, a);
                v += uniform * best_response_rec<GameT>(game, next_state, opp_policy, hero);
            }
        }

        return v;
    }

    template <class GameT>
    double best_response_value(GameT const &game, Policy<GameT> const &opp_policy, PlayerId hero)
    {
        return best_response_rec<GameT>(game, game.get_initial_state(), opp_policy, hero);
    }

    template <class GameT>
    double nash_conv(GameT const &game, Policy<GameT> const &policy)
    {
        double br1 = best_response_value<GameT>(game, policy, PLAYER_1);
        double br2 = best_response_value<GameT>(game, policy, PLAYER_2);
        return br1 + br2;
    }

    template <class GameT>
    double exploitability(GameT const &game, Policy<GameT> const &policy)
    {
        return 0.5 * nash_conv<GameT>(game, policy);
    }
};
