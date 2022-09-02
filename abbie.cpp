#include "abbie.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using std::chrono::system_clock,
      std::this_thread::sleep_for,
      std::cin, std::cout,
      std::string;


GameState Abbie::playBotMove(Chess& game) {
   return game.doRandomMove();
}

void Abbie::playAgainst() {
   Chess game;
   game.printBoard();
   GameState state = ONGOING;

   cout << "\n\033[A";
   cout << "Enter move: ";

   while (state == ONGOING || state == BADMOVE) {
      string move;
      cin >> move;
      if (move == "q" || move == "exit") {
         exit(1);
      }
      state = game.acceptMove(move);

      if (state == BADMOVE) {
         cout << "\33[2k\033[A\r";
         cout << "                                                        \r";
         cout << "Move invalid. Enter move: ";
      } else {
         for (int i = 0; i < 10; i++) {
            // clear line and move up
            cout << "\33[2k\033[A\r";
         }
         game.printBoard();
         sleep_for(400ms);
         state = playBotMove(game);
         for (int i = 0; i < 9; i++) {
            // clear line and move up
            cout << "\33[2k\033[A\r";
         }
         game.printBoard();
         cout << "\r                                                        \r";
         cout << "Enter move: ";
      }
   }

}