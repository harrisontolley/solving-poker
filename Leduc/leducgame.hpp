#pragma once
#include <array>
#include "leductypes.hpp"

struct LeducState
{
    double p1_contribution{ANTE};
    double p2_contribution{ANTE};
    double pot{2.0 * ANTE};

    int betting_round{PREFLOP};
    History preflop{H_R_EMPTY};
    History flop{H_R_EMPTY};

    Card p1_card{NO_CARD};
    Card p2_card{NO_CARD};
    Card public_card{NO_CARD};

    int player_turn{CHANCE_PLAYER};
};

class LeducGame
{
public:
    using State = LeducState;
    using Action = LeducAction;
    using InfoSet = ::InfoSet; // from commontypes.hpp

    bool verbose{VERBOSE_DEFAULT};
    bool cfr_verbose{CFR_VERBOSE_DEFAULT};

    inline static constexpr std::array<char, 6> CARDS{'J', 'j', 'Q', 'q', 'K', 'k'};

    State get_initial_state() const;

    bool is_terminal(State const &state) const;
    int get_current_player(State const &state) const;

    std::vector<Action> get_legal_actions(State const &state) const;

    State transition(State const &state, Action action) const;

    std::pair<State, double> chance_transition(State const &state) const;

    std::pair<double, double> get_payoffs(State const &state) const;

    InfoSet get_information_set(State const &state, int player) const;

    void print_game_state(State const &state) const;

    std::string action_to_string(Action a) const;

    std::vector<std::pair<State, double>> enumerate_chance_transitions(State const &state) const;

private:
    int get_hand_strength(char private_card, char public_card) const;
};
