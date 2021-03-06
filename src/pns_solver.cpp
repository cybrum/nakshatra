#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "eval_suicide.h"
#include "lmr.h"
#include "move.h"
#include "movegen.h"
#include "pn_search.h"
#include "stats.h"

#include <algorithm>
#include <cfloat>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, const char** argv) {
  Board board(Variant::SUICIDE);
  MoveGeneratorSuicide movegen(board);
  std::vector<std::string> egtb_filenames;
  assert(GlobFiles("egtb/*.egtb", &egtb_filenames));
  EGTB egtb(egtb_filenames, board);
  egtb.Initialize();
  EvalSuicide eval(&board, &movegen, &egtb);
  PNSParams pns_params;
  pns_params.max_nodes = 200000;
  pns_params.pns_type = PNSParams::PN2;
  pns_params.log_progress = 10;
  PNSearch pn_search(&board,
                     &movegen,
                     &eval,
                     &egtb,
                     nullptr);
  PNSResult pns_result;
  pn_search.Search(pns_params, &pns_result);
  return 0;
}
