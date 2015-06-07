/**
 * Implementation of the Datacoin Browser's utility functions
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
#include "utils.h"
#include <magic.h>
#include <iostream>
#include <bzlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#else
#include <sys/mman.h>
#endif
#include <unistd.h>
#include <errno.h>
#include "envelope.pb.h"
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include "std-magic.mgc.bz2.h"
#include "dtc-magic.mgc.bz2.h"

using namespace std;

char *strdup(string str) {
  
  char *dup = (char *) malloc(sizeof(char) * str.length() + 10);
  memset(dup, 0, sizeof(char) * str.length() + 10);
  memcpy(dup, str.c_str(), str.length());
  dup[str.length()] = 0;

  return dup;
}

/**
 * converts a given integer to a string
 */
string itoa(int i) {

  char a[32];

  if (i == 0) {
    a[0] = '0';
    a[1] = '\0';
    return string(a);
  }

  char is_neg = (i < 0);

  if (is_neg)
    i *= -1;

  int j;
  for (j = 0; i != 0; j++, i /= 10)
    a[j] = 48 + (i % 10);

  if (is_neg) {
     a[j] = '-';
    j++;
  }

  a[j] = '\0';
  string str = string(a);;
  return string(str.rbegin(), str.rend());
}

/**
 * Returns if the given args containing the given arg
 */
bool has_arg(int argc, 
             char *argv[], 
             const char *short_arg, 
             const char *long_arg) {

  int i;
  for (i = 1; i < argc; i++) {
    if ((short_arg != NULL && !strcmp(argv[i], short_arg)) ||
        (long_arg != NULL  && !strcmp(argv[i], long_arg))) {
      
      return true;
    } else if (strlen(argv[i]) == 2 && !strcmp(argv[i], "--"))
      return false;
  }

  return false;
}

/**
 * Returns the given argument arg
 */
string get_arg(int argc, 
               char *argv[], 
               const char *short_arg, 
               const char *long_arg) {

  int i;
  for (i = 1; i < argc - 1; i++) {
    if ((short_arg != NULL && !strcmp(argv[i], short_arg)) ||
        (long_arg != NULL  && !strcmp(argv[i], long_arg))) {
      
      return string(argv[i + 1]);
    } else if (strlen(argv[i]) == 2 && !strcmp(argv[i], "--"))
      return string("");
  }

  return string("");
}

/**
 * Returns the given argument index
 */
int get_arg_i(int argc, 
              char *argv[], 
              const char *short_arg, 
              const char *long_arg) {

  int i;
  for (i = 1; i < argc - 1; i++) {
    if ((short_arg != NULL && !strcmp(argv[i], short_arg)) ||
        (long_arg != NULL  && !strcmp(argv[i], long_arg))) {
      
      return i + 1;
    } else if (strlen(argv[i]) == 2 && !strcmp(argv[i], "--"))
      return -1;
  }

  return -1;
}

/* convert a string to uppercase */
string touppercase(string str) {
  
  string ret = "";

  for (unsigned i = 0; i < str.length(); i++)
    ret += toupper(str.c_str()[i]);

  return ret;
}

/**
 * Splits a String by a specific char
 * returns vector<string> slitted Strings
 */
vector<string> split(const string str, const string seperator) {

  char *copy = strdup(str);
  char *ptr = strtok(copy, seperator.c_str());

  vector<string> res;

  int i;
  for (i = 0; ptr != NULL; i++) {

    int size = strlen(ptr);
    res.push_back(string(ptr, size));
    ptr = strtok(NULL, seperator.c_str());
  }

  free(copy);
  return res;
}

/* convert a string to lowercase */
string tolowercase(string str) {

  string ret = "";

  for (unsigned i = 0; i < str.length(); i++)
    ret += tolower(str.c_str()[i]);

  return ret;
}

