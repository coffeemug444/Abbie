#include "abbie.hpp"
#include <chrono>
#include <thread>
#include <future>
#include <map>

using namespace std::chrono_literals;
using std::chrono::system_clock,
    std::this_thread::sleep_for,
    std::cin, std::cout,
    std::string,
    std::shared_ptr, std::make_shared, std::make_unique,
    std::vector,
    std::future;

Abbie::Abbie()
{
   rng_ = std::mt19937(dev_());
   model_ = BenBrain({INPUT_SIZE, 2048, 2048, 1}, 'h');
}

Abbie::Abbie(std::string modelPath)
{
   rng_ = std::mt19937(dev_());
   model_ = BenBrain(modelPath);
}

void Abbie::saveModel(std::string savePath)
{
   model_.save(savePath);
}



std::vector<float> Abbie::evaluateStates(std::vector<std::shared_ptr<Piece[]>> states) {
   std::vector<Mat> inputs;
   for (auto& state : states) {
      inputs.push_back(modelInputFromState(state));
   }
   Mat outputs = model_.multipleCompute(inputs);
   std::shared_ptr<float[]> outVals = outputs.getVals();
   return std::vector<float>(outVals.get(), outVals.get() + outputs.getHeight());
}

float Abbie::evaluateFEN(string FEN)
{
   Mat input = modelInputFromFEN(FEN);
   Mat output = model_.compute(input);
   float outval = output.getVal(0, 0);
   return outval;
}

Mat Abbie::modelInputFromState(std::shared_ptr<Piece[]> state) {
   static const std::map<PieceType, unsigned> inputPieceIndexMap {
      {PAWN, 0*2*64},
      {ROOK, 1*2*64},
      {KNIGHT, 2*2*64},
      {BISHOP, 3*2*64},
      {QUEEN, 4*2*64},
      {KING, 5*2*64}
   };
   static const std::map<Color, unsigned> inputColorIndexMap {
      {BLACK, 0},
      {WHITE, 64}
   };

   shared_ptr<float[]> model_input = make_unique<float[]>(INPUT_SIZE);
   memset(model_input.get(), 0, INPUT_SIZE*sizeof(float));

   for (int i = 0; i < 64; i++) {
      Piece piece = state[i];
      if (piece.type != BLANK_P) {
         auto index = 
               inputPieceIndexMap.at(piece.type) + 
               inputColorIndexMap.at(piece.color);
         model_input[index] = 1;
      }
   }
   return Mat(INPUT_SIZE, 1, model_input);
}

Mat Abbie::modelInputFromFEN(string FEN)
{
   string FEN_pieces = FEN.substr(0, FEN.find(' '));
   vector<string> rows{};
   for (int i = 0; i < 6; i++)
   {
      rows.push_back(FEN_pieces.substr(0, FEN_pieces.find('/')));
      FEN_pieces = FEN_pieces.substr(FEN_pieces.find('/') + 1, FEN_pieces.size() - 1);
   }
   rows.push_back(FEN_pieces);
   char squares[NUM_SQUARES];
   unsigned square = 0;
   for (auto row : rows)
   {
      for (auto chr : row)
      {
         if (chr >= '1' && chr <= '8')
         {
            unsigned num_empty = chr - '0';
            for (unsigned i = 0; i < num_empty; i++)
            {
               squares[square] = '-';
               square++;
            }
         }
         else
         {
            squares[square] = chr;
            square++;
         }
      }
   }

   shared_ptr<float[]> model_input = make_unique<float[]>(INPUT_SIZE);
   vector<char> pieces = {
       'p', 'P', // pawn
       'r', 'R', // rook
       'n', 'N', // knight
       'b', 'B', // bishop
       'q', 'Q', // queen
       'k', 'K'  // king
   };

   for (unsigned pc = 0; pc < NUM_PIECES; pc++)
   {
      for (unsigned sq = 0; sq < NUM_SQUARES; sq++)
      {
         float val = (pieces[pc] == squares[sq]) ? 1.f : 0.f;
         model_input[pc * NUM_SQUARES + sq] = val;
      }
   }
   return Mat(INPUT_SIZE, 1, model_input);
}

