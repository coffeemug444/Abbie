#include "chess/chess.hpp"
#include "benBrain/benBrain.hpp"
#include <iostream>

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
   static float evaluateFEN(std::string FEN, BenBrain* model);
   Move getBotMove(std::string FEN, float& eval);
   static void evaluateFutureMove(std::string FEN, Move move, BenBrain* model, float* output);
   static void getEvals(std::vector<Move> legalMoves, float* evaluations, BenBrain* model, std::string FEN);
   

public:
   Abbie();
   void playAgainst();

   void trainOneGame();
   void trainOneBoard();

};