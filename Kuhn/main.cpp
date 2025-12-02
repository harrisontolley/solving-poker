#include "kuhngame.hpp"
#include <iostream>

int main()
{
    KuhnGame game{};
    int player_1_wins = 0;
    int player_2_wins = 0;

    while (!game.is_terminal(game.get_initial_state()))
    {
    }

    return 0;
}