float std_dev(float *input, unsigned len)
{
   float sum = 0;
   for (unsigned i = 0; i < len; i++)
   {
      sum += input[i];
   }
   float m = sum / len;

   float accum = 0.0;
   for (unsigned i = 0; i < len; i++)
   {
      accum += (input[i] - m) * (input[i] - m);
   }

   return sqrt(accum / (len - 1)) + 0.000001; // add small constant to std_dev in case all values are exactly the same
}

Move Abbie::getBotMove(Board& board, float &eval, int maxDepth, int initialDepth) {
   Board currentBoard(board);
   vector<Move> legalMoves = currentBoard.getLegalMoves();
   int num_moves = legalMoves.size();
   assert(num_moves != 0);
   if (num_moves == 1)
   {
      return legalMoves[0];
   }

   bool playingAsWhite = currentBoard.getCurrentPlayer() == WHITE;

   Move bestMove;
   float negativeInf = - std::numeric_limits<double>::infinity();
   float positiveInf =   std::numeric_limits<double>::infinity();
   float bestEval = (playingAsWhite ? negativeInf : positiveInf);


   std::vector<std::shared_ptr<Piece[]>> moveStates;
   for (auto &move : legalMoves) {
      Board nextBoard(currentBoard);
      nextBoard.doMove(move);
      moveStates.push_back(nextBoard.getState());
      if (nextBoard.kingIsInCheck()) {
         if (nextBoard.getLegalMoves().size() == 0) {
            // always return mate in 1
            return move;
         }
      }
   }

   std::vector<float> evals = evaluateStates(moveStates);

   for (int i = 0; i < legalMoves.size(); i++) {
      eval = evals[i];
      if (playingAsWhite && eval > bestEval) {
         bestEval = eval;
         bestMove = legalMoves[i];
      }
      if (!playingAsWhite && eval < bestEval) {
         bestEval = eval;
         bestMove = legalMoves[i];
      }
   }
   
   eval = bestEval;
   return bestMove;
}

Move Abbie::getBotMove(string FEN, float &eval, int maxDepth, int initialDepth)
{
   Board currentBoard = Board(FEN);
   return getBotMove(currentBoard, eval, maxDepth, initialDepth);
}

void Abbie::playAgainst()
{
   Chess game;
   game.printBoard();
   GameState state = ONGOING;

   cout << "\n\033[A";
   cout << "Enter move: ";

   while (state == ONGOING || state == BADMOVE)
   {
      string move;
      cin >> move;
      if (move == "q" || move == "exit")
      {
         exit(1);
      }
      state = game.acceptMove(move);

      if (state == BADMOVE)
      {
         cout << "\33[2k\033[A\r";
         cout << "                                                        \r";
         cout << "Move invalid. Enter move: ";
      }
      else
      {
         for (int i = 0; i < 10; i++)
         {
            // clear line and move up
            cout << "\33[2k\033[A\r";
         }
         game.printBoard();
         if (state == ONGOING)
         {
            string FEN = game.getFEN();
            float eval = 0.f;
            Move botMove = getBotMove(FEN, eval, 2, 1);
            state = game.acceptMove(botMove);
            for (int i = 0; i < 9; i++)
            {
               // clear line and move up
               cout << "\33[2k\033[A\r";
            }
            game.printBoard();
            cout << "\r                                                        \r";
            cout << "Board was rated " << eval << ". Enter move: ";
         }
      }
   }
}

Mat Abbie::modelOutputFromVal(float val)
{
   shared_ptr<float[]> output = make_unique<float[]>(1);
   output[0] = val;
   return Mat(1, 1, output);
}

void Abbie::compute_W_B_fromStates (
   std::vector<std::shared_ptr<Piece[]>> states,
   float S,
   float E,
   std::vector<Mat>& weightGrads,
   std::vector<Mat>& biasGrads)
{
   // S = starting eval (always 0.5)
   // E = ending eval   (1 or 0 based on who won the game)
   float N = states.size();

   // half a cosine curve going between 0.5 and (0 or 1).
   // Applies less weighting to the beginning states of a game and more to the end.
   // interactive visualization of rating system:
   // https://www.desmos.com/calculator/tqkv6yqygv
   vector<Mat> inputs;
   vector<Mat> outputs;
   for (int i = 0; i < N; i++) {
      float eval = (1-2*E)*(cos((M_PI*i)/N) - 1.f)/(4.0f) + 0.5f;
      outputs.push_back(modelOutputFromVal(eval));
      inputs.push_back(modelInputFromState(states[i]));
   }

   model_.multipleBackPropagate(inputs,outputs,weightGrads,biasGrads);
}

