#pragma once

#include <iostream>
#include <fstream>
#include "commontypes.hpp"
#include <unordered_map>
#include <vector>
#include <limits>

template <class Game>
using Policy = std::unordered_map<typename Game::InfoSet, Strategy>;

class DataWriter
{
public:
    std::fstream logfile;

    DataWriter(const std::string &filename)
    {
        logfile.open(filename, std::ios::out | std::ios::trunc);
    }

    ~DataWriter()
    {
        if (logfile.is_open())
            logfile.close();
    }

    void write_line(const int iteration, double policy_evaluation, double nash_conv)
    {
        if (logfile.is_open())
        {
            logfile << iteration << "," << policy_evaluation << "," << nash_conv << "\n";
            logfile.flush();
        }
        else
            std::cerr << "Logfile not open for writing.\n";
    }

    template <class Game>
    void log_metrics(const Game &game, const int iteration, const Policy<Game> &policy)
    {
        double policy_eval = evaluate_policy<Game>(game, policy);
        double nc = nash_conv<Game>(game, policy);
        write_line(iteration, policy_eval, nc);
    }

private:
    template <class Game>
    double evaluate_policy_rec(Game const &game, typename Game::State const &state, Policy<Game> const &policy, PlayerId hero)
    {
        using State = typename Game::State;
        using Action = typename Game::Action;
        using InfoSet = typename Game::InfoSet;

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
                v += prob * evaluate_policy_rec<Game>(game, next_state, policy, hero);
            }
            return v;
        }

        std::vector<Action> actions = game.get_legal_actions(state);
        InfoSet infoset = game.get_information_set(state, player);

        Strategy const *sigma_ptr = nullptr;
        auto it = policy.find(infoset);
        if (it != policy.end())
            sigma_ptr = &it->second; // found

        double v = 0.0;
        if (sigma_ptr && sigma_ptr->size() == actions.size())
        {
            Strategy const &sigma = *sigma_ptr;
            for (std::size_t i = 0; i < actions.size(); ++i)
            {
                State next_state = game.transition(state, actions[i]);
                v += sigma[i] * evaluate_policy_rec<Game>(game, next_state, policy, hero);
            }
        }
        else
        {
            // uniform random if no policy defined for this infoset
            double uniform_prob = 1.0 / actions.size();
            for (auto a : actions)
            {
                State next_state = game.transition(state, a);
                v += uniform_prob * evaluate_policy_rec<Game>(game, next_state, policy, hero);
            }
        }
        return v;
    }

    template <class Game>
    double evaluate_policy(Game const &game, Policy<Game> const &policy)
    {
        // by convention return player 1s value vs itself
        return evaluate_policy_rec<Game>(game, game.get_initial_state(), policy, PLAYER_1);
    }

    template <class Game>
    double best_response_rec(Game const &game, typename Game::State const &state, Policy<Game> const &opp_policy, PlayerId hero)
    {
        using State = typename Game::State;
        using Action = typename Game::Action;
        using InfoSet = typename Game::InfoSet;

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
                v += prob * best_response_rec<Game>(game, next_state, opp_policy, hero);
            }
            return v;
        }

        std::vector<Action> actions = game.get_legal_actions(state);

        if (player == hero)
        {
            // maximize over actions
            double best = -std::numeric_limits<double>::infinity();
            for (auto a : actions)
            {
                State next_state = game.transition(state, a);
                double v = best_response_rec<Game>(game, next_state, opp_policy, hero);
                if (v > best)
                    best = v;
            }
            return best;
        }
        else
        {
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
                    v += sigma[i] * best_response_rec<Game>(game, next_state, opp_policy, hero);
                }
            }
            else
            {
                double uniform = 1.0 / actions.size();
                for (auto a : actions)
                {
                    State next_state = game.transition(state, a);
                    v += uniform * best_response_rec<Game>(
                                       game, next_state, opp_policy, hero);
                }
            }
            return v;
        }
    }

    template <class Game>
    double best_response_value(Game const &game, Policy<Game> const &opp_policy, PlayerId hero)
    {
        return best_response_rec<Game>(game, game.get_initial_state(), opp_policy, hero);
    }

    template <class Game>
    double nash_conv(Game const &game, Policy<Game> const &policy)
    {
        double br1 = best_response_value<Game>(game, policy, PLAYER_1);
        double br2 = best_response_value<Game>(game, policy, PLAYER_2);

        return br1 + br2;
    }

    template <class Game>
    double exploitability(Game const &game, Policy<Game> const &policy)
    {
        return 0.5 * nash_conv<Game>(game, policy);
    }
};