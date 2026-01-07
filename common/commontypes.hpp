#pragma once
#include <string>
#include <vector>
#include <unordered_map>

using PlayerId = int;
inline constexpr PlayerId CHANCE_PLAYER = -1;
inline constexpr PlayerId PLAYER_1 = 0;
inline constexpr PlayerId PLAYER_2 = 1;

// Abstraction uses strings for buckets to keep the generic map interface
using InfoSet = std::string;
using Strategy = std::vector<double>;
using StrategyProfile = std::unordered_map<InfoSet, Strategy>;
using History = std::string;

inline const History H_R_EMPTY = "";