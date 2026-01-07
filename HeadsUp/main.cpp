#include "headsupgame.hpp"
#include "mccfr.hpp"
#include <iostream>

int main()
{
    std::cout << "Initializing Heads Up No Limit Hold'em Solver..." << std::endl;
    std::cout << "Using Algorithm: MCCFR" << std::endl;

    HeadsUpGame game;
    MCCFR<HeadsUpGame> solver(game);

    int iterations = 10000;

    std::cout << "Starting training for " << iterations << " iterations..." << std::endl;
    solver.train(iterations);

    std::cout << "Training complete." << std::endl;
    return 0;
}