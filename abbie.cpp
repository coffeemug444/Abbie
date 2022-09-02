#include "abbie.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using std::chrono::system_clock,
      std::this_thread::sleep_for,
      std::cin, std::cout,
      std::string,
      std::shared_ptr, std::make_shared, std::make_unique,
      std::vector;

Abbie::Abbie() {
   model_ = BenBrain({INPUT_SIZE,2048,2048,1}, 'r');
}


float Abbie::EvaluateFEN(string FEN) {
   string FEN_pieces = FEN.substr(0, FEN.find(' '));
   vector<string> rows {};
   for (int i = 0; i < 6; i++) {
      rows.push_back(FEN_pieces.substr(0, FEN_pieces.find('/')));
      FEN_pieces = FEN_pieces.substr(FEN_pieces.find('/') + 1, FEN_pieces.size() - 1);
   }
   rows.push_back(FEN_pieces);
   char squares[NUM_SQUARES];
   unsigned square = 0;
   for (auto row: rows) {
      for (auto chr: row) {
         if (chr >= '1' && chr <= '8') {
            unsigned num_empty = chr - '0';
            for (unsigned i = 0; i < num_empty; i++) {
               squares[square] = '-';
               square++;
            }
         } else {
            squares[square] = chr;
            square++;
         }
      }
   }
   
   shared_ptr<float[]> model_input = make_unique<float[]>(INPUT_SIZE);
   vector<char> pieces = {
      'p', 'P',   // pawn
      'r', 'R',   // rook
      'n', 'N',   // knight
      'b', 'B',   // bishop
      'q', 'Q',   // queen
      'k', 'K'    // king
   };
   unsigned i = 0;
   for(char piece: pieces) {
      for (unsigned sq = 0; sq < NUM_SQUARES; sq++) {
         model_input[i*NUM_SQUARES + sq] = piece == squares[sq];
      }
      i++;
   }
   Mat input = Mat(INPUT_SIZE, 1, model_input);
   Mat output = model_.compute(input);
   return output.getVal(0,0);
}

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
         if (state == ONGOING) {
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

}