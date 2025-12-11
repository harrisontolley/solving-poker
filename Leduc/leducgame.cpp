#include "leducgame.hpp"
#include <stdexcept>
#include <tuple>
#include <string>
#include <vector>
#include <random>
#include <cctype>

namespace
{
    std::mt19937 rng{std::random_device{}()};
}

LeducState LeducGame::get_initial_state() const
{
    return LeducState{};
}

bool LeducGame::is_terminal(LeducState const &state) const
{
    const History &h = (state.betting_round == PREFLOP) ? state.preflop : state.flop;

    if (h == H_R_BET_FOLD || h == H_R_CHECK_BET_FOLD)
        return true;

    if (state.betting_round == FLOP && (h == H_R_CHECK_CHECK || h == H_R_BET_CALL || h == H_R_CHECK_BET_CALL))
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

    History const &h = (state.betting_round == PREFLOP) ? state.preflop : state.flop;

    if (h == H_R_EMPTY || h == H_R_CHECK)
    {
        return {BET, CALL};
    }
    else if (h == H_R_BET || h == H_R_CHECK_BET)
    {
        return {CALL, FOLD};
    }

    // No legal actions in other histories (terminal states)
    return {};
}

LeducState LeducGame::transition(LeducState const &state, LeducAction action) const
{
    LeducState new_state = state;

    // Append to the correct round history
    History &h = (state.betting_round == PREFLOP) ? new_state.preflop : new_state.flop;
    h += action;

    // Update contributions / pot
    if (action == BET)
    {
        int raise_amount = (state.betting_round == PREFLOP) ? PREFLOP_RAISE_AMOUNT : FLOP_RAISE_AMOUNT;

        if (state.player_turn == PLAYER_1)
        {
            new_state.p1_contribution += raise_amount;
            new_state.pot += raise_amount;
        }
        else if (state.player_turn == PLAYER_2)
        {
            new_state.p2_contribution += raise_amount;
            new_state.pot += raise_amount;
        }
    }
    else if (action == CALL)
    {
        double amount_to_call = (state.player_turn == PLAYER_1)
                                    ? state.p2_contribution - state.p1_contribution
                                    : state.p1_contribution - state.p2_contribution;

        if (state.player_turn == PLAYER_1)
        {
            new_state.p1_contribution += amount_to_call;
            new_state.pot += amount_to_call;
        }
        else if (state.player_turn == PLAYER_2)
        {
            new_state.p2_contribution += amount_to_call;
            new_state.pot += amount_to_call;
        }
    }

    // Determine if the round has ended and how
    bool round_complete =
        (h == H_R_CHECK_CHECK) ||
        (h == H_R_BET_CALL) ||
        (h == H_R_CHECK_BET_CALL) ||
        (h == H_R_BET_FOLD) ||
        (h == H_R_CHECK_BET_FOLD);

    bool fold =
        (h == H_R_BET_FOLD) ||
        (h == H_R_CHECK_BET_FOLD);

    if (!round_complete)
    {
        // Round still in progress: alternate player
        new_state.player_turn = (state.player_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;
    }
    else
    {
        if (state.betting_round == PREFLOP && !fold)
        {
            // Preflop finished without a fold -> deal public card next
            new_state.player_turn = CHANCE_PLAYER;
        }
        else
        {
            // Either preflop fold (terminal) or flop complete (terminal)
            // No further actions
            new_state.player_turn = (state.player_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;
        }
    }

    return new_state;
}

std::pair<LeducState, double> LeducGame::chance_transition(LeducState const &state) const
{
    if (state.public_card != NO_CARD &&
        state.p1_card != NO_CARD &&
        state.p2_card != NO_CARD)
    {
        throw std::runtime_error("Chance transition called in non-chance state");
    }

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
        // still chance's turn to deal p2
        new_state.player_turn = CHANCE_PLAYER;
    }
    else if (state.p2_card == NO_CARD)
    {
        new_state.p2_card = std::string(1, drawn);
        // both private cards dealt: start preflop betting with P1
        new_state.player_turn = PLAYER_1;
    }
    else if (state.public_card == NO_CARD)
    {
        new_state.public_card = std::string(1, drawn);
        new_state.betting_round = FLOP;
        // start flop betting with P1
        new_state.player_turn = PLAYER_1;
    }
    else
    {
        throw std::runtime_error("Chance transition called in non-chance state. All cards are already dealt.");
    }

    return {new_state, 1.0 / static_cast<double>(remaining_cards.size())};
}

std::string LeducGame::get_information_set(LeducState const &state, int player) const
{
    if (player != PLAYER_1 && player != PLAYER_2)
        throw std::runtime_error("Invalid player: " + std::to_string(player));

    const Card &priv = (player == PLAYER_1 ? state.p1_card : state.p2_card);
    std::string pub = (state.public_card == NO_CARD) ? "_" : state.public_card;

    return priv + "|" + pub + "|" + state.preflop + "/" + state.flop;
}

std::pair<double, double> LeducGame::get_payoffs(LeducState const &state) const
{
    const History &h = (state.betting_round == PREFLOP) ? state.preflop : state.flop;
    int winner = -1;

    // Showdown on flop
    if (h == H_R_CHECK_CHECK || h == H_R_BET_CALL || h == H_R_CHECK_BET_CALL)
    {
        int p1_strength = get_hand_strength(state.p1_card[0], state.public_card[0]);
        int p2_strength = get_hand_strength(state.p2_card[0], state.public_card[0]);

        if (p1_strength > p2_strength)
            winner = PLAYER_1;
        else if (p2_strength > p1_strength)
            winner = PLAYER_2;
        else
            return {0.0, 0.0}; // split pot
    }
    else if (h == H_R_BET_FOLD)
    {
        // "BF": bettor is P1, folder is P2
        winner = PLAYER_1;
    }
    else if (h == H_R_CHECK_BET_FOLD)
    {
        // "CBF": bettor is P2, folder is P1
        winner = PLAYER_2;
    }
    else
    {
        throw std::runtime_error("Invalid terminal state in get_payoffs: " + h);
    }

    if (winner == PLAYER_1)
    {
        return {state.pot - state.p1_contribution, -state.p2_contribution};
    }
    else if (winner == PLAYER_2)
    {
        return {-state.p1_contribution, state.pot - state.p2_contribution};
    }
    else
    {
        throw std::runtime_error("Winner not determined in get_payoffs");
    }
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
        // In Leduc, 'C' is "check" when no bet, "call" facing a bet.
        return "CHECK/CALL (C)";
    case BET:
        return "BET (B)";
    case FOLD:
        return "FOLD (F)";
    default:
        return std::string("UNKNOWN (") + a + ")";
    }
}

