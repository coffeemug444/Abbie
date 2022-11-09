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
    std::vector,
    std::future;

Abbie::Abbie()
{
   m_rng = std::mt19937(m_dev());
   m_model = BenBrain({INPUT_SIZE, 2048, 2048, 1}, 'h');
}

Abbie::Abbie(std::string model_path)
{
   m_rng = std::mt19937(m_dev());
   m_model = BenBrain(model_path);
}

float Abbie::evaluateFEN(string FEN) const
{
   Mat input = modelInputFromFEN(FEN);
   Mat output = m_model.compute(input);
   float outval = output.getVal(0, 0);
   return outval;
}

unsigned pieceOffset(Piece piece) {
   static const std::map<PieceType, unsigned> input_piece_index_map {
      {PAWN, 0*2*64},
      {ROOK, 1*2*64},
      {KNIGHT, 2*2*64},
      {BISHOP, 3*2*64},
      {QUEEN, 4*2*64},
      {KING, 5*2*64}
   };
   static const std::map<Color, unsigned> input_color_index_map {
      {BLACK, 0},
      {WHITE, 64}
   };
   return input_piece_index_map.at(piece.type) + input_color_index_map.at(piece.color);
}

Mat Abbie::modelInputFromBoard(Board& board) {
   std::vector<Piece> state = board.getState();
   Color current_player = board.getCurrentPlayer();
   bool white_oo = board.white_oo();
   bool white_ooo = board.white_ooo();
   bool black_oo = board.black_oo();
   bool black_ooo = board.black_ooo();
   vector<float> model_input(INPUT_SIZE);
   std::fill(begin(model_input), end(model_input), 0);
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
   model_input[after_pieces_index] = (current_player == WHITE ? 1.f : -1.f);
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

Move Abbie::getBotMove(Board& board, float &eval) {
   Board current_board(board);
   vector<Move> legal_moves = current_board.getLegalMoves();
   int num_moves = legal_moves.size();
   assert(num_moves != 0);
   if (num_moves == 1)
   {
      return legal_moves[0];
   }

   bool playing_as_white = current_board.getCurrentPlayer() == WHITE;

   Move best_move;
   const float negative_inf = - std::numeric_limits<double>::infinity();
   const float positive_inf =   std::numeric_limits<double>::infinity();
   float best_eval = (playing_as_white ? negative_inf : positive_inf);


   std::vector<Mat> inputs;
   for (auto &move : legal_moves) {
      Board next_board(current_board);
      next_board.doMove(move);
      inputs.push_back(modelInputFromBoard(next_board));
      if (next_board.kingIsInCheck()) {
         if (next_board.getLegalMoves().size() == 0) {
            // always return mate in 1
            return move;
         }
      }
   }

   std::vector<float> evals = evaluateInputs(inputs);

   for (int i = 0; i < legal_moves.size(); i++) {
      eval = evals[i];
      if (playing_as_white && eval > best_eval) {
         best_eval = eval;
         best_move = legal_moves[i];
      }
      if (!playing_as_white && eval < best_eval) {
         best_eval = eval;
         best_move = legal_moves[i];
      }
   }
   
   eval = best_eval;
   return best_move;
}

Move Abbie::getBotMove(string FEN, float &eval)
{
   Board current_board = Board(FEN);
   return getBotMove(current_board, eval);
}

void Abbie::playAgainst()
{
   Chess game;
   std::cout << game;
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
         std::cout << game;
         if (state == ONGOING)
         {
            string FEN = game.getFEN();
            float eval = 0.f;
            Move bot_move = getBotMove(FEN, eval);
            state = game.acceptMove(bot_move);
            for (int i = 0; i < 9; i++)
            {
               // clear line and move up
               cout << "\33[2k\033[A\r";
            }
            std::cout << game;
            cout << "\r                                                        \r";
            cout << "Board was rated " << eval << ". Enter move: ";
         }
      }
   }
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
   return E*(1.f-cos((M_PI*current)/N))/(2.0f);
}

