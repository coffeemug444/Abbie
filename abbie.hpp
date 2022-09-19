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
   static Mat modelInputFromState(std::shared_ptr<Piece[]> state);
   static Mat modelOutputFromVal(float val);
   float evaluateFEN(std::string FEN);

   std::vector<float> evaluateStates(std::vector<std::shared_ptr<Piece[]>> states);
   void compute_W_B_forState(
      std::vector<std::string>& FENs,
      unsigned state,
      float starting_eval,
      float ending_eval,
      std::vector<Mat>& weightGrads,
      std::vector<Mat>& biasGrads
   );
   Move getRandomMove(std::string FEN);
   Move getBotMove(Board& board, float &eval, int maxDepth, int initialDepth);

public:
   Abbie();
   Abbie(std::string modelPath);
   void playAgainst();
   Move getBotMove(std::string FEN, float &eval, int maxDepth, int initialDepth);

   void trainOneGame();

   void saveModel(std::string savePath);

};