/* classify a file content */
string file_content(string file) {
  
  /* to identify file content */
  static magic_t magic_cookie;
  static bool init = false;

  if (!init) {
    init = true;

    string std_magic = bzip2_decompress(string((char *) std_magic_mgc_bz2, std_magic_mgc_bz2_len));
    string dtc_magic = bzip2_decompress(string((char *) dtc_magic_mgc_bz2, dtc_magic_mgc_bz2_len));

    char *buffer[2];
    buffer[0] = strdup(dtc_magic);
    buffer[1] = strdup(std_magic);

    size_t size[2];
    size[0] = dtc_magic.length();
    size[1] = std_magic.length();

    /* initialize file magic */
    magic_cookie = magic_open(MAGIC_MIME);
 
    if (magic_cookie == NULL) {
      cerr << "unable to initialize magic library" << endl;
      exit(EXIT_FAILURE);
    }

    if (magic_load_buffers(magic_cookie, (void **) buffer, size, 2) != 0) {
      cerr << "cannot load magic database - " << magic_error(magic_cookie) << endl;
      magic_close(magic_cookie);
      exit(EXIT_FAILURE);
    }
 }

  const char *content = magic_buffer(magic_cookie, file.c_str(), file.length());
  return string(content);
}

/**
 * prepares a base64 string to contain only valid characters
 * (no new line chars ans so on)
 */
string b64_prepare(string b64) {
  
  if (b64 == "") return "";
  stringstream ss;

  /**
   * Array which orders the base 64 elements to list code
   */
  static const bool code[] = { 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 
    1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, };

  for (unsigned i = 0; i < b64.length(); i++) {
    while (code[(unsigned char) b64[i]])
      i++;

    ss << b64[i];
  }

  return ss.str();
}

/**
 * converts an byte array to base64
 */
