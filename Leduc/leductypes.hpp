#pragma once

#include <string>
#include <vector>
#include "commontypes.hpp"

inline constexpr bool VERBOSE_DEFAULT = true;
inline constexpr int VERBOSE_UPDATE_PERCENT = 10;
inline constexpr bool CFR_VERBOSE_DEFAULT = true;
inline constexpr bool WRITE_LOG_FILE = true;
inline constexpr char LOG_FILE_NAME[] = "leduc_cfr_log.csv";
inline constexpr int NUM_LOG_INTERVALS = 10'000;

using LeducAction = char;
using Card = std::string;
inline const Card NO_CARD{" "};

inline constexpr int PREFLOP = 0;
inline constexpr int FLOP = 1;

inline constexpr double ANTE = 1;
inline constexpr double PREFLOP_BET_INCREMENT = 2;
inline constexpr double FLOP_BET_INCREMENT = 4;

// Maximum aggresive accounts per round (bet + raise = 2)
inline constexpr int MAX_AGGRESIVE_ACTIONS = 2;

inline constexpr char BET = 'B';
inline constexpr char CALL = 'C'; // Acts as Check if pot is even, Call if facing bet
inline constexpr char FOLD = 'F';
inline constexpr char RAISE = 'R';

inline const History H_R_CHECK = "C";
inline const History H_R_BET = "B";

inline const History H_R_CHECK_CHECK = "CC";
inline const History H_R_CHECK_BET = "CB";
inline const History H_R_BET_CALL = "BC";
inline const History H_R_BET_FOLD = "BF";

inline const History H_R_CHECK_BET_CALL = "CBC";
inline const History H_R_CHECK_BET_FOLD = "CBF";