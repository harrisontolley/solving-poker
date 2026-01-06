#pragma once

#include <array>
#include "commontypes.hpp"
#include "headsuptypes.hpp"

struct HeadsUpState
{
    double p1_contribution{SMALL_BLIND};
    double p2_contribution{BIG_BLIND};
    double pot{SMALL_BLIND + BIG_BLIND};

    int betting_round{PREFLOP};
    int player_turn{CHANCE_PLAYER};

    Card p1_card{NO_CARD};
    Card p2_card{NO_CARD};

    std::array<Card, 5> public_cards = {NO_CARD, NO_CARD, NO_CARD, NO_CARD, NO_CARD};

    History preflop_history{H_R_EMPTY};
    History flop_history{H_R_EMPTY};
    History turn_history{H_R_EMPTY};
    History river_history{H_R_EMPTY};
};

class HeadsUpGame
{
public:
    using State = HeadsUpState;
    using Action = HeadsUpAction;
    using InfoSet = ::InfoSet;

    bool verbose{VERBOSE_DEFAULT};
    bool cfr_verbose{CFR_VERBOSE_DEFAULT};

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

protected:
private:
    int get_hand_strength(char private_card, char public_card) const;

    bool is_round_complete(const History &h) const;
};
