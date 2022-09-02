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


float Abbie::EvaluateFEN(string FEN) {
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
   Mat input = Mat(INPUT_SIZE, 1, model_input);
   Mat output = model_.compute(input);
   return output.getVal(0,0);
}

float std_dev(vector<float> input) {
   float sum = std::accumulate(std::begin(input), std::end(input), 0.0);
   float m =  sum / input.size();

   float accum = 0.0;
   std::for_each (std::begin(input), std::end(input), [&](const float d) {
      accum += (d - m) * (d - m);
   });

   return sqrt(accum / (input.size()-1)) + 0.0001;
}

GameState Abbie::playBotMove(Chess& game) {
   vector<Move> legalMoves = game.getLegalMoves();
   vector<float> evaluations {};

   string currentFEN = game.getFEN();
   Board currentBoard();
   char player = currentFEN[currentFEN.find(' ') + 1];
   bool playingAsWhite = player == 'w';

   for (auto move: legalMoves) {
      Board futureBoard(currentFEN);
      futureBoard.doMove(move);
      string futureFEN = futureBoard.genFEN();
      float futureEval = EvaluateFEN(futureFEN);
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
   for (unsigned i = 1; i < evaluations.size(); i++) {
      if (playingAsWhite) {
         if ((evaluations[i] + eval_std_dev) > best_evaluation) {
            bestMoves.push_back(legalMoves[i]);
         }
      } else {
         if ((evaluations[i] - eval_std_dev) < best_evaluation) {
            bestMoves.push_back(legalMoves[i]);
         }
      }
   }

   std::uniform_int_distribution<std::mt19937::result_type> dist(0,bestMoves.size() - 1);
   return game.acceptMove(bestMoves[dist(rng_)]);
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
            state = playBotMove(game);
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