#pragma once
#include <string>
#include <vector>
#include <unordered_map>

using PlayerId = int;
inline constexpr PlayerId CHANCE_PLAYER = -1;
inline constexpr PlayerId PLAYER_1 = 0;
inline constexpr PlayerId PLAYER_2 = 1;

using InfoSet = std::string;
using Strategy = std::vector<double>;
using StrategyProfile = std::unordered_map<InfoSet, Strategy>;
using History = std::string;
// using Card = std::string;

// inline const Card NO_CARD{" "};

inline const History H_R_EMPTY = "";

inline constexpr char BET = 'B';
inline constexpr char CALL = 'C'; // Acts as Check if pot is even, Call if facing bet
inline constexpr char FOLD = 'F';
inline constexpr char RAISE = 'R';