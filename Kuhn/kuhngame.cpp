#include "kuhngame.hpp"
#include "commontypes.hpp"
#include "kuhntypes.hpp"
#include <stdexcept>
#include <tuple>
#include <string>
#include <vector>
#include <random>
#include <utility>
#include <iostream>

namespace
{
    std::mt19937 rng{std::random_device{}()};
}

KuhnState KuhnGame::get_initial_state() const
{
    return KuhnState{};
}

bool KuhnGame::is_terminal(KuhnState const &state) const
{
    return state.history == H_CALL_CALL || state.history == H_BET_CALL ||
           state.history == H_BET_FOLD || state.history == H_CALL_BET_CALL ||
           state.history == H_CALL_BET_FOLD;
}

int KuhnGame::get_current_player(KuhnState const &state) const
{
    if (state.p1_card == NO_CARD || state.p2_card == NO_CARD)
        return CHANCE_PLAYER; // chance node

    return (state.history.length() % 2 == 0) ? PLAYER_1 : PLAYER_2;
}

std::vector<KuhnAction> KuhnGame::get_legal_actions(State const &state) const
{
    if (state.history == H_NO_MOVES_PLAYED || state.history == H_CALL)
    {
        return {CALL, BET};
    }
    else if (state.history == H_BET || state.history == H_CALL_BET)
    {
        return {CALL, FOLD};
    }

    return {};
}
KuhnState KuhnGame::transition(KuhnState const &state, Action action) const
{
    KuhnState new_state = state;
    new_state.history += action;

    int player = KuhnGame::get_current_player(state);
    if (action == BET)
    {
        if (player == PLAYER_1)
        {
            new_state.p1_contribution += 1;
            new_state.pot += 1;
        }
        else if (player == PLAYER_2)
        {
            new_state.p2_contribution += 1;
            new_state.pot += 1;
        }
    }
    else if (action == CALL && (state.history == H_BET || state.history == H_CALL_BET))
    {
        if (player == PLAYER_1)
        {
            new_state.p1_contribution += 1;
            new_state.pot += 1;
        }
        else if (player == PLAYER_2)
        {
            new_state.p2_contribution += 1;
            new_state.pot += 1;
        }
    }
    return new_state;
}

std::pair<KuhnState, double> KuhnGame::chance_transition(KuhnState const &state) const
{
    KuhnState new_state = state;

    if (state.p1_card == NO_CARD)
    {
        std::uniform_int_distribution<int> dist(0, 2);
        int idx = dist(rng);

        char dealt = KuhnGame::CARDS[idx];
        new_state.p1_card = dealt;

        return {new_state, (1.0f / 3.0f)};
    }

    else if (state.p2_card == NO_CARD)
    {
        std::vector<char> remaining_cards;

        for (char c : KuhnGame::CARDS)
        {
            std::string card_str(1, c);
            if (card_str != state.p1_card)
                remaining_cards.push_back(c);
        }

        std::uniform_int_distribution<int> dist(0, 1);
        int idx = dist(rng);

        char dealt = remaining_cards[idx];
        new_state.p2_card = dealt;

        return {new_state, (1.0f / 2.0f)};
    }

    throw std::runtime_error("Chance transition called in non-chance state");
}

std::pair<double, double> KuhnGame::get_payoffs(KuhnState const &state) const
{
    int winner;

    if (state.history == H_CALL_CALL || state.history == H_BET_CALL || state.history == H_CALL_BET_CALL)
    {
        int p1_rank = card_rank(state.p1_card.at(0));
        int p2_rank = card_rank(state.p2_card.at(0));
        winner = (p1_rank > p2_rank) ? 1 : 2;
    }
    else if (state.history == H_BET_FOLD)
    {
        winner = 1;
    }
    else if (state.history == H_CALL_BET_FOLD)
    {
        winner = 2;
    }
    else
    {
        throw std::runtime_error("Invalid terminal state: " + state.history);
    }

    if (winner == 1)
    {
        return {state.pot - state.p1_contribution, -state.p2_contribution};
    }
    else
    {
        return {-state.p1_contribution, state.pot - state.p2_contribution};
    }
}

std::string KuhnGame::get_information_set(State const &state, int player) const
{
    if (player != PLAYER_1 && player != PLAYER_2)
        throw std::runtime_error("Invalid player: " + std::to_string(player));

    const Card &priv = (player == PLAYER_1 ? state.p1_card : state.p2_card);
    std::string hist = state.history;

    // Prefix with player id so P1 and P2 infosets never collide
    return std::to_string(player) + ":" + priv + "|" + hist;
}

void KuhnGame::print_game_state(KuhnState const &state) const
{
    std::cout << "Player 1 Contribution: " << state.p1_contribution << "\n";
    std::cout << "Player 2 Contribution: " << state.p2_contribution << "\n";
    std::cout << "Pot: " << state.pot << "\n";
    std::cout << "History: " << state.history << "\n";
    std::cout << "Cards Dealt: " << state.p1_card << ", " << state.p2_card << "\n";
}

inline int KuhnGame::card_rank(char c) const
{
    switch (c)
    {
    case 'J':
        return 0;
    case 'Q':
        return 1;
    case 'K':
        return 2;
    default:
        return -1; // or throw
    }
}

std::string KuhnGame::action_to_string(Action a) const
{
    switch (a)
    {
    case CALL:
        return "CALL (c)";
    case BET:
        return "BET  (b)";
    case FOLD:
        return "FOLD (f)";
    default:
        return std::string("UNKNOWN (") + a + ")";
    }
}

std::vector<std::pair<KuhnState, double>> KuhnGame::enumerate_chance_transitions(KuhnState const &state) const
{
    std::vector<std::pair<KuhnState, double>> outcomes;

    if (state.p1_card == NO_CARD)
    {
        double p = 1.0 / CARDS.size();

        for (char c : CARDS)
        {
            KuhnState s2 = state;
            s2.p1_card = c;
            outcomes.emplace_back(s2, p);
        }
    }
    else if (state.p2_card == NO_CARD)
    {
        std::vector<char> remaining;

        for (char c : CARDS)
        {
            std::string card_str(1, c);
            if (card_str != state.p1_card)
                remaining.push_back(c);
        }

        double p = 1.0 / remaining.size();

        for (char c : remaining)
        {
            KuhnState s2 = state;
            s2.p2_card = c;
            outcomes.emplace_back(s2, p);
        }
    }
    else
    {
        throw std::runtime_error("enumerate_chance_transitions called in non-chance state");
    }

    return outcomes;
}