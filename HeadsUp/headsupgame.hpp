#pragma once

#include <array>
#include "commontypes.hpp"
#include "headsuptypes.hpp"
#include <random>

struct HeadsUpState
{
    double p1_contribution{SMALL_BLIND};
    double p2_contribution{BIG_BLIND};
    double pot{SMALL_BLIND + BIG_BLIND};

    // Stack tracking is implicit: STACK_SIZE - contribution

    int betting_round{PREFLOP};
    int player_turn{PLAYER_1}; // Preflop P1 (SB/Button) acts first

    bool round_is_over{false};
    bool game_over{false};

    Card p1_card_1{NO_CARD};
    Card p1_card_2{NO_CARD};
    Card p2_card_1{NO_CARD};
    Card p2_card_2{NO_CARD};

    std::vector<Card> board_cards;

    // We store the deck in the state to make chance sampling efficient and correct
    std::vector<Card> deck;

    History current_round_history{H_R_EMPTY};
};

class HeadsUpGame
{
public:
    using State = HeadsUpState;
    using Action = HeadsUpAction;
    using InfoSet = ::InfoSet;

    HeadsUpGame();

    State get_initial_state(); // Not const, uses RNG to shuffle initial deck

    bool is_terminal(State const &state) const;

    int get_current_player(State const &state) const;

    std::vector<Action> get_legal_actions(State const &state) const;

    State transition(State const &state, Action action) const;

    // MCCFR specific: Sample ONE chance outcome
    State sample_chance(State const &state);

    std::pair<double, double> get_payoffs(State const &state) const;

    // Abstraction Logic
    InfoSet get_information_set(State const &state, int player) const;

    std::string get_abstraction_key(State const &state, int player) const;

    std::string action_to_string(Action a) const;

private:
    // Internal Evaluation Helpers
    double calculate_hand_strength(Card c1, Card c2, const std::vector<Card> &board) const;

    int get_rank_idx(Card c1, Card c2, const std::vector<Card> &board) const;

    std::string card_to_string(Card c) const;

    std::uint64_t strength_cache_key(Card c1, Card c2, const std::vector<Card> &board) const;

private:
    mutable std::mt19937 rng;

    // Cache for deterministic postflop abstraction strength values
    mutable std::unordered_map<std::uint64_t, double> strength_cache_;
};