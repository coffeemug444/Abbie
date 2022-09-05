#include "abbie.hpp"


int main() {

   Abbie bot = Abbie();
   std::cout << "Beginning training" << std::endl;
   bot.trainOneGame();
   std::cout << "Training complete" << std::endl;
   //bot.playAgainst();

}
