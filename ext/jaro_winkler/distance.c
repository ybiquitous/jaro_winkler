#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "distance.h"

typedef struct{
  unsigned long long code;
  unsigned int byte_length;
} UnicodeHash;

typedef struct{
  unsigned long long *ary;
  int length;
} Codepoints;

static UnicodeHash unicode_hash_new(const char *str);
static Codepoints codepoints_new(const char *str, int byte_len);
// static void build_matrix(const char **adj_table, int length);

Option option_new(){
  Option opt;
  opt.ignore_case = 0;
  opt.weight = 0.1;
  opt.threshold = 0.7;
  return opt;
}

double c_distance(char *s1, int byte_len1, char *s2, int byte_len2, Option opt){
  // set default option if NULL passed
  int free_opt_flag = 0;

  Codepoints code_ary_1 = codepoints_new(s1, byte_len1);
  Codepoints code_ary_2 = codepoints_new(s2, byte_len2);

  if(opt.ignore_case){
    for(int i = 0; i < code_ary_1.length; ++i) if(code_ary_1.ary[i] < 256 && islower(code_ary_1.ary[i])) code_ary_1.ary[i] -= 32;
    for(int i = 0; i < code_ary_2.length; ++i) if(code_ary_2.ary[i] < 256 && islower(code_ary_2.ary[i])) code_ary_2.ary[i] -= 32;
  }

  // Guarantee the order
  if(code_ary_1.length > code_ary_2.length){
    unsigned long long *tmp = code_ary_1.ary; code_ary_1.ary = code_ary_2.ary; code_ary_2.ary = tmp;
    int tmp2 = code_ary_1.length; code_ary_1.length = code_ary_2.length; code_ary_2.length = tmp2;
  }
  int window_size = code_ary_2.length / 2 - 1;
  if(window_size < 0) window_size = 0;
  double matches     = 0.0;
  int transpositions = 0;
  int previous_index = -1;
  int max_index      = code_ary_2.length - 1;
  for(int i = 0; i < code_ary_1.length; i++){
    int left  = i - window_size;
    int right = i + window_size;
    if(left  < 0) left = 0;
    if(right > max_index) right = max_index;
    char matched = 0, found = 0;
    for(int j = left; j <= right; j++){
      if(code_ary_1.ary[i] == code_ary_2.ary[j]){
        matched = 1;
        if(!found && j > previous_index){
          previous_index = j;
          found = 1;
        }
      } // if(code_ary_1.ary[i] == code_ary_2.ary[j]){
    } // for(int j = left; j <= right; j++){
    if(matched){
      matches++;
      if(!found) transpositions++;
    }
  } // for(int i = 0; i < code_ary_1.length; i++){

  // Adjusting table
  // build_matrix(DEFAULT_ADJ_TABLE, sizeof(DEFAULT_ADJ_TABLE) / 8);

  // Don't divide transpositions by 2 since it's been counted directly by above code.
  double jaro_distance =  matches == 0 ? 0 : (matches / code_ary_1.length + matches / code_ary_2.length + (matches - transpositions) / matches) / 3.0;

  // calculate jaro-winkler distance
  double threshold = opt.threshold, weight = opt.weight;
  int prefix = 0;
  int max_length = code_ary_1.length > 4 ? 4 : code_ary_1.length;
  for(int i = 0; i < max_length; ++i){
    if(code_ary_1.ary[i] == code_ary_2.ary[i]) prefix++;
    else break;
  }
  free(code_ary_1.ary); free(code_ary_2.ary);
  return jaro_distance < threshold ? jaro_distance : jaro_distance + ((prefix * weight) * (1 - jaro_distance));
}

static UnicodeHash unicode_hash_new(const char *str){
  UnicodeHash ret;
  unsigned char first_char = str[0];
  if(first_char >= 252) ret.byte_length = 6;      // 1111110x
  else if(first_char >= 248) ret.byte_length = 5; // 111110xx
  else if(first_char >= 240) ret.byte_length = 4; // 11110xxx
  else if(first_char >= 224) ret.byte_length = 3; // 1110xxxx
  else if(first_char >= 192) ret.byte_length = 2; // 110xxxxx
  else ret.byte_length = 1;
  memcpy(&ret.code, str, ret.byte_length);
  return ret;
}

static Codepoints codepoints_new(const char *str, int byte_len){
  Codepoints ret = {0};
  ret.ary = calloc(byte_len, sizeof(long long));
  int count = 0;
  for(int i = 0; i < byte_len;){
    UnicodeHash hash = unicode_hash_new(str + i);
    ret.ary[count] = hash.code;
    count++;
    i += hash.byte_length;
  }
  ret.length += count;
  return ret;
}