std::vector<std::pair<LeducState, double>> LeducGame::enumerate_chance_transitions(LeducState const &state) const
{
    if (state.public_card != NO_CARD && state.p1_card != NO_CARD && state.p2_card != NO_CARD)
        throw std::runtime_error("enumerate_chance_transitions called in non-chance state.");

    std::vector<std::pair<LeducState, double>> outcomes;

    // build remaining deck
    std::vector<char> remaining_cards;
    for (char card : CARDS)
    {
        std::string cs(1, card);
        if (state.p1_card != cs and state.p2_card != cs && state.public_card != cs)
            remaining_cards.push_back(card);
    }

    if (remaining_cards.empty())
        throw std::runtime_error("No remaining cards in deck");

    double p = 1.0 / static_cast<double>(remaining_cards.size());

    for (char drawn : remaining_cards)
    {
        LeducState s2 = state;

        if (state.p1_card == NO_CARD)
        {
            s2.p1_card = std::string(1, drawn);
            s2.player_turn = CHANCE_PLAYER; // still dealing p2
        }
        else if (state.p2_card == NO_CARD)
        {
            s2.p2_card = std::string(1, drawn);
            s2.player_turn = PLAYER_1; // start preflop betting
        }
        else if (state.public_card == NO_CARD)
        {
            s2.public_card = std::string(1, drawn);
            s2.betting_round = FLOP;
            s2.player_turn = PLAYER_1; // start flop betting
        }
        else
        {
            throw std::runtime_error("enumerate_chance_transitions called in non-chance state. All cards are already dealt.");
        }

        outcomes.emplace_back(s2, p);
    }

    return outcomes;
}
