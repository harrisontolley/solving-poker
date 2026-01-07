#include "headsupgame.hpp"
#include "SevenEval.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <random>
#include <numeric>
#include <iomanip>
#include <array>
#include <cmath>

namespace
{
    inline constexpr double SEVENEVAL_MAX_RANK = 7462.0;

    inline int as_int(Card c)
    {
        return static_cast<int>(c);
    }

    inline void mark_used(std::array<bool, 52> &used, Card c)
    {
        int x = as_int(c);
        if (x >= 0 && x < 52)
            used[static_cast<std::size_t>(x)] = true;
    }
}

HeadsUpGame::HeadsUpGame() : rng(std::random_device{}()) {}

HeadsUpState HeadsUpGame::get_initial_state()
{
    HeadsUpState s;
    s.player_turn = CHANCE_PLAYER;

    // Initialize full deck
    s.deck.resize(52);
    std::iota(s.deck.begin(), s.deck.end(), 0);

    return s;
}

bool HeadsUpGame::is_terminal(HeadsUpState const &state) const
{
    return state.game_over;
}

int HeadsUpGame::get_current_player(HeadsUpState const &state) const
{
    return state.player_turn;
}

std::vector<HeadsUpAction> HeadsUpGame::get_legal_actions(HeadsUpState const &state) const
{
    if (state.player_turn == CHANCE_PLAYER || state.game_over)
        return {};

    std::vector<HeadsUpAction> actions;

    double p1_cont = state.p1_contribution;
    double p2_cont = state.p2_contribution;

    double my_cont = (state.player_turn == PLAYER_1) ? p1_cont : p2_cont;
    double opp_cont = (state.player_turn == PLAYER_1) ? p2_cont : p1_cont;
    double deficit = opp_cont - my_cont;

    double my_remaining_stack = STACK_SIZE - my_cont;

    // Fold
    if (deficit > 1e-9)
        actions.push_back({'F', 0.0});

    // Call / Check
    actions.push_back({'C', 0.0});

    // Bet / Raise (cap aggression to keep tree small)
    int aggression_count = 0;
    for (char c : state.current_round_history)
    {
        if (c == 'B' || c == 'R')
            aggression_count++;
    }

    if (aggression_count < 2 && my_remaining_stack > deficit + 1.0)
    {
        double current_pot = state.pot;
        double pot_base = current_pot + deficit;

        for (double pct : POT_PROPORTION_BET_SIZE)
        {
            double amt = pot_base * pct;
            if (my_remaining_stack >= deficit + amt)
            {
                char type = (deficit > 1e-9) ? 'R' : 'B';
                actions.push_back({type, std::floor(amt)});
            }
        }
    }

    return actions;
}

HeadsUpState HeadsUpGame::transition(HeadsUpState const &state, HeadsUpAction action) const
{
    HeadsUpState next = state;
    next.current_round_history += action.actionType;

    double &actor_contrib = (state.player_turn == PLAYER_1) ? next.p1_contribution : next.p2_contribution;
    double &opp_contrib = (state.player_turn == PLAYER_1) ? next.p2_contribution : next.p1_contribution;
    double deficit = opp_contrib - actor_contrib;

    if (action.actionType == 'F')
    {
        next.game_over = true;
        return next;
    }

    if (action.actionType == 'C')
    {
        double call_amt = deficit;
        double stack = STACK_SIZE - actor_contrib;
        if (call_amt > stack)
            call_amt = stack;

        actor_contrib += call_amt;
        next.pot += call_amt;
    }
    else if (action.actionType == 'B' || action.actionType == 'R')
    {
        double add = deficit + action.amount;
        actor_contrib += add;
        next.pot += add;
    }

    // round end checks
    bool round_end = false;
    size_t len = next.current_round_history.size();

    if (next.current_round_history == "CC")
        round_end = true;

    if (len >= 2)
    {
        if (next.current_round_history.back() == 'C')
        {
            char prev = next.current_round_history[len - 2];
            if (prev == 'B' || prev == 'R')
                round_end = true;
        }
    }

    if (round_end)
    {
        if (next.betting_round == RIVER)
        {
            next.game_over = true;
        }
        else
        {
            next.betting_round++;
            next.player_turn = CHANCE_PLAYER;
            next.current_round_history = "";
        }
    }
    else
    {
        next.player_turn = (state.player_turn == PLAYER_1) ? PLAYER_2 : PLAYER_1;
    }

    return next;
}

