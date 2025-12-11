#pragma once
#include "kuhntypes.hpp"
#include "commontypes.hpp"
#include <array>
#include <tuple>
#include <utility>

struct KuhnState
{
    double p1_contribution{ANTE};
    double p2_contribution{ANTE};
    double pot{2 * ANTE};

    History history{H_R_EMPTY};

    Card p1_card{NO_CARD};
    Card p2_card{NO_CARD};
};

class KuhnGame
{
public:
    using State = KuhnState;
    using Action = KuhnAction;
    using InfoSet = ::InfoSet; // from commontypes

    bool verbose{VERBOSE_DEFAULT};
    bool cfr_verbose{CFR_VERBOSE_DEFAULT};

    inline static constexpr std::array<char, 3> CARDS{'J', 'Q', 'K'};

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
    int card_rank(char c) const;
};
