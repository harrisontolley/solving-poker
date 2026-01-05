#include "leducgame.hpp"
#include <stdexcept>
#include <tuple>
#include <string>
#include <vector>
#include <random>
#include <cctype>
#include <iostream>

namespace
{
    std::mt19937 rng{std::random_device{}()};
}

LeducState LeducGame::get_initial_state() const
{
    return LeducState{};
}

bool LeducGame::is_round_complete(const History &h) const
{
    if (h.empty())
        return false;

    char last = h.back();

    // fold ends the round (and game)
    if (last == FOLD or h == "CC")
        return true;

    // Betting has settled: call ending a sequence involing bet or raise
    // e.g. BC or BRC
    if (last == CALL)
    {
        bool has_aggression = (h.find(BET) != std::string::npos) || (h.find(RAISE) != std::string::npos);
        if (has_aggression)
            return true;
    }

    return false;
}

bool LeducGame::is_terminal(LeducState const &state) const
{
    const History &h = (state.betting_round == PREFLOP) ? state.preflop : state.flop;

    if (h.empty())
        return false;

    if (h.back() == FOLD)
        return true;

    if (state.betting_round == FLOP && is_round_complete(h))
        return true;

    return false;
}

int LeducGame::get_current_player(LeducState const &state) const
{
    return state.player_turn;
}

std::vector<LeducAction> LeducGame::get_legal_actions(LeducState const &state) const
{
    if (state.player_turn == CHANCE_PLAYER)
        return {}; // No legal actions for chance player

    if (is_terminal(state))
        return {};

    History const &h = (state.betting_round == PREFLOP) ? state.preflop : state.flop;

    // Start of round or after a check
    if (h.empty() || h == "C")
        return {CALL, BET};

    int aggresive_actions = 0;
    for (char c : h)
    {
        if (c == BET || c == RAISE)
            aggresive_actions++;
    }

    // facing aggresion
    std::vector<LeducAction> actions = {FOLD, CALL};
    if (aggresive_actions < MAX_AGGRESIVE_ACTIONS)
        actions.push_back(RAISE);

    return actions;
}

