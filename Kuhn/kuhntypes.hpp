#pragma once
#include "commontypes.hpp"
#include <string>
#include <vector>

inline constexpr bool VERBOSE_DEFAULT = true;
inline constexpr int VERBOSE_UPDATE_PERCENT = 10;
inline constexpr bool CFR_VERBOSE_DEFAULT = true;
inline constexpr bool WRITE_LOG_FILE = true;
inline constexpr char LOG_FILE_NAME[] = "kuhn_cfr_log.csv";
inline constexpr int NUM_LOG_INTERVALS = 10000;

using KuhnAction = char;
using Card = std::string;
inline const Card NO_CARD{" "};

inline constexpr std::string H_NO_MOVES_PLAYED = "";
inline constexpr std::string H_CALL = "C";
inline constexpr std::string H_BET = "B";
inline constexpr std::string H_CALL_BET = "CB";
inline constexpr std::string H_CALL_CALL = "CC";
inline constexpr std::string H_BET_CALL = "BC";
inline constexpr std::string H_BET_FOLD = "BF";
inline constexpr std::string H_CALL_BET_CALL = "CBC";
inline constexpr std::string H_CALL_BET_FOLD = "CBF";

inline constexpr double ANTE = 1.0;