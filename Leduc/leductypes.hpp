#pragma once

#include <string>
#include <vector>
#include "commontypes.hpp"

inline constexpr bool VERBOSE_DEFAULT = false;
inline constexpr bool CFR_VERBOSE_DEFAULT = false;

using LeducAction = char;

inline constexpr int PREFLOP = 0;
inline constexpr int FLOP = 1;

inline constexpr double ANTE = 1;
inline constexpr double PREFLOP_RAISE_AMOUNT = 2;
inline constexpr double FLOP_RAISE_AMOUNT = 4;

inline constexpr char BET = 'B';
inline constexpr char CALL = 'C';
inline constexpr char FOLD = 'F';

inline const History H_R_CHECK = "C";
inline const History H_R_BET = "B";

inline const History H_R_CHECK_CHECK = "CC";
inline const History H_R_CHECK_BET = "CB";
inline const History H_R_BET_CALL = "BC";
inline const History H_R_BET_FOLD = "BF";

inline const History H_R_CHECK_BET_CALL = "CBC";
inline const History H_R_CHECK_BET_FOLD = "CBF";
