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
   BenBrain model_;
   float EvaluateFEN(std::string FEN);


public:
   Abbie();
   void playAgainst();
   GameState playBotMove(Chess& game);

};