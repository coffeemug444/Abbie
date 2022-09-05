#include "abbie.hpp"
#include <chrono>
#include <thread>
#include <future>

using namespace std::chrono_literals;
using std::chrono::system_clock,
      std::this_thread::sleep_for,
      std::cin, std::cout,
      std::string,
      std::shared_ptr, std::make_shared, std::make_unique,
      std::vector,
      std::future;

Abbie::Abbie() {
   rng_ = std::mt19937(dev_());
   model_ = BenBrain({INPUT_SIZE,100,100,1}, 'r');
}

Abbie::Abbie(std::string modelPath) {
   rng_ = std::mt19937(dev_());
   model_ = BenBrain(modelPath);
}

void Abbie::saveModel(std::string savePath) {
   model_.save(savePath);
}


float Abbie::evaluateFEN(string FEN, BenBrain* model) {
   Mat input = modelInputFromFEN(FEN);
   Mat output = model->compute(input);
   float outval = output.getVal(0,0);
   return outval;
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
   
   for(unsigned pc = 0; pc < NUM_PIECES; pc++) {
      for (unsigned sq = 0; sq < NUM_SQUARES; sq++) {
         float val = (pieces[pc] == squares[sq]) ? 1.f : 0.f;
         model_input[pc*NUM_SQUARES + sq] = val;
      }
   }
   return Mat(INPUT_SIZE, 1, model_input);
}

float std_dev(float* input, unsigned len) {
   float sum = 0;
   for (unsigned i = 0; i < len; i++) {
      sum += input[i];
   }
   float m =  sum / len;

   float accum = 0.0;
   for (unsigned i = 0; i < len; i++) {
      accum += (input[i] - m) * (input[i] - m);
   }

   return sqrt(accum / (len-1)) + 0.000001; // add small constant to std_dev in case all values are exactly the same
}


void Abbie::evaluateFutureMove(string FEN, Move move, BenBrain* model, float* output) {
      Board futureBoard(FEN);
      futureBoard.doMove(move);
      string futureFEN = futureBoard.genFEN();
      float futureEval = evaluateFEN(futureFEN, model);
      assert(!std::isnan(futureEval));
      *output = futureEval;
}


void Abbie::getEvals(vector<Move> legalMoves, float* evaluations, BenBrain* model, string FEN) {
   vector<future<void>> activePool {};

   for (unsigned i = 0; i < legalMoves.size(); i++) {
      while (activePool.size() >= 6) {
         for (int i = 0; i < activePool.size(); i++) {
            if(activePool[i].wait_for(0ms) == std::future_status::ready) {
               activePool.erase(activePool.begin() + i);
               i--;
            }
         }
      }
      activePool.push_back(std::async(evaluateFutureMove, FEN, legalMoves[i], model, evaluations + i));
   }
   for (auto& ftr: activePool) {
      ftr.wait();
   }
}


Move Abbie::getBotMove(string FEN, float& eval) {
   Board currentBoard = Board(FEN);
   vector<Move> legalMoves = currentBoard.getLegalMoves();
   int num_moves = legalMoves.size();
   assert(num_moves != 0);
   if (num_moves == 1) {
      return legalMoves[0];
   }
   float evaluations[num_moves];

   char player = FEN[FEN.find(' ') + 1];
   bool playingAsWhite = player == 'w';

   getEvals(legalMoves, evaluations, &model_, FEN);

   float best_evaluation = evaluations[0];

   for (unsigned i = 1; i < num_moves; i++) {
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

   float eval_std_dev = std_dev(evaluations, legalMoves.size());
   vector<Move> bestMoves {};
   vector<float> bestEvals {};
   for (unsigned i = 0; i < num_moves; i++) {
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
   model_.save("model.csv");
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
      std::cout << "\033[Fturn " << turnCount << "\n";
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
      float eval_shouldBe = ending_eval - (state - (FENs.size()-1))*(starting_eval/(FENs.size()));
      if (state != 0) {
         cout << "\033[F";
      }
      cout << "Computing weight and bias gradients for board " << state << "\n";
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


void Abbie::trainOneBoard() {
   Chess game;
   float eval;

   bool whitePlaying = true;
   string FEN1 = game.getFEN();
   Move move;
   cout << "Finding first move\n";
   move = getBotMove(FEN1, eval);
   game.acceptMove(move);
   string FEN2 = game.getFEN();
   cout << "Finding second move\n";
   move = getBotMove(FEN2, eval);
   game.acceptMove(move);

   vector<Mat> weightGrads = model_.weightsZero();
   vector<Mat> biasGrads = model_.biasesZero();

   unsigned board = 0;
   for(auto FEN: {FEN1, FEN2}) {
      board++;
      float eval_shouldBe = 0.5;

      Mat input = modelInputFromFEN(FEN);
      Mat output = modelOutputFromVal(eval_shouldBe);
      std::cout << "calculating diffs for board " << board << "\n";
      auto diffs = model_.backPropagate(input, output);

      std::cout << "computing weight gradients\n";
      auto nw = model_.computeWeights(diffs);
      std::cout << "adding weight gradients to gradient sum\n";
      for (int w = 0; w < weightGrads.size(); w++) {
         auto newWeightGrad = weightGrads[w] + nw[w];
         weightGrads[w] = newWeightGrad;
      }

      std::cout << "computing bias gradients\n";
      auto nb = model_.computeBiases(diffs);
      std::cout << "adding bias gradients to gradient sum\n";
      for (int b = 0; b < biasGrads.size(); b++) {
         auto newBiasGrad = biasGrads[b] + nb[b];
         biasGrads[b] = newBiasGrad;
      }
   }


   std::cout << "Adjusting weights and biases\n";
   model_.adjustWeightsAndBiases(weightGrads, biasGrads, 0.03, 2);
}