#pragma once
#include "commontypes.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

class DataWriter
{
public:
    std::string filename_;
    DataWriter(std::string filename) : filename_(filename) {}

    void log_strategy(const StrategyProfile &profile)
    {
        std::ofstream out(filename_);
        out << "InfoSet,ActionIndex,Prob\n";
        for (auto const &[is, strat] : profile)
        {
            for (size_t i = 0; i < strat.size(); ++i)
            {
                out << is << "," << i << "," << strat[i] << "\n";
            }
        }
        std::cout << "Strategy saved to " << filename_ << "\n";
    }

    // Note: Exploitability/NashConv calculation removed.
    // Exact calculation is O(TreeSize) which is impossible for HUTH.
};