void Abbie::compute_W_B_fromInputs (
   std::vector<Mat> inputs,
   float S,
   float E,
   std::vector<Mat>& weight_grads,
   std::vector<Mat>& bias_grads)
{
   unsigned N = inputs.size();
   vector<Mat> outputs;
   for (int i = 0; i < N; i++) {
      float eval = hindsightEvalAtState(S, E, N, i);
      outputs.push_back(modelOutputFromVal(eval));
   }

   if (N > 200) {
      // BenMat has trouble with very large matrix operations
      // split the load in half for everything to go well
      std::vector<Mat> nw;
      std::vector<Mat> nb;

      vector<Mat> inputs_first_half(begin(inputs), begin(inputs) + N/2);
      vector<Mat> outputs_first_half(begin(outputs), begin(outputs) + N/2);

      vector<Mat> inputs_second_half(begin(inputs) +  N/2 + 1, end(inputs));
      vector<Mat> outputs_second_half(begin(outputs) +  N/2 + 1, end(outputs));

      m_model.multipleBackPropagate(inputs_first_half,outputs_first_half,weight_grads,bias_grads);
      m_model.multipleBackPropagate(inputs_second_half,outputs_second_half,nw,nb);

      for (int i = 0; i < nw.size(); i++) {
         weight_grads[i] = weight_grads[i] + nw[i];
         bias_grads[i] = bias_grads[i] + nb[i];
      }
   } else {
      m_model.multipleBackPropagate(inputs,outputs,weight_grads,bias_grads);
   }
}

void Abbie::compute_W_B_fromInput(
    Mat& input,
    float eval,
    vector<Mat>& weight_grads,
    vector<Mat>& bias_grads)
{
   Mat output = modelOutputFromVal(eval);
   m_model.backPropagate(input, output, weight_grads, bias_grads);
}

Move Abbie::getRandomMove(Board& board) {
   // returns a "random" move. If it sees it can get
   // mate in 1 then it will return that move, otherwise
   // pick from current legal moves
   std::vector<Move> legal_moves = board.getLegalMoves();
   for (auto & move : legal_moves) {
      Board new_board(board);
      new_board.doMove(move);
      auto remainingMoves = new_board.getLegalMoves();
      if (remainingMoves.size() == 0) {
         if (new_board.kingIsInCheck()) {
            return move;
         }
      }
   }
   std::uniform_int_distribution<int> uid(0, legal_moves.size() - 1);
   return legal_moves[uid(m_rng)];
}

void Abbie::trainOneGame()
{
   Board board;
   std::vector<Mat> inputs;
   inputs.push_back(modelInputFromBoard(board));

   float starting_eval = 0.5f;
   float ending_eval;
   int move_count = 0;

   GameState game_state = ONGOING;

   // play the game
   while (game_state == ONGOING) {
      std::cout << "\033[F\33[2KMove " << move_count << "\n";
      float eval;
      std::uniform_int_distribution<int> uid(1, 100);
      Move move;
      if (uid(m_rng) <= 5) {
         // 5% chance of performing a random move
         move = getRandomMove(board);
      } else {
         move = getBotMove(board, eval);
      }
      move_count++;
      board.doMove(move);
      inputs.push_back(modelInputFromBoard(board));
      if(board.GameIsDraw()) {
         game_state = DRAW;
      }
      else if (board.getLegalMoves().size() == 0) {
         if (board.kingIsInCheck()) {
            game_state = CHECKMATE;
         } else {
            game_state = DRAW;
         }
      }
   }

   if (game_state == CHECKMATE)
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

   vector<Mat> weight_grads;
   vector<Mat> bias_grads;

   compute_W_B_fromInputs(inputs, starting_eval, ending_eval, weight_grads, bias_grads);
   m_model.adjustWeightsAndBiases(weight_grads, bias_grads, 0.03, inputs.size());
}