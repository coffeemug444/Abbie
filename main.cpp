#include "abbie.hpp"
#include <sstream>
#include <string>

int main() {

   Board board;
   Abbie bot("models/model_15.csv");


   float eval;
   Move move;

   while (true) {
      move = bot.getBotMove(board.genFEN(), eval);
      std::cout << eval << ", ";
      board.doMove(move);
      auto moves = board.getLegalMoves().size();
      std::cout << moves << " legal moves available" << std::endl;
      if (moves == 0) {
         break;
      }

   }

   return 0;

/*
   for (unsigned i = 0; i < 20; i++) {
      if (i % 5 == 0)  {
         std::stringstream outpath;
         outpath << "models/model_" << i << ".csv";
         bot.saveModel(outpath.str());
      }
      bot.trainOneGame();
   }
   bot.saveModel("models/model_20.csv");
   */
}
