#include "abbie.hpp"
#include <sstream>
#include <string>

int main() {

   Board board;
   Abbie bot;
   
   int epoch_length = 5;
   int epochs = 20;
   
   bot.saveModel("models/model_0.csv");

   std::cout << "\n\n\n\nGame 0";

   for (unsigned i = 0; i < epoch_length*epochs; i++) {
      std::cout << "\033[F\33[2K\033[F\33[2K\033[F\33[2K\033[F\33[2K";
      bot.trainOneGame();
      if (i != 0 && i % epoch_length == 0)  {
         std::stringstream outpath;
         outpath << "models/model_" << i << ".csv";
         bot.saveModel(outpath.str());
      }
   }
   
   return 0;
}
