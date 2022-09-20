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

std::vector<float> Abbie::evaluateInputs(std::vector<Mat> inputs) {
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

unsigned pieceOffset(Piece piece) {
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
   return inputPieceIndexMap.at(piece.type) + inputColorIndexMap.at(piece.color);
}

Mat Abbie::modelInputFromBoard(Board& board) {
   std::shared_ptr<Piece[]> state = board.getState();
   Color currentPlayer = board.getCurrentPlayer();
   bool white_oo = board.white_oo();
   bool white_ooo = board.white_ooo();
   bool black_oo = board.black_oo();
   bool black_ooo = board.black_ooo();
   shared_ptr<float[]> model_input = make_unique<float[]>(INPUT_SIZE);
   memset(model_input.get(), 0, INPUT_SIZE*sizeof(float));
   for (int i = 0; i < 64; i++) {
      Piece piece = state[i];
      if (piece.type != BLANK_P) {
         auto index = pieceOffset(piece) + i;
         model_input[index] = 1;
      }
   }

   unsigned after_pieces_index = INPUT_SIZE - OTHER_INPUTS;
   // colour            1  (-1 for black, 1 for white)
   // P1 castling       2
   // P2 castling       2
   model_input[after_pieces_index] = (currentPlayer == WHITE ? 1.f : -1.f);
   model_input[after_pieces_index + 1] =  white_oo;
   model_input[after_pieces_index + 2] =  white_ooo;
   model_input[after_pieces_index + 3] =  black_oo;
   model_input[after_pieces_index + 4] =  black_ooo;

   return Mat(INPUT_SIZE, 1, model_input);
}

Mat Abbie::modelInputFromFEN(string FEN)
{
   Board board(FEN);
   return modelInputFromBoard(board);
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

Move Abbie::getBotMove(Board& board, float &eval) {
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


   std::vector<Mat> inputs;
   for (auto &move : legalMoves) {
      Board nextBoard(currentBoard);
      nextBoard.doMove(move);
      inputs.push_back(modelInputFromBoard(nextBoard));
      if (nextBoard.kingIsInCheck()) {
         if (nextBoard.getLegalMoves().size() == 0) {
            // always return mate in 1
            return move;
         }
      }
   }

   std::vector<float> evals = evaluateInputs(inputs);

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

Move Abbie::getBotMove(string FEN, float &eval)
{
   Board currentBoard = Board(FEN);
   return getBotMove(currentBoard, eval);
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
            Move botMove = getBotMove(FEN, eval);
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

float Abbie::hindsightEvalAtState(   
   float S,
   float E,
   unsigned N,
   unsigned current
   ) {
   // half a cosine curve going between -1 and 1 (or just flat 0 if draw)
   // Applies less weighting to the beginning states of a game and more to the end.
   // interactive visualization of rating system:
   // https://www.desmos.com/calculator/tqkv6yqygv
   return E*(cos((M_PI*current)/N) - 1.f)/(2.0f);
}

void Abbie::compute_W_B_fromInputs (
   std::vector<Mat> inputs,
   float S,
   float E,
   std::vector<Mat>& weightGrads,
   std::vector<Mat>& biasGrads)
{
   unsigned N = inputs.size();
   vector<Mat> outputs;
   for (int i = 0; i < N; i++) {
      float eval = hindsightEvalAtState(S, E, N, i);
      outputs.push_back(modelOutputFromVal(eval));
   }

   model_.multipleBackPropagate(inputs,outputs,weightGrads,biasGrads);
}

void Abbie::compute_W_B_fromInput(
    Mat& input,
    float eval,
    vector<Mat>& weightGrads,
    vector<Mat>& biasGrads)
{
   Mat output = modelOutputFromVal(eval);
   model_.backPropagate(input, output, weightGrads, biasGrads);
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
   Board board;
   std::vector<Mat> inputs;
   inputs.push_back(modelInputFromBoard(board));

   float starting_eval = 0.5f;
   float ending_eval;

   GameState gameState = ONGOING;

   // play the game
   while (gameState == ONGOING) {
      float eval;
      Move move = getBotMove(board, eval);
      board.doMove(move);
      inputs.push_back(modelInputFromBoard(board));
      if (board.getLegalMoves().size() == 0) {
         if (board.kingIsInCheck()) {
            gameState == CHECKMATE;
         } else {
            gameState == DRAW;
         }
      }
   }

   if (gameState == CHECKMATE)
   {
      if (board.getCurrentPlayer() == WHITE) {
         // white is playing but is in checkmate, black wins
         ending_eval = -1.f;
      } else {
         // black is playing but is in checkmate, white wins
         ending_eval = 1.f;
      }
   }
   else
   {
      // draw
      ending_eval = 0.0f;
   }

   vector<Mat> weightGrads;
   vector<Mat> biasGrads;

   compute_W_B_fromInputs(inputs, starting_eval, ending_eval, weightGrads, biasGrads);
   model_.adjustWeightsAndBiases(weightGrads, biasGrads, 0.03, inputs.size());
}