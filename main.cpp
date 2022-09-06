#include "abbie.hpp"
#include <sstream>
#include <string>

int main() {

   Abbie bot;

   //bot.playAgainst();

   Board board;

   
   int epoch_length = 5;
   int epochs = 20;
   
   bot.saveModel("models/model_0.csv");

   std::cout << "\n\n\n\n";

   unsigned i = 1;

   while(true) {
      std::cout << "Game " << i;
      std::cout << "\033[F\33[2K\033[F\33[2K\033[F\33[2K\033[F\33[2K";
      bot.trainOneGame();
      if (i % epoch_length == 0)  {
         std::stringstream outpath;
         outpath << "models/model_" << i << ".csv";
         bot.saveModel(outpath.str());
      }
      i++;
   }
   
   
   return 0;
}
