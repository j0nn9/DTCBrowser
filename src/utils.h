/**
 * Header of the Datacoin Browser's utility functions
 *
 * Copyright (C)  2015  Jonny Frey  <j0nn9.fr39@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdlib.h>
#include <inttypes.h>
#include <string>
#include <string.h>
#include <vector>

using namespace std;

char *strdup(string str);

/**
 * converts a given integer to a string
 */
string itoa(int i);

/**
 * Returns if the given args containing the given arg
 */
bool has_arg(int argc, 
             char *argv[], 
             const char *short_arg, 
             const char *long_arg);

/**
 * Returns the given argument arg
 */
string get_arg(int argc, 
               char *argv[], 
               const char *short_arg, 
               const char *long_arg);
/**
 * Returns the given argument index
 */
int get_arg_i(int argc, 
              char *argv[], 
              const char *short_arg, 
              const char *long_arg);

/**
 * shorter macros to use in main
 */
#define has_arg1(long_arg) has_arg(argc, argv, NULL, long_arg)
#define has_arg2(short_arg, long_arg) has_arg(argc, argv, short_arg, long_arg)
#define has_arg4(argc, argv, short_arg, long_arg) \
  has_arg(argc, argv, short_arg, long_arg)
#define has_argx(X, T1, T2, T3, T4, FUNC, ...) FUNC
#define has_arg(...) has_argx(, ##__VA_ARGS__,            \
                                has_arg4(__VA_ARGS__),    \
                                "3 args not allowed",     \
                                has_arg2(__VA_ARGS__),    \
                                has_arg1(__VA_ARGS__))

#define get_arg1(long_arg) get_arg(argc, argv, NULL, long_arg)
#define get_arg2(short_arg, long_arg) get_arg(argc, argv, short_arg, long_arg)
#define get_arg4(argc, argv, short_arg, long_arg) \
  get_arg(argc, argv, short_arg, long_arg)
#define get_argx(X, T1, T2, T3, T4, FUNC, ...) FUNC
#define get_arg(...) get_argx(, ##__VA_ARGS__,            \
                                get_arg4(__VA_ARGS__),    \
                                "3 args not allowed",     \
                                get_arg2(__VA_ARGS__),    \
                                get_arg1(__VA_ARGS__))

#define get_arg_i1(long_arg) get_arg_i(argc, argv, NULL, long_arg)
#define get_arg_i2(short_arg, long_arg) get_arg_i(argc, argv, short_arg, long_arg)
#define get_arg_i4(argc, argv, short_arg, long_arg) \
  get_arg_i(argc, argv, short_arg, long_arg)
#define get_arg_ix(X, T1, T2, T3, T4, FUNC, ...) FUNC
#define get_arg_i(...) get_arg_ix(, ##__VA_ARGS__,              \
                                  get_arg_i4(__VA_ARGS__),      \
                                  "3 args not allowed",         \
                                  get_arg_i2(__VA_ARGS__),      \
                                  get_arg_i1(__VA_ARGS__))

/**
 * to get integer args
 */
#define get_i_arg(...)                                                    \
  (has_arg(__VA_ARGS__) ? atoi(get_arg(__VA_ARGS__).c_str()) : -1)

#define get_l_arg(...)                                                    \
  (has_arg(__VA_ARGS__) ? atol(get_arg(__VA_ARGS__).c_str() : -1)

#define get_ll_arg(...)                                                   \
  (has_arg(__VA_ARGS__) ? atoll(get_arg(__VA_ARGS__).c_str() : -1)

/**
 * to get float args
 */
#define get_f_arg(...)                                                    \
  (has_arg(__VA_ARGS__) ? atof(get_arg(__VA_ARGS__).c_str() : -1.0)

/* convert a string to uppercase */
string touppercase(string str);

/* convert a string to lowercase */
string tolowercase(string str);

/**
 * Splits a String by a specific char
 * returns vector<string> slitted Strings
 */
vector<string> split(const string str, const string seperator);

inline uint32_t rand32(uint32_t *x) {
  *x ^= *x << 13;
  *x ^= *x >> 17;
  *x ^= *x << 5;
  return *x;
}

/* random value */
typedef struct {
  uint32_t x, y, z, w;
} rand128_t; 

inline rand128_t *new_rand128_t(uint32_t seed) {
  rand128_t *rand = (rand128_t *) malloc(sizeof(rand128_t));

  rand->x = rand32(&seed);
  rand->y = rand32(&seed);
  rand->z = rand32(&seed);
  rand->w = rand32(&seed);

  return rand;
}
                      
inline uint32_t rand128(rand128_t *rand) {
                               
  uint32_t t = rand->x ^ (rand->x << 11);
  rand->x = rand->y; rand->y = rand->z; rand->z = rand->w;
  rand->w ^= (rand->w >> 19) ^ t ^ (t >> 8);
                                      
  return rand->w;
}


/* classify a file content */
string file_content(string file);

/**
 * converts an byte array to base64
 */
string to_b64(string str);

/**
 * decodes an b64 encoded string
 * (no new line chars allowed)
 */
string b64_to_byte(string b64);

/**
 * clone a string
 */
char *str_clone(const char *str);

/* compresses the given string with the bzip2 library */
string bzip2_compress(string str, int level = 9);

/* decompresses the given string with the bzip2 library */
string bzip2_decompress(string str);

/* reads th file at path and returns the content as string */
string read_file(string path);

/* convert string to hex */
string to_hex(string str);

/* returns the sha256 of data (hex format) */
string sha256sum(string data);

/* return the current time */
string get_time();

/* the overall log level */
extern unsigned loglevel;

/* log levels */
#define LOG_DD 5 /* debug level 2      */
#define LOG_D  4 /* debug level 1      */
#define LOG_I  3 /* information output */
#define LOG_W  2 /* warning            */
#define LOG_E  1 /* error              */
#define LOG_EE 0 /* fatal error        */

/* prints a synchronized message */
void log_str(string msg, unsigned log_level = LOG_I);

#define log_err(str, status) \
  log_str(string("[") + __FILE__ + ":" + itoa(__LINE__) + "] " + str, status)

string str_repleace(string str, string src, string target);


#endif /* __UTILS_H__ */
