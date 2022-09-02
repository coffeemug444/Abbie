#include "abbie.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using std::chrono::system_clock,
      std::this_thread::sleep_for,
      std::cin, std::cout,
      std::string,
      std::shared_ptr, std::make_shared, std::make_unique,
      std::vector;

Abbie::Abbie() {
   rng_ = std::mt19937(dev_());
   model_ = BenBrain({INPUT_SIZE,2048,2048,1}, 'r');
}


float Abbie::evaluateFEN(string FEN) {
   Mat input = modelInputFromFEN(FEN);
   Mat output = model_.compute(input);
   return output.getVal(0,0);
}

Mat Abbie::modelInputFromFEN(string FEN) {
   string FEN_pieces = FEN.substr(0, FEN.find(' '));
   vector<string> rows {};
   for (int i = 0; i < 6; i++) {
      rows.push_back(FEN_pieces.substr(0, FEN_pieces.find('/')));
      FEN_pieces = FEN_pieces.substr(FEN_pieces.find('/') + 1, FEN_pieces.size() - 1);
   }
   rows.push_back(FEN_pieces);
   char squares[NUM_SQUARES];
   unsigned square = 0;
   for (auto row: rows) {
      for (auto chr: row) {
         if (chr >= '1' && chr <= '8') {
            unsigned num_empty = chr - '0';
            for (unsigned i = 0; i < num_empty; i++) {
               squares[square] = '-';
               square++;
            }
         } else {
            squares[square] = chr;
            square++;
         }
      }
   }
   
   shared_ptr<float[]> model_input = make_unique<float[]>(INPUT_SIZE);
   vector<char> pieces = {
      'p', 'P',   // pawn
      'r', 'R',   // rook
      'n', 'N',   // knight
      'b', 'B',   // bishop
      'q', 'Q',   // queen
      'k', 'K'    // king
   };
   unsigned i = 0;
   for(char piece: pieces) {
      for (unsigned sq = 0; sq < NUM_SQUARES; sq++) {
         model_input[i*NUM_SQUARES + sq] = piece == squares[sq];
      }
      i++;
   }
   return Mat(INPUT_SIZE, 1, model_input);
}

float std_dev(vector<float> input) {
   float sum = std::accumulate(std::begin(input), std::end(input), 0.0);
   float m =  sum / input.size();

   float accum = 0.0;
   std::for_each (std::begin(input), std::end(input), [&](const float d) {
      accum += (d - m) * (d - m);
   });

   return sqrt(accum / (input.size()-1)) + 0.000001; // add small constant to std_dev in case all values are exactly the same
}

Move Abbie::getBotMove(string FEN, float& eval) {
   vector<float> evaluations {};
   Board currentBoard = Board(FEN);
   vector<Move> legalMoves = currentBoard.getLegalMoves();

   char player = FEN[FEN.find(' ') + 1];
   bool playingAsWhite = player == 'w';

   for (auto move: legalMoves) {
      Board futureBoard(FEN);
      futureBoard.doMove(move);
      string futureFEN = futureBoard.genFEN();
      float futureEval = evaluateFEN(futureFEN);
      evaluations.push_back(futureEval);
   }

   float best_evaluation = evaluations[0];

   for (unsigned i = 1; i < evaluations.size(); i++) {
      if (playingAsWhite) {
         if (evaluations[i] > best_evaluation) {
            best_evaluation = evaluations[i];
         }
      } else {
         if (evaluations[i] < best_evaluation) {
            best_evaluation = evaluations[i];
         }
      }
   }

   float eval_std_dev = std_dev(evaluations);
   vector<Move> bestMoves = {};
   vector<float> bestEvals = {};
   for (unsigned i = 1; i < evaluations.size(); i++) {
      if (playingAsWhite) {
         if ((evaluations[i] + eval_std_dev) > best_evaluation) {
            bestMoves.push_back(legalMoves[i]);
            bestEvals.push_back(evaluations[i]);
         }
      } else {
         if ((evaluations[i] - eval_std_dev) < best_evaluation) {
            bestMoves.push_back(legalMoves[i]);
            bestEvals.push_back(evaluations[i]);
         }
      }
   }

   std::uniform_int_distribution<std::mt19937::result_type> dist(0,bestMoves.size() - 1);
   int index = dist(rng_);
   eval = bestEvals[index];
   return bestMoves[index];
}

void Abbie::playAgainst() {
   Chess game;
   game.printBoard();
   GameState state = ONGOING;

   cout << "\n\033[A";
   cout << "Enter move: ";

   while (state == ONGOING || state == BADMOVE) {
      string move;
      cin >> move;
      if (move == "q" || move == "exit") {
         exit(1);
      }
      state = game.acceptMove(move);

      if (state == BADMOVE) {
         cout << "\33[2k\033[A\r";
         cout << "                                                        \r";
         cout << "Move invalid. Enter move: ";
      } else {
         for (int i = 0; i < 10; i++) {
            // clear line and move up
            cout << "\33[2k\033[A\r";
         }
         game.printBoard();
         if (state == ONGOING) {
            string FEN = game.getFEN();
            float eval = 0.f;
            Move botMove = getBotMove(FEN, eval);
            state = game.acceptMove(botMove);
            for (int i = 0; i < 9; i++) {
               // clear line and move up
               cout << "\33[2k\033[A\r";
            }
            game.printBoard();
            cout << "\r                                                        \r";
            cout << "Enter move: ";
         }
      }
   }
}

Mat Abbie::modelOutputFromVal(float val) {
   shared_ptr<float[]> output = make_unique<float[]>(1);
   output[0] = val;
   return Mat(1,1,output);
}

void Abbie::trainOneGame() {
   Chess game;
   GameState gameState = ONGOING;
   vector<string> FENs {};

   std::cout << "turn 0\n";

   bool whitePlaying = true;
   int turnCount= 0;
   while (gameState == ONGOING) {
      turnCount++;
      std::cout << "turn " << turnCount << "\n";
      string FEN = game.getFEN();
      FENs.push_back(FEN);
      float eval;
      Move move = getBotMove(FEN, eval);
      gameState = game.acceptMove(move);
      whitePlaying = !whitePlaying;
   }
   FENs.push_back(game.getFEN());

   float starting_eval = 0.5f;
   float ending_eval;
   if (gameState == CHECKMATE) {
      ending_eval = (whitePlaying ? 1.f : 0.f);
   } else {
      // draw
      ending_eval = 0.5f;
   }



   vector<Mat> weightGrads = model_.weightsZero();
   vector<Mat> biasGrads = model_.biasesZero();

   std::cout << "Game was " << FENs.size() << " moves long\n";

   for (int state = 0; state < FENs.size(); state++) {
      std::cout << "Computing grads from board " << state << "\n";
      float eval_shouldBe = ending_eval - (state - (FENs.size()-1))*(starting_eval/(FENs.size()));
      Mat input = modelInputFromFEN(FENs[state]);
      Mat output = modelOutputFromVal(eval_shouldBe);
      auto diffs = model_.backPropagate(input, output);

      auto nw = model_.computeWeights(diffs);
      for (int w = 0; w < weightGrads.size(); w++) {
         auto newWeightGrad = weightGrads[w] + nw[w];
         weightGrads[w] = newWeightGrad;
      }

      auto nb = model_.computeBiases(diffs);
      for (int b = 0; b < biasGrads.size(); b++) {
         auto newBiasGrad = biasGrads[b] + nb[b];
         biasGrads[b] = newBiasGrad;
      }
   }

   std::cout << "Adjusting weights and biases\n";
   model_.adjustWeightsAndBiases(weightGrads, biasGrads, 0.03, FENs.size());
}