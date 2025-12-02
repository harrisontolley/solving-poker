#pragma once
#include <string>
#include <vector>

using History = std::string;
using Action = char;
using CardsDealt = std::string;

using InfoSet = std::string;
using Strategy = std::vector<float>;

inline char const BET = 'b';
inline char const CALL = 'c';
inline char const FOLD = 'f';

inline std::string H_NO_MOVES_PLAYED = "";
inline std::string H_CALL = "c";
inline std::string H_BET = "b";
inline std::string H_CALL_BET = "cb";
inline std::string H_CALL_CALL = "cc";
inline std::string H_BET_CALL = "bc";
inline std::string H_BET_FOLD = "bf";
inline std::string H_CALL_BET_CALL = "cbc";
inline std::string H_CALL_BET_FOLD = "cbf";