LeducState LeducGame::transition(LeducState const &state, LeducAction action) const
{
    LeducState new_state = state;

    // Append to the correct round history
    History &h = (state.betting_round == PREFLOP) ? new_state.preflop : new_state.flop;
    h += action;

    double increment = (state.betting_round == PREFLOP) ? PREFLOP_BET_INCREMENT : FLOP_BET_INCREMENT;
    double &actor_contrib = (state.player_turn == PLAYER_1) ? new_state.p1_contribution : new_state.p2_contribution;
    double &opp_contrib = (state.player_turn == PLAYER_1) ? new_state.p2_contribution : new_state.p1_contribution;

    if (action == FOLD)
    {
        // terminal state reached, no money changes
    }
    else if (action == CALL)
    {
        double amount_to_call = opp_contrib - actor_contrib;
        if (amount_to_call > 0)
        {
            actor_contrib += amount_to_call;
            new_state.pot += amount_to_call;
        }
    }
    else if (action == BET)
    {
        actor_contrib += increment;
        new_state.pot += increment;
    }
    else if (action == RAISE)
    {
        // Matches current deficit + adds increment
        double amount_to_call = opp_contrib - actor_contrib;
        double total_add = amount_to_call + increment;
        actor_contrib += total_add;
        new_state.pot += total_add;
    }

    // determine state progression
    bool round_ended = is_round_complete(h);
    bool folded = (!h.empty() && h.back() == FOLD);

    if (!round_ended)
    {
        // switch turns
        new_state.player_turn = (state.player_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;
    }
    else
    {
        if (folded)
        {
            // round over - turn doesn't matter much, but keeping consistent
            new_state.player_turn = (state.player_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;
        }
        else
        {
            // round finished
            if (state.betting_round == PREFLOP)
            {
                // go to flop
                new_state.player_turn = CHANCE_PLAYER;
            }
            else
            {
                // showdown
                new_state.player_turn = (state.player_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;
            }
        }
    }

    return new_state;
}

std::pair<LeducState, double> LeducGame::chance_transition(LeducState const &state) const
{
    if (state.public_card != NO_CARD && state.p1_card != NO_CARD && state.p2_card != NO_CARD)
        throw std::runtime_error("Chance transition called in non-chance state");

    LeducState new_state = state;

    std::vector<char> remaining_cards;
    for (char card : CARDS)
    {
        std::string card_str(1, card);
        if (state.p1_card != card_str &&
            state.p2_card != card_str &&
            state.public_card != card_str)
        {
            remaining_cards.push_back(card);
        }
    }

    if (remaining_cards.empty())
        throw std::runtime_error("No remaining cards in deck");

    std::uniform_int_distribution<int> dist(0, static_cast<int>(remaining_cards.size()) - 1);
    int idx = dist(rng);
    char drawn = remaining_cards[idx];

    if (state.p1_card == NO_CARD)
    {
        new_state.p1_card = std::string(1, drawn);
        new_state.player_turn = CHANCE_PLAYER;
    }
    else if (state.p2_card == NO_CARD)
    {
        new_state.p2_card = std::string(1, drawn);
        new_state.player_turn = PLAYER_1;
    }
    else if (state.public_card == NO_CARD)
    {
        new_state.public_card = std::string(1, drawn);
        new_state.betting_round = FLOP;
        new_state.player_turn = PLAYER_1;
    }

    return {new_state, 1.0 / static_cast<double>(remaining_cards.size())};
}

std::string LeducGame::get_information_set(LeducState const &state, int player) const
{
    const Card &priv = (player == PLAYER_1 ? state.p1_card : state.p2_card);
    std::string pub = (state.public_card == NO_CARD) ? "_" : state.public_card;

    // include player id to avoid collisions between P1 and P2 infosets
    return std::to_string(player) + ":" + priv + "|" + pub + "|" + state.preflop + "/" + state.flop;
}
std::pair<double, double> LeducGame::get_payoffs(LeducState const &state) const
{
    const History &h_pre = state.preflop;
    const History &h_flop = state.flop;

    // Check Folds
    if (!h_pre.empty() && h_pre.back() == FOLD)
    {
        // Determine who folded. The last player to act folded.
        // Current state.player_turn was set to the NEXT player in transition,
        // so the folder is the OPPONENT of the current player token.
        // However, checking turn logic in transition is tricky.
        // Easier: Check length. P1 acts 1st, 3rd... P2 acts 2nd, 4th...
        int p1_moves = 0, p2_moves = 0;
        // Count preflop moves
        for (size_t i = 0; i < h_pre.length(); ++i)
        {
            if (i % 2 == 0)
                p1_moves++;
            else
                p2_moves++;
        }
        // If fold happened preflop:
        int winner = (h_pre.length() % 2 != 0) ? PLAYER_2 : PLAYER_1; // Odd length = P1 folded

        if (winner == PLAYER_1)
            return {state.pot - state.p1_contribution, -state.p2_contribution};
        else
            return {-state.p1_contribution, state.pot - state.p2_contribution};
    }

    if (!h_flop.empty() && h_flop.back() == FOLD)
    {
        // Similar logic for flop
        int p1_moves = 0, p2_moves = 0;
        // P1 starts Flop.
        int winner = (h_flop.length() % 2 != 0) ? PLAYER_2 : PLAYER_1;

        if (winner == PLAYER_1)
            return {state.pot - state.p1_contribution, -state.p2_contribution};
        else
            return {-state.p1_contribution, state.pot - state.p2_contribution};
    }

    // Showdown
    int p1_strength = get_hand_strength(state.p1_card[0], state.public_card[0]);
    int p2_strength = get_hand_strength(state.p2_card[0], state.public_card[0]);

    if (p1_strength > p2_strength)
        return {state.pot - state.p1_contribution, -state.p2_contribution};
    else if (p2_strength > p1_strength)
        return {-state.p1_contribution, state.pot - state.p2_contribution};
    else
        return {0.0, 0.0}; // Split
}

int LeducGame::get_hand_strength(char private_card, char public_card) const
{
    int strength = 0;

    if (std::tolower(private_card) == std::tolower(public_card))
    {
        strength += 3;
    }

    switch (std::tolower(private_card))
    {
    case 'j':
        strength += 0;
        break;
    case 'q':
        strength += 1;
        break;
    case 'k':
        strength += 2;
        break;
    default:
        throw std::runtime_error("Invalid card: " + std::string(1, private_card));
    }

    return strength;
}

std::string LeducGame::action_to_string(Action a) const
{
    switch (a)
    {
    case CALL:
        return "CHECK/CALL (C)";
    case BET:
        return "BET (B)";
    case RAISE:
        return "RAISE (R)";
    case FOLD:
        return "FOLD (F)";
    default:
        return std::string("UNKNOWN (") + a + ")";
    }
}

void LeducGame::print_game_state(State const &state) const
{
    std::cout << "P1: " << state.p1_contribution << " | P2: " << state.p2_contribution << " | Pot: " << state.pot << "\n";
    std::cout << "Preflop: " << state.preflop << " | Flop: " << state.flop << "\n";
    std::cout << "Cards: " << state.p1_card << "/" << state.p2_card << " Public: " << state.public_card << "\n";
}

std::vector<std::pair<LeducState, double>> LeducGame::enumerate_chance_transitions(LeducState const &state) const
{
    // Same wrapper as chance_transition but enumerating
    std::vector<std::pair<LeducState, double>> outcomes;
    std::vector<char> remaining_cards;
    for (char card : CARDS)
    {
        std::string cs(1, card);
        if (state.p1_card != cs && state.p2_card != cs && state.public_card != cs)
            remaining_cards.push_back(card);
    }

    double p = 1.0 / static_cast<double>(remaining_cards.size());

    for (char drawn : remaining_cards)
    {
        LeducState s2 = state;
        if (state.p1_card == NO_CARD)
        {
            s2.p1_card = std::string(1, drawn);
            s2.player_turn = CHANCE_PLAYER;
        }
        else if (state.p2_card == NO_CARD)
        {
            s2.p2_card = std::string(1, drawn);
            s2.player_turn = PLAYER_1;
        }
        else if (state.public_card == NO_CARD)
        {
            s2.public_card = std::string(1, drawn);
            s2.betting_round = FLOP;
            s2.player_turn = PLAYER_1;
        }
        outcomes.emplace_back(s2, p);
    }
    return outcomes;
}