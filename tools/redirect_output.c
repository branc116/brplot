#define NOB_IMPLEMENTATION
#include "./external/nob.h"
#include <stdio.h>


int main(int argc, char** argv) {
  if (argc < 2) {
    nob_log(NOB_ERROR, "Usage: %s <output-file> <commands and args>", argv[0]);
    return 1;
  }

  const char* out_file_path = argv[1];
  Nob_Cmd cmd = { 0 };
  for (int i = 2; i < argc; ++i) {
    nob_cmd_append(&cmd, argv[i]);
  }
  if (false == nob_cmd_run(&cmd, .stdout_path = out_file_path)) return 1;
  nob_cmd_free(cmd);
  return 0;
}
