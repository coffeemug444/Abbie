#include "abbie.hpp"
#include <sstream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;


bool getTrainingMode() {
   std::string response = "";
   std::cout << "Training mode (y/n): ";
   while (!(response == "y" || response == "n")) {
      if (response != "") {
         std::cout << "Invalid response. Training mode (y/n): ";
      }
      std::cin >> response;
      std::cout << "\033[F\33[2K"; // clear previous line
   }
   if (response == "y") {
      return true;
   } else {
      return false;
   }
}

int main() {
   std::string path = "models";
   std::vector<int> iterations;
   for (const auto & entry : fs::directory_iterator(path)) {
      std::string pathstring = entry.path().string();
      pathstring = pathstring.substr(pathstring.find('_') + 1, pathstring.find('.'));
      iterations.push_back(std::stoi(pathstring));
   }

   Abbie bot;
   unsigned i = 0;
   if (iterations.size() != 0) {
      std::sort(iterations.begin(), iterations.end());
      i = iterations.back();
      std::stringstream ss;
      ss << "models/model_" << i << ".csv";
      Abbie bot(ss.str());
   } else {
      std::stringstream outpath;
      outpath << "models/model_" << i << ".csv";
      bot.saveModel(outpath.str());
   }
   
   bool training_mode = getTrainingMode();

   if ( training_mode ) {
      Board board;
      
      int epoch_length = 100;

      std::cout << "\n\n\n\n";


      while(true) {
         i++;
         std::cout << "Game " << i;
         std::cout << "\033[F\33[2K\033[F\33[2K\033[F\33[2K\033[F\33[2K";
         bot.trainOneGame();
         if (i % epoch_length == 0)  {
            std::stringstream outpath;
            outpath << "models/model_" << i << ".csv";
            bot.saveModel(outpath.str());
         }
      }
   } else {
      bot.playAgainst();
   }

   return 0;
}
