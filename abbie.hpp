#include "chess/chess.hpp"
#include "benBrain/benBrain.hpp"
#include <iostream>
#include <mutex>

const unsigned NUM_PIECES = 6;
const unsigned NUM_COLORS = 2;
const unsigned NUM_ROWS = 8;
const unsigned NUM_COLS = 8;
const unsigned NUM_SQUARES = NUM_ROWS * NUM_COLS;
const unsigned OTHER_INPUTS = 5;
const unsigned INPUT_SIZE = NUM_PIECES * NUM_COLORS * NUM_SQUARES + OTHER_INPUTS;



// P1 piece          6x64
// P2 piece          6x64

// colour            1
// P1 castling       2
// P2 castling       2



class Abbie {
private:
   std::random_device m_dev;
   std::mt19937 m_rng;
   BenBrain m_model;
   static Mat modelInputFromFEN(std::string FEN);
   static Mat modelInputFromBoard(Board& board);
   static Mat modelOutputFromVal(float val) { return Mat::val(1,1,val); }
   float evaluateFEN(std::string FEN) const;

   std::vector<float> evaluateInputs(std::vector<Mat> inputs) { return m_model.multipleCompute(inputs).getVals(); }

   float hindsightEvalAtState (
   float S,
   float E,
   unsigned N,
   unsigned current);

   void compute_W_B_fromInputs (
      std::vector<Mat> inputs,
      float starting_eval,
      float ending_eval,
      std::vector<Mat>& weight_grads,
      std::vector<Mat>& bias_grads
   );

   void compute_W_B_fromInput(
      Mat& input,
      float current_eval,
      std::vector<Mat>& weight_grads,
      std::vector<Mat>& bias_grads
   );
   
   Move getRandomMove(Board& board);
   Move getBotMove(Board& board, float &eval);

public:
   Abbie();
   Abbie(std::string modelPath);
   void playAgainst();
   Move getBotMove(std::string FEN, float &eval);

   void trainOneGame();

   void saveModel(std::string save_path) { m_model.save(save_path); }

};