#pragma once
#include "commontypes.hpp"
#include <string>
#include <vector>

inline constexpr bool VERBOSE_DEFAULT = true;
inline constexpr int VERBOSE_UPDATE_PERCENT = 10;
inline constexpr bool CFR_VERBOSE_DEFAULT = true;

using KuhnAction = char;

inline constexpr char BET = 'b';
inline constexpr char CALL = 'c';
inline constexpr char FOLD = 'f';

inline constexpr std::string H_NO_MOVES_PLAYED = "";
inline constexpr std::string H_CALL = "c";
inline constexpr std::string H_BET = "b";
inline constexpr std::string H_CALL_BET = "cb";
inline constexpr std::string H_CALL_CALL = "cc";
inline constexpr std::string H_BET_CALL = "bc";
inline constexpr std::string H_BET_FOLD = "bf";
inline constexpr std::string H_CALL_BET_CALL = "cbc";
inline constexpr std::string H_CALL_BET_FOLD = "cbf";

inline constexpr double ANTE = 1.0;