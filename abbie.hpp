#include "chess/chess.hpp"
#include "benBrain/benBrain.hpp"
#include <iostream>
#include <mutex>

const unsigned NUM_PIECES = 12;
const unsigned NUM_ROWS = 8;
const unsigned NUM_COLS = 8;
const unsigned NUM_SQUARES = NUM_ROWS * NUM_COLS;
const unsigned INPUT_SIZE = NUM_PIECES * NUM_SQUARES;


class Abbie {
private:
   std::random_device dev_;
   std::mt19937 rng_;
   BenBrain model_;
   static Mat modelInputFromFEN(std::string FEN);
   static Mat modelOutputFromVal(float val);
   static float minimax(BenBrain *model, int maxDepth, int depthFromStart, std::string FEN, int currentDepth, float alpha, float beta, bool white);
   static float evaluateFEN(std::string FEN, BenBrain* model);
   void compute_W_B_forState(
      std::vector<std::string>& FENs,
      unsigned state,
      float starting_eval,
      float ending_eval,
      std::vector<Mat>& weightGrads,
      std::vector<Mat>& biasGrads
   );

   

public:
   Abbie();
   Abbie(std::string modelPath);
   void playAgainst();
   Move getBotMove(std::string FEN, float& eval, int maxDepth);

   void trainOneGame();
   void trainOneBoard();

   void saveModel(std::string savePath);

};