#pragma once
#include <string>
#include <vector>

using PlayerId = int;
inline constexpr PlayerId CHANCE_PLAYER = -1;
inline constexpr PlayerId PLAYER_1 = 0;
inline constexpr PlayerId PLAYER_2 = 1;

using InfoSet = std::string;
using Strategy = std::vector<double>;