HeadsUpState HeadsUpGame::sample_chance(HeadsUpState const &state)
{
    HeadsUpState next = state;

    if (next.deck.empty())
        throw std::runtime_error("Empty deck");

    auto draw_one = [&]() -> Card
    {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(next.deck.size()) - 1);
        int idx = dist(rng);
        Card c = next.deck[idx];
        next.deck[idx] = next.deck.back();
        next.deck.pop_back();
        return c;
    };

    // deal hole cards (P1 then P2, two each)
    if (next.p1_card_1 == NO_CARD)
    {
        next.p1_card_1 = draw_one();
        next.player_turn = CHANCE_PLAYER;
        return next;
    }
    if (next.p1_card_2 == NO_CARD)
    {
        next.p1_card_2 = draw_one();
        next.player_turn = CHANCE_PLAYER;
        return next;
    }
    if (next.p2_card_1 == NO_CARD)
    {
        next.p2_card_1 = draw_one();
        next.player_turn = CHANCE_PLAYER;
        return next;
    }
    if (next.p2_card_2 == NO_CARD)
    {
        next.p2_card_2 = draw_one();
        next.player_turn = PLAYER_1; // SB acts first preflop
        return next;
    }

    // Deal community cards as needed for the current street
    if (next.betting_round == FLOP)
    {
        if (next.board_cards.size() < 3)
        {
            next.board_cards.push_back(draw_one());
            next.player_turn = (next.board_cards.size() < 3) ? CHANCE_PLAYER : PLAYER_2; // BB first postflop
            return next;
        }
        next.player_turn = PLAYER_2;
        return next;
    }

    if (next.betting_round == TURN)
    {
        if (next.board_cards.size() < 4)
            next.board_cards.push_back(draw_one());
        next.player_turn = PLAYER_2;
        return next;
    }

    if (next.betting_round == RIVER)
    {
        if (next.board_cards.size() < 5)
            next.board_cards.push_back(draw_one());
        next.player_turn = PLAYER_2;
        return next;
    }

    // there's nothing to deal.
    throw std::runtime_error("sample_chance called but nothing left to deal for this state");
}

std::pair<double, double> HeadsUpGame::get_payoffs(HeadsUpState const &state) const
{
    // Fold
    if (state.game_over && !state.current_round_history.empty() && state.current_round_history.back() == 'F')
    {
        // player_turn is the folder
        if (state.player_turn == PLAYER_2) // P2 folded
            return {state.p2_contribution, -state.p2_contribution};
        else
            return {-state.p1_contribution, state.p1_contribution};
    }

    // Showdown requires full board
    if (state.board_cards.size() != 5)
        throw std::runtime_error("Showdown reached without 5 board cards");

    int r1 = get_rank_idx(state.p1_card_1, state.p1_card_2, state.board_cards);
    int r2 = get_rank_idx(state.p2_card_1, state.p2_card_2, state.board_cards);

    if (r1 > r2)
        return {state.pot - state.p1_contribution, -state.p2_contribution};
    else if (r2 > r1)
        return {-state.p1_contribution, state.pot - state.p2_contribution};

    return {0.0, 0.0};
}

InfoSet HeadsUpGame::get_information_set(HeadsUpState const &state, int player) const
{
    // Public betting state (bucketed) to prevent merging states with different legal action sets.
    double my_cont = (player == PLAYER_1) ? state.p1_contribution : state.p2_contribution;
    double opp_cont = (player == PLAYER_1) ? state.p2_contribution : state.p1_contribution;
    double to_call = std::max(0.0, opp_cont - my_cont);
    double eff_stack = STACK_SIZE - std::max(state.p1_contribution, state.p2_contribution);

    auto bb_bucket = [](double chips) -> int
    {
        return static_cast<int>(std::llround(chips / BIG_BLIND));
    };

    std::stringstream ss;
    ss << state.betting_round << "|";
    ss << get_abstraction_key(state, player) << "|";
    ss << state.current_round_history << "|";
    ss << "potBB=" << bb_bucket(state.pot) << "|";
    ss << "tcBB=" << bb_bucket(to_call) << "|";
    ss << "esBB=" << bb_bucket(eff_stack);
    return ss.str();
}

std::string HeadsUpGame::get_abstraction_key(HeadsUpState const &state, int player) const
{
    Card c1 = (player == PLAYER_1) ? state.p1_card_1 : state.p2_card_1;
    Card c2 = (player == PLAYER_1) ? state.p1_card_2 : state.p2_card_2;

    if (state.betting_round == PREFLOP)
    {
        int r1 = c1 / 4;
        int s1 = c1 % 4;
        int r2 = c2 / 4;
        int s2 = c2 % 4;

        if (r1 < r2)
        {
            std::swap(r1, r2);
            std::swap(s1, s2);
        }

        bool suited = (s1 == s2);
        std::string s = suited ? "s" : "o";
        if (r1 == r2)
            s = "";

        const std::string ranks = "23456789TJQKA";
        return std::string(1, ranks[r1]) + std::string(1, ranks[r2]) + s;
    }
    else
    {
        // Postflop: deterministic strength bucket (cached)
        if (state.board_cards.size() < 3)
            return "0";

        double hs = calculate_hand_strength(c1, c2, state.board_cards);
        int bucket = static_cast<int>(hs * POSTFLOP_BUCKETS);
        if (bucket >= POSTFLOP_BUCKETS)
            bucket = POSTFLOP_BUCKETS - 1;
        if (bucket < 0)
            bucket = 0;
        return std::to_string(bucket);
    }
}

