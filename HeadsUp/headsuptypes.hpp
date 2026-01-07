#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <cmath>
#include "commontypes.hpp"

inline constexpr bool VERBOSE_DEFAULT = false;
inline constexpr int VERBOSE_UPDATE_PERCENT = 5;
inline constexpr bool WRITE_LOG_FILE = true;
inline constexpr char LOG_FILE_NAME[] = "headsup_mccfr_log.csv";
inline constexpr int NUM_LOG_INTERVALS = 100;

struct HeadsUpAction
{
    char actionType; // 'C', 'R', 'F', 'B'

    // amount is only valid when actionType is a raise/bet (no default amounts)
    double amount{0.0};

    // need operator overloading for std::find in CFR TU
    bool operator==(const HeadsUpAction &other) const
    {
        return actionType == other.actionType && std::abs(amount - other.amount) < 0.001;
    }
};

using Card = int_fast8_t;
inline constexpr Card NO_CARD = -1;

// Betting rounds
inline constexpr int PREFLOP = 0;
inline constexpr int FLOP = 1;
inline constexpr int TURN = 2;
inline constexpr int RIVER = 3;

inline constexpr double SMALL_BLIND = 100.0;
inline constexpr double BIG_BLIND = 200.0;
inline constexpr double STACK_SIZE = 20000.0; // 100BB deep

// 1/3rd, 2/3rds, 1.5x pot size possible bet sizes
inline const std::vector<double> POT_PROPORTION_BET_SIZE{0.3333, 0.6667, 1.5};

// Abstraction Settings
inline constexpr int POSTFLOP_BUCKETS = 20; // Number of strength buckets