string to_b64(string str) {
  
  if (str == "") return "";
  static const char code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "abcdefghijklmnopqrstuvwxyz"
                             "0123456789+/";

  /* number of filling bytes */
  unsigned char *bytes = (unsigned char *) str.c_str();
  unsigned n_fill = (3 - (str.length() % 3)) % 3;
  stringstream ss;

  unsigned i;
  for (i = 0; i < str.length() - (3 - n_fill); i += 3) {
    ss << code[bytes[i] >> 2];
    ss << code[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
    ss << code[((bytes[i + 1] & 0xF) << 2) |
          (bytes[i + 2] >> 6)];
    ss << code[bytes[i + 2] & 0x3F];
  }

  if (n_fill == 0) {
    ss <<  code[bytes[i] >> 2];
    ss <<  code[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
    ss <<  code[((bytes[i + 1] & 0xF) << 2) |
          (bytes[i + 2] >> 6)];
    ss <<  code[bytes[i + 2] & 0x3F];
  } else if (n_fill == 1) {
    ss << code[bytes[i] >> 2];
    ss << code[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
    ss << code[((bytes[i + 1] & 0xF) << 2)];
    ss << '=';
  } else if (n_fill == 2) {
    ss << code[bytes[i] >> 2];
    ss << code[((bytes[i] & 3) << 4)];
    ss << '=';
    ss << '=';
  }

  return ss.str();
}

/**
 * decodes an b64 encoded string
 * (no new line chars allowed)
 */
string b64_to_byte(string b64) {

  b64 = b64_prepare(b64);
  
  /**
   * Array which orders the base 64 elements to list code
   */
  static const unsigned char code[] = { 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 
    255, 255,  63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 
    255,   0, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9, 
     10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24, 
     25, 255, 255, 255, 255, 255, 255,  26,  27,  28,  29,  30,  31,  32,  33, 
     34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48, 
     49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    255, };

  stringstream ss;
  unsigned n_fill = 0;
  unsigned i = 0;
  for (; b64[b64.length() - 1 - n_fill] == '='; n_fill++);
  
  if (b64.length() > 4) {
    for (; i < b64.length() - 4; i += 4) {
      
      ss << (char) ((code[(unsigned char) b64[i]] << 2)     |
                    (code[(unsigned char) b64[i + 1]] >> 4));
      ss << (char) ((code[(unsigned char) b64[i + 1]] << 4) |
                    (code[(unsigned char) b64[i + 2]] >> 2));
      ss << (char) ((code[(unsigned char) b64[i + 2]] << 6) |
                    code[(unsigned char) b64[i + 3]]);
    }
  }

  if (n_fill == 0) {

    ss << (char) ((code[(unsigned char) b64[i]] << 2)     |
                  (code[(unsigned char) b64[i + 1]] >> 4));
    ss << (char) ((code[(unsigned char) b64[i + 1]] << 4) |
                  (code[(unsigned char) b64[i + 2]] >> 2));
    ss << (char) ((code[(unsigned char) b64[i + 2]] << 6) |
                  code[(unsigned char) b64[i + 3]]);

  } else if (n_fill == 1) {

    ss << (char) ((code[(unsigned char) b64[i]] << 2)     |
                  (code[(unsigned char) b64[i + 1]] >> 4));
    ss << (char) ((code[(unsigned char) b64[i + 1]] << 4) |
                  (code[(unsigned char) b64[i + 2]] >> 2));

  } else if (n_fill == 2) {
    ss << (char) ((code[(unsigned char) b64[i]] << 2)     |
                  (code[(unsigned char) b64[i + 1]] >> 4));
  }

  return ss.str();
}

/**
 * clone a string
 */
char *str_clone(const char *str) {

  if (str == NULL)
    return NULL;

  char *clone;
  size_t len = strlen(str);

  clone = (char *) malloc(len + 1);
  memcpy(clone, str, len + 1);

  return clone;
}

/* compresses the given string with the bzip2 library */
string bzip2_compress(string str, int level) {

  unsigned len = (str.length() * 1.2 > 256) ? str.length() * 1.2 : 256;
  char *dst = (char *) malloc(len);

  int bz_ret = BZ2_bzBuffToBuffCompress(dst, &len, (char *) str.c_str(), str.length(), level, 0, 30);

  string ret = "";
  if (bz_ret == BZ_OK)
    ret = string(dst, len);

  if (bz_ret == BZ_CONFIG_ERROR)
    ret += "BZ_CONFIG_ERROR: the library has been mis-compiled";

  if (bz_ret == BZ_PARAM_ERROR) 
    ret += "BZ_PARAM_ERROR";

  if (bz_ret == BZ_MEM_ERROR)
    ret += "BZ_MEM_ERROR: insufficient memory is available";

  if (bz_ret == BZ_OUTBUFF_FULL)
    ret += "BZ_OUTBUFF_FULL: the size of the compressed data exceeds *destLen";

  free(dst);
  return ret;
}

/* decompresses the given string with the bzip2 library */
string bzip2_decompress(string str) {
    
  char *dst = NULL;
  unsigned len = str.length() * 5;

  int bz_ret = BZ_OUTBUFF_FULL;
  while (bz_ret == BZ_OUTBUFF_FULL) {

    if (dst != NULL)
      free(dst);

    len *= 2;
    dst = (char *) malloc(len);
    bz_ret = BZ2_bzBuffToBuffDecompress(dst, &len, (char *) str.c_str(), str.length(), 0,0);
  }

  string ret = "";
  if (bz_ret == BZ_OK)
    ret = string(dst, len);
  
  if (bz_ret == BZ_CONFIG_ERROR)
    ret += "BZ_CONFIG_ERROR: the library has been mis-compiled";

  if (bz_ret == BZ_PARAM_ERROR) 
    ret += "BZ_PARAM_ERROR";

  if (bz_ret == BZ_MEM_ERROR)
    ret += "BZ_MEM_ERROR: insufficient memory is available";

  if (bz_ret == BZ_DATA_ERROR)
    ret += "BZ_DATA_ERROR: a data integrity error was detected in the compressed data";

  if (bz_ret == BZ_DATA_ERROR_MAGIC)
    ret += "BZ_DATA_ERROR_MAGIC: the compressed data doesn't begin with the right magic bytes";

  if (bz_ret == BZ_UNEXPECTED_EOF)
    ret += "BZ_UNEXPECTED_EOF: the compressed data ends unexpectedly";


  free(dst);
  return ret;

}

/* reads th file at path and returns the content as string */
string read_file(string path) {

  if (access(path.c_str(), R_OK))
    return string(strerror(errno));

#ifdef WIN32
  FILE *f = fopen(path.c_str(), "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *str = (char *) malloc(fsize + 1);
  fread(str, fsize, 1, f);
  fclose(f);

  str[fsize] = 0;
  string ret(str, fsize);
  free(str);
  return ret;
#else

  int fd = open(path.c_str(), O_RDONLY);

  if (fd < 0)
    return string(strerror(errno));

  /* get file size */
  struct stat st; 

  if (stat(path.c_str(), &st))
    return string(strerror(errno));

  char *file = (char *) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file == MAP_FAILED)
    return string(strerror(errno));

  string ret = string(file, st.st_size);
  munmap(file, st.st_size);
  return ret;

#endif
}

/* convert string to hex */
string to_hex(string str) {
  
  stringstream ss;
  for (unsigned i = 0; i < str.length(); i++) {
    unsigned char c = str[i];

    if ((c >> 4) < 10)
      ss << ((int) (c >> 4));
    else
      ss << ((char) ((c >> 4) + 87));

    if ((c & 0xf) < 10)
      ss << ((int) (c & 0xf));
    else
      ss << ((char) ((c & 0xf) + 87));
  }

  return ss.str();
}

/* returns the sha256 of data (hex format) */
string sha256sum(string data) {
  
  uint8_t hash[SHA256_DIGEST_LENGTH];
  uint8_t tmp[SHA256_DIGEST_LENGTH];                                   

  SHA256_CTX sha256;                                                          
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, data.c_str(), data.length());                                   
  SHA256_Final(tmp, &sha256); 

  /* hash the result again */
  SHA256_Init(&sha256);                                                       
  SHA256_Update(&sha256, tmp, SHA256_DIGEST_LENGTH);  
  SHA256_Final(hash, &sha256);
  
  return to_hex(string((char *) hash, SHA256_DIGEST_LENGTH));
}

/**
 * mutex to avoid mutual exclusion by writing output
 */
static pthread_mutex_t io_mutex = PTHREAD_MUTEX_INITIALIZER;

/* return the current time */
string get_time() {
  time_t timer;
  char buffer[25];
  struct tm* tm_info;

  time(&timer);
  tm_info = localtime(&timer);

  strftime(buffer, 25, "[%Y-%m-%d %H:%M:%S] ", tm_info);
  return std::string(buffer);
}


/* the overall log level */
unsigned loglevel = LOG_DD;

/* prints a synchronized message */
void log_str(string msg, unsigned log_level) {

#ifdef WIN32
  while (pthread_mutex_trylock(&io_mutex))
    usleep(100000);
#else
  while (pthread_mutex_trylock(&io_mutex))
    pthread_yield();
#endif
  
  if (loglevel >= log_level) {
    
    switch (log_level) {
      case LOG_DD : cerr << "[D2]"; break;
      case LOG_D  : cerr << "[D1]"; break;
      case LOG_I  : cerr << "[II]"; break;
      case LOG_W  : cerr << "[EE]"; break;
      case LOG_E  : cerr << "[EE]"; break;
      case LOG_EE : cerr << "[FE]"; break;
      default     : cerr << "[UU]"; break;
    }

    cerr << get_time();
    cerr << msg << endl;
  } 

  if (log_level == LOG_EE)
    abort();
  pthread_mutex_unlock(&io_mutex);
}

string str_repleace(string str, string src, string target) {
  
  size_t pos = str.find(src);
  while (pos < str.length()) {
    str.replace(pos, src.length(), target);
    pos = str.find(src, pos + target.length());
  }

  return str;
}