std::uint64_t HeadsUpGame::strength_cache_key(Card c1, Card c2, const std::vector<Card> &board) const
{
    // order invariant for hole cards + board cards
    int a = std::min(as_int(c1), as_int(c2));
    int b = std::max(as_int(c1), as_int(c2));

    std::array<int, 5> bc{-1, -1, -1, -1, -1};

    for (std::size_t i = 0; i < board.size() && i < 5; ++i)
        bc[i] = as_int(board[i]);

    // sort hand order
    std::sort(bc.begin(), bc.begin() + static_cast<long>(std::min<std::size_t>(board.size(), 5)));

    // simple 64 bit FNV-1a style hash
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&](std::uint64_t x)
    {
        h ^= x;
        h *= 1099511628211ULL;
    };

    mix(static_cast<std::uint64_t>(a + 2));
    mix(static_cast<std::uint64_t>(b + 2));
    mix(static_cast<std::uint64_t>(board.size()));

    for (std::size_t i = 0; i < board.size() && i < 5; ++i)
        mix(static_cast<std::uint64_t>(bc[i] + 2));

    return h;
}

double HeadsUpGame::calculate_hand_strength(Card c1, Card c2, const std::vector<Card> &board) const
{
    if (board.size() < 3)
        return 0.0;

    std::uint64_t key = strength_cache_key(c1, c2, board);
    auto it = strength_cache_.find(key);
    if (it != strength_cache_.end())
        return it->second;

    // Build remaining deck deterministically from known cards
    std::array<bool, 52> used{};
    used.fill(false);
    mark_used(used, c1);
    mark_used(used, c2);
    for (Card bc : board)
        mark_used(used, bc);

    std::vector<int> remaining;
    remaining.reserve(52);
    for (int i = 0; i < 52; ++i)
        if (!used[static_cast<std::size_t>(i)])
            remaining.push_back(i);

    long double sum_rank = 0.0L;
    long double count = 0.0L;

    if (board.size() == 5)
    {
        int r = SevenEval::GetRank(as_int(c1),
                                   as_int(c2),
                                   as_int(board[0]),
                                   as_int(board[1]),
                                   as_int(board[2]),
                                   as_int(board[3]),
                                   as_int(board[4]));
        double hs = static_cast<double>(static_cast<long double>(r) / SEVENEVAL_MAX_RANK);
        strength_cache_[key] = hs;
        return hs;
    }

    if (board.size() == 4)
    {
        // enumerate all possible rivers
        for (int rcard : remaining)
        {
            int r = SevenEval::GetRank(as_int(c1),
                                       as_int(c2),
                                       as_int(board[0]),
                                       as_int(board[1]),
                                       as_int(board[2]),
                                       as_int(board[3]),
                                       rcard);
            sum_rank += static_cast<long double>(r);
            count += 1.0L;
        }
    }
    else // board.size() == 3
    {
        // enumerate all possible turn+river pairs
        for (std::size_t i = 0; i < remaining.size(); ++i)
        {
            for (std::size_t j = i + 1; j < remaining.size(); ++j)
            {
                int t = remaining[i];
                int rcard = remaining[j];
                int r = SevenEval::GetRank(as_int(c1),
                                           as_int(c2),
                                           as_int(board[0]),
                                           as_int(board[1]),
                                           as_int(board[2]),
                                           t,
                                           rcard);
                sum_rank += static_cast<long double>(r);
                count += 1.0L;
            }
        }
    }

    double hs = 0.0;
    if (count > 0.0L)
        hs = static_cast<double>((sum_rank / count) / SEVENEVAL_MAX_RANK);

    strength_cache_[key] = hs;
    return hs;
}

int HeadsUpGame::get_rank_idx(Card c1, Card c2, const std::vector<Card> &board) const
{
    if (board.size() != 5)
        throw std::runtime_error("SevenEval::GetRank requires exactly 7 cards (2 hole + 5 board)");

    return SevenEval::GetRank(as_int(c1),
                              as_int(c2),
                              as_int(board[0]),
                              as_int(board[1]),
                              as_int(board[2]),
                              as_int(board[3]),
                              as_int(board[4]));
}

std::string HeadsUpGame::action_to_string(HeadsUpAction a) const
{
    std::stringstream ss;

    ss << a.actionType;
    if (a.actionType == 'B' || a.actionType == 'R')
        ss << a.amount;

    return ss.str();
}

std::string HeadsUpGame::card_to_string(Card c) const
{
    if (c == NO_CARD)
        return "??";
    const std::string ranks = "23456789TJQKA";
    const std::string suits = "shdc";
    return std::string{ranks[c / 4]} + suits[c % 4];
}
