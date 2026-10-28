__kernel void count_words(__global const char* text,volatile __global int* total_words,const int chunk_size) {
  int my_id = get_global_id(0);
  if (my_id >= chunk_size - 1) {
    return;
  }
  char current_char = text[my_id];
  char next_char = text[my_id+1];
  int is_curr_letter = 0;
  int is_next_sep = 0;

  if ((current_char >= 48 && current_char <= 57) || (current_char >= 64 && current_char <= 122) || current_char == 39) {
    is_curr_letter = 1;
  }
  if (((next_char >= 0 && next_char <= 47) && next_char != 39) || (next_char >= 123 && next_char <= 127) || (next_char >= 58 && next_char <= 63)){
    is_next_sep = 1;
  }
  if(is_curr_letter == 1 && is_next_sep == 1) {
    atomic_inc(total_words);
  }
}
