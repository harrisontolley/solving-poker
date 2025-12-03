#pragma once
#include "commontypes.hpp"
#include <string>
#include <vector>

using History = std::string;
using CardsDealt = std::string;

using KuhnAction = char;
using KuhnActionSet = std::vector<KuhnAction>;

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
