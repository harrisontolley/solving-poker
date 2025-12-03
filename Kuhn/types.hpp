#pragma once
#include <string>
#include <vector>

using History = std::string;
using Action = char;
using CardsDealt = std::string;

using InfoSet = std::string;
using Strategy = std::vector<float>;

inline const char BET = 'b';
inline const char CALL = 'c';
inline const char FOLD = 'f';

inline const std::string H_NO_MOVES_PLAYED = "";
inline const std::string H_CALL = "c";
inline const std::string H_BET = "b";
inline const std::string H_CALL_BET = "cb";
inline const std::string H_CALL_CALL = "cc";
inline const std::string H_BET_CALL = "bc";
inline const std::string H_BET_FOLD = "bf";
inline const std::string H_CALL_BET_CALL = "cbc";
inline const std::string H_CALL_BET_FOLD = "cbf";

inline const int CHANCE_PLAYER = 0;
inline const int PLAYER_1 = 1;
inline const int PLAYER_2 = 2;