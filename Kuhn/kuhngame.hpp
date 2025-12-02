#include "types.hpp"

#include <string>
#include <vector>
#include <tuple>
#include <array>
#include <utility>

#pragma once

struct KuhnState
{
    int p1_contribution{0};
    int p2_contribution{0};

    int pot{0};

    History history{""};

    CardsDealt cards_dealt{""};
};

class KuhnGame
{
public:
    KuhnGame() = default;

    inline static constexpr std::array<char, 3> CARDS{'J', 'Q', 'K'};

    KuhnState get_initial_state() const;

    bool is_terminal(KuhnState const &state) const;

    int get_current_player(KuhnState const &state) const;

    std::vector<char> get_legal_actions(KuhnState const &state) const;

    KuhnState transition(KuhnState const &state, Action action) const;

    std::tuple<KuhnState, float> chance_transition(KuhnState const &state) const;

    std::pair<int, int> get_payoffs(KuhnState const &state) const;

    std::string get_information_set(KuhnState const &state, int player) const;

    void print_game_state(KuhnState const &state) const;

    char read_player_action() const;
};
