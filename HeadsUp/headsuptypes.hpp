#pragma once

#include <string>
#include <cstdint>
#include "commontypes.hpp"

inline constexpr bool VERBOSE_DEFAULT = true;
inline constexpr int VERBOSE_UPDATE_PERCENT = 10;
inline constexpr bool CFR_VERBOSE_DEFAULT = true;
inline constexpr bool WRITE_LOG_FILE = true;
inline constexpr char LOG_FILE_NAME[] = "headsup_cfr_log.csv";
inline constexpr int NUM_LOG_INTERVALS = 10'000;

struct HeadsUpAction
{
    // 'C', 'R', 'F', 'B'
    char actionType;

    // amount is only valid when actionType is a raise/bet (no default amounts)
    double amount{0.0};

    // need operator overloading for std::find in CFR TU
    bool operator==(const HeadsUpAction &other) const
    {
        return actionType == other.actionType && std::abs(amount - other.amount) < 0.001;
    }
};

using Card = int_fast8_t;
int_fast8_t NO_CARD = -1;

// Betting rounds
inline constexpr int PREFLOP = 0;
inline constexpr int FLOP = 1;
inline constexpr int TURN = 2;
inline constexpr int RIVER = 3;

inline const double SMALL_BLIND = 100;
inline const double BIG_BLIND = 200;

// 1/3rd, 2/3rds, 1.5x pot size possible bet sizes
inline const std::vector<double> POT_PROPORTION_BET_SIZE{0.3333, 0.6667, 1.5};