void Abbie::compute_W_B_forState(
    vector<string>& FENs,
    unsigned state,
    float S,
    float E,
    vector<Mat>& weightGrads,
    vector<Mat>& biasGrads)
{
   // S = starting eval (always 0.5)
   // E = ending eval   (1 or 0 based on who won the game)
   float N = FENs.size();

   // half a cosine curve going between 0.5 and (0 or 1).
   // Applies less weighting to the beginning states of a game and more to the end.
   // interactive visualization of rating system:
   // https://www.desmos.com/calculator/tqkv6yqygv
   float eval_shouldBe = (1-2*E)*(cos((M_PI*state)/N) - 1.f)/(4.0f) + 0.5f;


   Mat input = modelInputFromFEN(FENs[state]);
   Mat output = modelOutputFromVal(eval_shouldBe);
   vector<Mat> diffs;
   vector<Mat> as;

   model_.backPropagate(input, output, diffs, as);

   auto nw = model_.computeWeights(diffs, as);
   for (int w = 0; w < weightGrads.size(); w++)
   {
      auto newWeightGrad = weightGrads[w] + nw[w];
      weightGrads[w] = newWeightGrad;
   }
   auto nb = model_.computeBiases(diffs);
   for (int b = 0; b < biasGrads.size(); b++)
   {
      auto newBiasGrad = biasGrads[b] + nb[b];
      biasGrads[b] = newBiasGrad;
   }
}

Move Abbie::getRandomMove(std::string FEN) {
   // returns a "random" move. If it sees it can get
   // mate in 1 then it will return that move, otherwise
   // pick from current legal moves
   Board board(FEN);
   std::vector<Move> legalMoves = board.getLegalMoves();
   for (auto & legalMove : legalMoves) {
      Board newBoard(board);
      newBoard.doMove(legalMove);
      auto remainingMoves = newBoard.getLegalMoves();
      if (remainingMoves.size() == 0) {
         if (newBoard.kingIsInCheck()) {
            return legalMove;
         }
      }
   }
   std::uniform_int_distribution<int> uid(0, legalMoves.size() - 1);
   return legalMoves[uid(rng_)];
}

void Abbie::trainOneGame()
{
   Chess game;
   GameState gameState = ONGOING;
   vector<std::shared_ptr<Piece[]>> states{};

   auto thing = game.getBoardState();

   std::cout << "turn 0\n";

   bool whitePlaying = true;
   int turnCount = 0;
   while (gameState == ONGOING)
   {
      turnCount++;
      std::cout << "\033[Fturn " << turnCount << "\n";
      string FEN = game.getFEN();
      states.push_back(game.getBoardState());
      float eval;
      std::uniform_int_distribution<int> uid(1,20);
      Move move;
      if (uid(rng_) == 1) {
         // 5% chance of doing a random move
         move = getRandomMove(FEN);
      } else {
         move = getBotMove(FEN, eval, 0, 0);
      }

      gameState = game.acceptMove(move);
      whitePlaying = !whitePlaying;
   }
   states.push_back(game.getBoardState());

   float starting_eval = 0.5f;
   float ending_eval;
   if (gameState == CHECKMATE)
   {
      if (whitePlaying) {
         // white is playing but is in checkmate, black wins
         ending_eval = 0.f;
      } else {
         // black is playing but is in checkmate, white wins
         ending_eval = 1.f;
      }
   }
   else
   {
      // draw
      ending_eval = 0.5f;
   }

   vector<Mat> weightGrads;
   vector<Mat> biasGrads;

   std::cout << "Game was " << states.size() - 1 << " moves long\n";
   std::cout << "Computing weight and bias gradients\n";

   compute_W_B_fromStates(states, starting_eval, ending_eval, weightGrads, biasGrads);

   std::cout << "Adjusting weights and biases\n";
   model_.adjustWeightsAndBiases(weightGrads, biasGrads, 0.03, states.size());
}