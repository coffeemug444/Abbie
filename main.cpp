#include "abbie.hpp"
#include <sstream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

int main() {

   std::string path = "models";
   std::vector<int> iterations;
   for (const auto & entry : fs::directory_iterator(path)) {
      std::string pathstring = entry.path().string();
      pathstring = pathstring.substr(pathstring.find('_') + 1, pathstring.find('.'));
      iterations.push_back(std::stoi(pathstring));
   }
   std::sort(iterations.begin(), iterations.end());

   unsigned i = iterations.back();
   std::stringstream ss;
   ss << "models/model_" << i << ".csv";

   Abbie bot(ss.str());

   //bot.playAgainst();

   Board board;
   
   int epoch_length = 5;

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
   
   return 0;
}
