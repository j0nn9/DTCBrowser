/**
 * Implementation of the Datacoin Browser's database Back-End
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "Database.h"
#include "Rpc.h"
#include "utils.h"
#include "envelope.pb.h"
#include <pthread.h>

using namespace std;

#ifdef DEBUG
#ifdef DEBUG_SQLITE_SPEED
int sqlite3_exec_speed_check(
    sqlite3 *db,                               /* An open database */
    const char *sql,                           /* SQL to be evaluated */
    int (*callback)(void*,int,char**,char**),  /* Callback function */
    void *arg,                                 /* 1st argument to callback */
    char **errmsg                              /* Error msg written here */
    ) {
  
  uint64_t start = gettime_usec();
  int ret = sqlite3_exec(db, sql, callback, arg, errmsg);
  log_str("SQL SPEED[" + itoa(gettime_usec() - start) + "]: " + string(sql), LOG_D);

  return ret;
}

int sqlite3_prepare_v2_speed_check(
    sqlite3 *db,            /* Database handle */
    const char *zSql,       /* SQL statement, UTF-8 encoded */
    int nByte,              /* Maximum length of zSql in bytes. */
    sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
    const char **pzTail     /* OUT: Pointer to unused portion of zSql */
    ) {

  uint64_t start = gettime_usec();
  int ret = sqlite3_prepare_v2(db, zSql, nByte, ppStmt, pzTail);
  log_str("SQL SPEED[" + itoa(gettime_usec() - start) + "]: " + string(zSql), LOG_D);

  return ret;
}

#define sqlite3_exec sqlite3_exec_speed_check
#define sqlite3_prepare_v2 sqlite3_prepare_v2_speed_check
#endif
#endif


#define sqlite3_column_string(query, column)                          \
  ((sqlite3_column_text(query, column) == NULL) ? string("") :        \
   string((char *) sqlite3_column_text(query, column)))

/* synchronization mutexes */
pthread_mutex_t Database::creation_mutex = PTHREAD_MUTEX_INITIALIZER;

/* this will be a singleton */
Database *Database::only_instance = NULL;

/* all currently known testnet data blocks */
unsigned Database::testnet_data_blocks[] = {
19957,  19971,  19976,  19977,  19980,  19999,  20008,  20010,  20014,  20016, 
20019,  20272,  20281,  20283,  20284,  20286,  20287,  20289,  20293,  20294, 
20295,  20297,  21338,  21342,  21344,  21377,  21380,  21381,  34208,  34255, 
34281,  34317,  34347,  34379,  34389,  34395,  34401,  34407,  36060,  36068, 
36073,  36080,  36086,  36092,  36098,  36105,  36113,  36121,  36128,  36135, 
36142,  36148,  36155,  36161,  36167,  36174,  36182,  36190,  36198,  36206, 
36213,  36219,  36225,  36238,  36303,  36333,  36357,  36379,  36401,  36422, 
36443,  36465,  36488,  36510,  36531,  36554,  36575,  36596,  36618,  36640, 
36663,  36687,  36732,  36753,  36775,  36798,  36819,  36843,  36866,  36887, 
36910,  36932,  36957,  36979,  37002,  37023,  37046,  37067,  37090,  37113, 
37135,  37158,  37179,  37201,  37224,  37246,  37270,  37292,  37313,  37335, 
37357,  37380,  37402,  37424,  37446,  37468,  37491,  37512,  37535,  37558, 
37579,  37602,  37628,  37651,  37672,  37695,  37717,  37740,  37762,  37785, 
37808,  37829,  37851,  37873,  37894,  37918,  37940,  37964,  37987,  38009, 
38030,  38053,  38074,  38096,  38119,  38141,  38223,  38246,  38268,  38291, 
38312,  38335,  38356,  38379,  38402,  38423,  38445,  38466,  38489,  38511, 
38534,  38555,  38577,  38600,  38622,  38643,  38665,  38688,  38710,  38733, 
38755,  38776,  38799,  38822,  38844,  38866,  38890,  38913,  38936,  38958, 
38981,  39002,  39025,  39047,  39071,  39093,  39114,  39136,  39159,  39180, 
39202,  39223,  39245,  39268,  39289,  39311,  39332,  39353,  39376,  39399, 
39420,  39442,  39464,  39486,  39509,  39532,  39555,  39578,  39599,  39620, 
39643,  39665,  39686,  39708,  39731,  39752,  39775,  39796,  39818,  39839, 
39862,  39883,  39905,  39928,  39950,  39973,  39994,  40016,  40038,  40060, 
40083,  40105,  40127,  40149,  40170,  40193,  40217,  40239,  40260,  40282, 
40305,  40326,  40348,  40371,  40393,  40416,  40438,  40460,  40481,  40504, 
40527,  40548,  40661,  40683,  40705,  40727,  40749,  40771,  40793,  40814, 
40837,  40859,  40881,  40903,  40926,  40948,  40969,  40990,  41012,  41034, 
41057,  41079,  41101,  41123,  41146,  41168,  41189,  41212,  41234,  41257, 
41280,  41302,  41325,  41347,  41370,  41393,  41415,  41437,  41459,  41481, 
41503,  41525,  41547,  41570,  41591,  41614,  41637,  41659,  41680,  41701, 
41722,  41745,  41766,  41788,  41809,  41831,  41854,  41876,  41898,  41919, 
46255,  48255,  48277,  48300,  48329,  48351,  48357,  48363,  48369,  48374, 
48379,  48391,  48392,  48393,  48394,  48418,  48420,  48422,  48424,  48426, 
48428,  48431,  48434,  48436,  48458,  48471,  48494,  48573,  48606,  48912, 
49018,  49135,  49241,  49346,  49452,  49557,  49664,  49770,  49873,  49979, 
50081,  50184,  50286,  50391,  50496,  50600,  50703,  50811,  50918,  51019, 
51122,  51226,  51333,  51446,  51550,  51656,  51761,  51866,  51973,  52075, 
52180,  52320,  52427,  52532,  52543,  52556,  52570,  54117,  54130,  54142, 
54154,  54166,  54180,  54193,  54210,  54224,  54236,  54249,  54261,  54465, 
54526,  54541,  54554,  54568,  54619,  54632,  54646,  54661,  54674,  54689, 
54703,  54717,  54730,  54744,  54757,  54773,  54785,  54799,  54813,  54825, 
54838,  54851,  54865,  54876,  54889,  54902,  54916,  54930,  54944,  54957, 
54971,  54985,  55000,  55013,  55029,  55041,  55052,  55066,  55080,  55172, 
55185,  55198,  55210,  55226,  55240,  55255,  55266,  55280,  55292,  55306, 
55320,  55335,  55346,  55361,  55375,  55391,  55403,  55414,  55518,  55614, 
55720,  55744,  55954,  55969,  55984,  55998,  56011,  56024,  56032,  56046, 
56059,  56069,  56078,  56087 };

/* all currently known data blocks */
unsigned Database::data_blocks[] = {
 18587,  19996,  19997,  19998,  25030,  25036,  25891,  25895,  25903,  25941, 
 25952,  26233,  26271,  41710,  41711,  41746,  41788,  41804,  41806,  41824, 
 41855,  42227,  42259,  42390,  42391,  42460,  43146,  43156,  43540,  44175, 
 44546,  45082,  46351,  48469,  51583,  54488,  58403,  58404,  58753,  59338, 
 59339,  62063,  64445,  65254,  65255,  66436,  67437,  68286,  68289,  68301, 
 68572,  73848,  75693,  76409,  76410,  78125,  79430,  79556,  81788,  82062, 
 82548,  82949,  82963,  83561,  84644,  85684,  85686,  85688,  85715,  85717, 
 85772,  85863,  85868,  86124,  86971,  87349,  87656,  87707,  90123,  92779, 
 93713,  93772,  93990,  94984,  95065,  97468,  98479,  98750,  98751,  98841, 
 99685, 100013, 100023, 101778, 102696, 103404, 106642, 106648, 107121, 107733, 
107978, 108000, 108262, 108450, 108990, 108991, 117061, 126996, 131513, 133165, 
134089, 134433, 134440, 135479, 137631, 137886, 137887, 138364, 138371, 139483, 
139899, 139904, 141072, 148744, 148767, 148772, 148773, 148916, 152466, 152771, 
155707, 155987, 156361, 157749, 157750, 158709, 158772, 158944, 159251, 161913, 
170172, 170874, 187659, 200561, 202119, 215042, 215043, 215055, 215252, 215253, 
215308, 220105, 223563, 223726, 223741, 224347, 224452, 224476, 224583, 224591, 
224706, 224734, 224850, 225655, 225659, 226744, 226792, 228088, 228099, 231467, 
232368, 232630, 232662, 232675, 233949, 233975, 233985, 234003, 234008, 234012, 
234162, 234181, 237581, 237605, 237607, 237632, 237694, 237701, 237747, 239945, 
239951, 239968, 240269, 242785, 247745, 247749, 247760, 247774, 248931, 249021, 
249059, 249064, 249071, 249099, 249107, 249126, 249133, 249145, 249151, 249154, 
249178, 249188, 249322, 249388, 249394, 249396, 249657, 252007, 252377, 252745, 
253203, 253212, 253580, 253648, 253665, 254101, 254488, 254492, 254493, 254942, 
260468, 264776, 269134, 271243, 275874, 280035, 280037, 280366, 280519, 280523, 
280541, 280574, 281770, 281840, 282246, 282305, 282500, 282673, 282674, 282707, 
282744, 282745, 282782, 282785, 282810, 282836, 282855, 283562, 283620, 283659, 
283660, 284510, 285748, 285779, 285783, 285801, 285805, 285844, 285857, 285918, 
286116, 288709, 289231, 289441, 290438, 290748, 291020, 291710, 294037, 294048, 
294050, 294408, 294419, 294443, 294445, 294752, 295471, 296105, 296208, 296282, 
296298, 297776, 297847, 297849, 297881, 297931, 298256, 298263, 298266, 302629, 
312375, 312382, 315102, 315169, 326895, 327263, 330807, 332286, 332348, 332360, 
333174, 333375, 345585, 345590, 345591, 348645, 348697, 348698, 348717, 348728, 
348774, 348790, 348858, 349732, 350016, 350798, 351282, 351297, 351302, 351303, 
352333, 352482, 352492, 352495, 352531, 352678, 352685, 352688, 352729, 352730, 
352731, 352733, 352737, 352799, 352824, 352826, 352829, 352835, 352836, 352841, 
353207, 353210, 353223, 353244, 353539, 353677, 353686, 353777, 353790, 353813, 
353940, 353960, 354019, 354231, 354247, 354640, 354641, 354920, 355722, 355723, 
355740, 355752, 355758, 355772, 355986, 355993, 355994, 356034, 356087, 356691, 
357016, 357019, 357026, 357027, 357067, 357116, 357330, 357404, 357423, 358398, 
358429, 358638, 358649, 358658, 358720, 359241, 359669, 360437, 360540, 361858, 
361872, 361882, 361884, 362164, 362401, 362694, 363452, 363488, 363836, 364719, 
365104, 365156, 365381, 365570, 365596, 365597, 365611, 365612, 365786, 365787, 
365958, 367177, 367190, 367196, 367205, 368187, 368196, 368299, 368310, 368554, 
369228, 369613, 369852, 369868, 369984, 370006, 370009, 370062, 370087, 371221, 
371362, 371368, 371374, 371512, 371514, 371753, 371880, 372081, 373263, 373412, 
374463, 374880, 376038, 377085, 377086, 377767, 378059, 379417, 380679, 380915, 
382415, 382558, 382566, 383567, 385524, 385526, 385718, 385751, 388063, 388070, 
389538, 389600, 389608, 389660, 389808, 390725, 390785, 390793, 390871, 391429, 
392065, 393395, 393578, 394790, 394895, 395276, 395404, 395636, 396109, 396937, 
397393, 397396, 397605, 398683, 398835, 398849, 398861, 400083, 400589, 400590, 
400897, 401182, 401185, 401195, 401211, 401230, 401411, 402427, 402595, 402646, 
403044, 403053, 403071, 403150, 403151, 403155, 403208, 403233, 403469, 403696, 
403751, 403798, 403801, 403804, 403811, 403812, 403817, 403822, 403834, 403838, 
403863, 403892, 403894, 403897, 403922, 403926, 403928, 403930, 403931, 403932, 
403935, 403937, 403941, 403944, 403948, 403950, 403953, 403956, 403958, 404370, 
404427, 405160, 405162, 405244, 405352, 405434, 405577, 405786, 405873, 405880, 
406314, 406612, 406617, 406618, 406809, 406932, 406951, 407101, 407103, 407104, 
407130, 407161, 407222, 407238, 407266, 407454, 407481, 407484, 407485, 407504, 
407511, 407516, 407569, 407591, 408371, 408396, 408457, 408458, 408610, 408642, 
408833, 408870, 408903, 408952, 409079, 409147, 409158, 409167, 409175, 409191, 
409196, 409202, 409253, 409624, 409720, 409721, 409722, 409731, 409732, 409895, 
410177, 410179, 410244, 410472, 410512, 410607, 410731, 410740, 410781, 411400, 
411410, 411489, 411494, 411550, 411559, 411861, 411887, 412046, 412068, 412079, 
412086, 412088, 412089, 412109, 412110, 412132, 412134, 412136, 412139, 412150, 
412155, 412171, 412177, 412270, 412277, 412280, 412281, 412288, 412292, 412316, 
412320, 412326, 412333, 412379, 412740, 412969, 413031, 413085, 413101, 413219, 
413419, 413446, 413472, 413473, 413476, 413480, 413488, 413499, 413502, 413503, 
413507, 413514, 413515, 413521, 413543, 413553, 413573, 413588, 413685, 413687, 
413690, 413804, 413811, 413925, 414038, 414061, 414067, 414163, 414238, 414240, 
414498, 414518, 414530, 414531, 414535, 414537, 414665, 414734, 414735, 414762, 
414848, 414879, 414882, 414906, 414910, 414940, 415054, 415065, 415251, 415297, 
415300, 415302, 415389, 415461, 415641, 415747, 415838, 415856, 415895, 416100, 
416174, 416247, 416348, 416350, 416363, 416365, 416366, 416398, 416422, 416680, 
416686, 416808, 416997, 417046, 417057, 417158, 417166, 417173, 417175, 417231, 
417291, 417303, 417374, 417562, 417575, 417591, 417592, 417594, 417614, 417615, 
417623, 417647, 417665, 417718, 417731, 417763, 417787, 417794, 417946, 418008, 
418009, 418034, 418055, 418113, 418136, 418137, 418139, 418141, 418143, 418144, 
418145, 418147, 418157, 418160, 418161, 418163, 418199, 418413, 418417, 418427, 
418436, 418485, 418532, 418539, 418560, 418624, 418663, 418712, 418746, 418788, 
418816, 418847, 418875, 418911, 418938, 418969, 419011, 419047, 419048, 419084, 
419093, 419424, 419430, 419452, 419456, 419458, 419657, 419686, 419688, 419694, 
420064, 420074, 420152, 420309, 420326, 420442, 420510, 420679, 420698, 420875, 
420918, 420920, 420921, 420923, 420924, 420925, 420927, 420928, 420929, 420930, 
420931, 420936, 420990, 421021, 421098, 421113, 421190, 421284, 421285, 421286, 
421296, 421297, 421300, 421341, 421355, 421457, 421614, 421637, 421850, 422149, 
422460, 422805, 422806, 422810, 422818, 422831, 422832, 422840, 422843, 422847, 
422981, 422993, 422994, 423007, 423200, 423715, 423958, 423968, 424344, 424367, 
424375, 424402, 424490, 424505, 424539, 425614, 425992, 426041, 426139, 426797, 
426935, 426968, 427254, 427429, 427437, 427458, 427461, 427466, 427482, 427546, 
427643, 427842, 427951, 428029, 428038, 428043, 428047, 428054, 428064, 428068, 
428072, 428073, 428075, 428077, 428089, 428093, 428098, 428102, 428416, 428423, 
428463, 428544, 428548, 428616, 428617, 428618, 428725, 428726, 428727, 428786, 
428837, 428856, 428918, 428974, 429007, 429008, 429082, 429083, 429084, 429087, 
429088, 429722, 429740, 429783, 430018, 430038, 430184, 430213, 430250, 430325, 
430358, 430374, 430410, 430702, 430937, 431156, 432436, 432997, 433419, 433423, 
433571, 433690, 433740, 433793, 434692, 435253, 435278, 435804, 435823, 435844, 
435897, 436261, 436409, 436427, 436657, 436668, 436774, 437231, 437274, 437300, 
437313, 437576, 438041, 438325, 438393, 438550, 439038, 439040, 439057, 439150, 
440165, 440203, 440214, 440258, 440538, 440579, 440995, 441156, 441431, 441489, 
441712, 441739, 441740, 442131, 442294, 442374, 442375, 442380, 442443, 442465, 
442487, 442544, 442845, 443776, 444319, 444328, 444339, 444374, 444703, 444890, 
444929, 445034, 445056, 445155, 445195, 445273, 445300, 445338, 445339, 445358, 
445387, 445446, 445452, 445625, 445701, 445754, 445757, 446221, 446301, 446310, 
446348, 446428, 446440, 446464, 446488, 446490, 446750, 446793, 447094, 447271, 
447277, 447691, 448061, 448597, 448614, 448635, 448672, 448675, 448809, 449295, 
449724, 450142, 451061, 453247, 453967, 456030, 456031, 456551, 456563, 456568, 
457510, 457976, 457978, 459005, 459361, 459820, 459855, 460000, 460008, 460092, 
462316, 462435, 463994, 465735, 467844, 469191, 470594, 470703, 472325, 473409, 
473450, 475122, 475176, 475248, 475256, 475280, 481214, 481961, 481999, 490330, 
490860, 490979, 492855, 493182, 494857, 495391, 495878, 495893, 496853, 497457, 
500174, 501375, 501376, 514467, 553591, 565507, 579710, 579725, 609684, 649380, 
662202, 662527, 662537, 702345, 704913, 706058, 708167, 708168, 708296, 708298, 
709205, 709208, 709250, 709252, 709421, 709423, 709455, 709461, 709507, 709518, 
709519, 709532, 712436, 719871, 723525, 741081, 794852, 794854, 802812, 805688, 
805774, 805778, 831180, 839248, 841156, 853472, 853473, 854400, 854401, 866839, 
866852, 866859, 866866, 866875, 866898, 866908, 866922, 866929, 866935, 866945, 
866962, 866969, 866975, 866988, 867007, 867066, 867123, 867150, 867167 };

/* indicates if this was initialized */
bool Database::initialized = false;

void *Database::db_thread(void *args) {
  
  Database *db = (Database *) args;

  if (db->scann_fast) {
    if (db->testnet) {
      db->parse_specific_blocks(vector<unsigned>(testnet_data_blocks, 
                                                 testnet_data_blocks + 
                                                 sizeof(testnet_data_blocks) / 
                                                 sizeof(unsigned)));
    } else {
      db->parse_specific_blocks(vector<unsigned>(data_blocks, 
                                                 data_blocks + 
                                                 sizeof(data_blocks) / 
                                                 sizeof(unsigned)));
    }
  }
  db->parse_blocks();

  while (db->running) {
    sleep(30);
    db->parse_blocks();
  }

  return NULL;
}

/* return the only instance of this */
Database *Database::get_instance(const char *path, bool scann_fast, bool testnet) {

  pthread_mutex_lock(&creation_mutex);
  if (!initialized && path != NULL) {
    only_instance = new Database(path, scann_fast, testnet);
    initialized   = true;
  }
  pthread_mutex_unlock(&creation_mutex);

  return only_instance;
}


/* private constructor to allow only one instance of this */
Database::Database(const char *path, bool scann_fast, bool testnet) {

  this->running     = true;
  this->scann_fast  = scann_fast;
  this->testnet     = testnet;

  if (sqlite3_initialize() != SQLITE_OK)
    log_err("Failed to initialize sqlite library", LOG_EE);
  

  /* if file doesn't exist create it */
  if (access(path, F_OK)) {
  
    if (sqlite3_open_v2(path, 
                        &db, 
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 
                        NULL)) {

      log_err("Can't open database: " + string(sqlite3_errmsg(db)), LOG_EE);
    }

    /* create table structure */
    char *tx, *envelope, *insert, *my_files, *file_txids; 
    tx = "CREATE TABLE TX("                                  \
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"             \
         "id_block       INT                   NOT NULL,"    \
         "id_envelope    INT                   ,"            \
         "txid           CHAR(64)              NOT NULL,"    \
         "type           CHAR(64)              NOT NULL);";
    
    envelope = "CREATE TABLE ENVELOPE("                      \
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"       \
               "filename             TEXT        ,"          \
               "compression          INT         NOT NULL,"  \
               "publickey            TEXT        ,"          \
               "signature            TEXT        ,"          \
               "partnumber           INT         ,"          /* starts counting by 1 */\
               "totalparts           INT         ,"          \
               "prevtxid             TEXT        ,"          /* revers to TX table */\
               "prevdatahash         TEXT        ,"          \
               "datahash             TEXT        ,"          \
               "datetime             INT         ,"          \
               "version              INT         );";

    my_files = "CREATE TABLE MY_FILES("                      \
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"       \
               "filename           TEXT          NOT NULL,"  \
               "datahash           TEXT          NOT NULL,"  \
               "content_type       TEXT          NOT NULL,"  \
               "compression        INT           NOT NULL,"  \
               "publickey          TEXT          NOT NULL,"  \
               "status             INT           NOT NULL);";

    file_txids = "CREATE TABLE FILE_TXIDS("                   \
                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"      \
                 "file_id          INT           NOT NULL,"   \
                 "partnumber       INT           NOT NULL,"   \
                 "totalparts       INT           NOT NULL,"   \
                 "signature        TEXT                  ,"   \
                 "prevdatahash     TEXT                  ,"   \
                 "datetime         INT                   ,"   \
                 "version          INT                   ,"   \
                 "data             TEXT          NOT NULL,"   \
                 "txid             TEXT                  ,"   \
                 "error            TEXT                  );";
               

    insert = "INSERT INTO ENVELOPE (id, compression) VALUES (0, 0);"      \
             "INSERT INTO TX (id, id_block, txid, type) VALUES "          \
             "(0, 0, '0x0', 'null');";


    char *err_msg = NULL;
    if (sqlite3_exec(db, tx, NULL, NULL, &err_msg))
      log_err("Failed to create table tx: " + string(err_msg), LOG_EE);

    if (sqlite3_exec(db, envelope, NULL, NULL, &err_msg))
      log_err("Failed to create table envelope: " + string(err_msg), LOG_EE);

    if (sqlite3_exec(db, my_files, NULL, NULL, &err_msg))
      log_err("Failed to create table my_files: " + string(err_msg), LOG_EE);

    if (sqlite3_exec(db, file_txids, NULL, NULL, &err_msg)) 
      log_err("Failed to create table file_txids: " + string(err_msg), LOG_EE);

    if (sqlite3_exec(db, insert, NULL, NULL, &err_msg)) 
      log_err("Failed to insert dummy: " + string(err_msg), LOG_EE);

  } else if (!access(path, R_OK | W_OK)) {

    if (sqlite3_open(path, &db)) 
      log_err("Can't open database: " + string(sqlite3_errmsg(db)), LOG_EE);

  } else {
    log_err("Failed to open / create database", LOG_EE);
  }

  /* delete finished uploads */
  string delete_finished = "DELETE FROM FILE_TXIDS WHERE file_id IN (SELECT "\
                           "id FROM MY_FILES WHERE status=" + \
                           itoa(FS_FINISHED) + "); "\
                           "DELETE FROM MY_FILES WHERE status=" + \
                           itoa(FS_FINISHED) + ";";
    
  char *err_msg = NULL;
  if (sqlite3_exec(db, delete_finished.c_str(), NULL, NULL, &err_msg)) 
    log_err("Failed to delete finished uploads: " + string(err_msg), LOG_W);

  pthread_create(&this->thread, NULL, db_thread, (void *) this);
}     

/* private destructor */
Database::~Database() {
  sqlite3_close(db);
}


/* returns the last parsed block */
unsigned Database::get_height() {

  char *select = "SELECT MAX(id_block) FROM TX;";
  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select, -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  unsigned height = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return height;
}

/* returns the last insert envelope id */
unsigned Database::get_max_envelope() {

  char *select = "SELECT MAX(id) FROM ENVELOPE;";
  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select, -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  unsigned height = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return height;
}

/* returns the last insert tx id */
unsigned Database::get_max_my_files() {

  char *select = "SELECT MAX(id) FROM MY_FILES;";
  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select, -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  unsigned height = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return height;
}

/* returns the status for the given file */
unsigned Database::get_file_status(string datahash) {

  string select = "SELECT status FROM MY_FILES WHERE datahash='" + datahash + "';";
  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  unsigned status = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return status;
}


/* returns the tx id for the given tx hash */
unsigned Database::get_tx_id(string txid) {

  string select = "SELECT id FROM TX WHERE txid='" + txid + "';";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  if (sqlite3_column_count(query) != 1) {
    log_err("Failed to select tx: " + txid + " sqlite column: " +
            itoa(sqlite3_column_count(query)), LOG_E);
    return 0;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }
  
  unsigned id = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return id;
}

/**
 * inserts an array of txids into the database 
 */
unsigned Database::db_insert(unsigned block, vector<string> txids) {

  unsigned txcount = 0;
  Rpc *rpc = Rpc::get_instance();
  char *err_msg = NULL;

  for (unsigned n = 0; n < txids.size(); n++) {
    
    string data = rpc->getdata(txids[n]);
    if (data.length() < 10)
      continue;

    bool is_envelope = false;
    string content = file_content(data);
    TX tx(testnet, block, true, txids[n], content, data);

    if (tx.is_envelope()) {

      /* get the public key of this */
      string basetxid  = txids[n];
      string publickey = "";
      
      /* publickey is only relevant above the limits */
      if ((testnet && block > DTC_TESTNET_LIMIT) ||
          (!testnet && block > DTC_LIMIT)) {

        if (tx.get_prevdatahash().length() == 64)
          basetxid = get_restricted_first_txid(tx.get_prevdatahash());


        publickey = (basetxid == txids[n]) ? tx.get_publickkey() : 
                                             get_tx(basetxid).get_publickkey();
      }

      if (tx.valid(publickey)) {
      
        is_envelope = true;
        Envelope msg = tx.get_envelope();
     
        if (msg.has_contenttype()) 
          content = msg.contenttype();
        else 
          content = file_content(msg.data());
        
     
        stringstream ss;
        ss << "INSERT INTO ENVELOPE (filename, compression, publickey,";
        ss << " signature, partnumber, totalparts, prevtxid, prevdatahash, ";
        ss << "datahash, datetime, version) VALUES (";
        ss << (msg.has_filename() ? "'" + msg.filename() + "'" : "NULL") << ", ";
        ss << (msg.has_compression() ? itoa(msg.compression()) : "NULL") << ", ";
        ss << (msg.has_publickey() ? "'" + to_b64(msg.publickey()) + "'" : "NULL") << ", ";
        ss << (msg.has_signature() ? "'" + to_b64(msg.signature()) + "'" : "NULL") << ", ";
        ss << (msg.has_partnumber() ? itoa(msg.partnumber()) : "NULL") << ", ";
        ss << (msg.has_totalparts() ? itoa(msg.totalparts()) : "NULL") << ", ";
        ss << (msg.has_prevtxid() ? "'" + msg.prevtxid() + "'" : "NULL") << ", ";
        ss << (msg.has_prevdatahash() ? "'" + msg.prevdatahash() + "'" : "NULL") << ", ";
        ss << "'" << tx.get_datahash() << "', ";
        ss << (msg.has_datetime() ? "'" + itoa(msg.datetime()) + "'" : "NULL") << ", ";
        ss << (msg.has_version() ? "'" + itoa(msg.version()) + "'" : "NULL") << ");";
        
        if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &err_msg)) {
          log_err("Failed to insert tx: " + string(err_msg) + "SQL: " + 
                  ss.str(), LOG_EE);
        }
       
        log_str("parsed " + itoa(data.length()) + " Envelope Bytes of Data type: " + string(content));
      } else
        log_str("invalid Envelope found:\n" + tx.to_string() + 
                "publickey:    " + publickey + "\nbasetxid:     " + basetxid, LOG_W);
    } else
      log_str("parsed " + itoa(data.length()) + " Bytes of Data type: " + string(content));
    
    
    stringstream ss;

    if (is_envelope) {
      ss << "INSERT INTO TX (id_block, id_envelope, txid, type) VALUES (";
      ss << block << ", " << get_max_envelope();
    } else
      ss << "INSERT INTO TX (id_block, txid, type) VALUES (" << block;

    ss << ", \"" << txids[n] << "\", \"" << content << "\");";

    if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &err_msg))
      log_err("Failed to insert tx: " + string(err_msg), LOG_EE);

    txcount++;
  }

  return txcount;
}
  

/* parse all unparsed blocks from datacoin */
void Database::parse_blocks() {

  Rpc *rpc = Rpc::get_instance();
  unsigned blockcount = rpc->getblockcount() - 4;
  unsigned height = get_height();
  static unsigned lastheight = 0;
  unsigned txcount = 0;

  if (blockcount <= lastheight)
    return;

  if (lastheight > height)
    height = lastheight;
  
  for (unsigned i = height + 1; i <= blockcount; i++) {

    log_str("Parsing block " + itoa(i) + " of " + itoa(blockcount) +
            "  -  tx: " + itoa(txcount));;
    vector<string> tx = rpc->getblocktransactions(i);

    txcount += db_insert(i, tx);  
  }
 lastheight = blockcount;

  if (blockcount - height)
    log_str("Parsed " + itoa(blockcount - height) + " blocks from the datacoin daemon");
}

/* parse the given blocks */
void Database::parse_specific_blocks(vector<unsigned> blocks) {

  Rpc *rpc = Rpc::get_instance();
  unsigned height = get_height();
  unsigned txcount = 0;
  
  for (unsigned i = 0; i < blocks.size(); i++) {
    
    if (blocks[i] <= height)
      continue;

    log_str("Parsing block " + itoa(i) + " of " + itoa(blocks.size()) +
            "  -  tx: " + itoa(txcount));;

    vector<string> tx = rpc->getblocktransactions(blocks[i]);
    txcount += db_insert(blocks[i], tx);
  }

  if (blocks.size())
    log_str("Parsed " + itoa(blocks.size()) + " blocks from the datacoin daemon"); 
}

/* run clean up */
void Database::cleanup() {
  pthread_mutex_lock(&creation_mutex);
  delete only_instance;
  initialized = false;
  pthread_mutex_unlock(&creation_mutex);
}


/* get a list with the different file types */
vector<string> Database::get_file_types() {

  vector<string> types;
  char *select = "SELECT DISTINCT(type) FROM TX;";
  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select, -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<string>();
  }

  for (int ret = sqlite3_step(query); 
       ret == SQLITE_ROW; 
       ret = sqlite3_step(query)) {
    
    types.push_back(sqlite3_column_string(query, 0));
  }
  sqlite3_finalize(query);

  return types;
}

/* get all tx with the given data type */
vector<tuple<string, string, string> > Database::get_tx_type(string type) {

  string select = "SELECT txid, filename, type, id_block FROM TX INNER JOIN " \
                  "ENVELOPE ON TX.id_envelope=ENVELOPE.id WHERE "\
                  "TX.type LIKE '" + type + "%' AND "\
                  "(ENVELOPE.prevtxid IS NULL OR ENVELOPE.prevtxid='') AND "    \
                  "(ENVELOPE.prevdatahash IS NULL OR ENVELOPE.prevdatahash='') "\
                  "UNION SELECT DISTINCT txid, NULL AS filename, type, id_block FROM TX " \
                  "WHERE type LIKE '" + type + "%' AND "\
                  "id_envelope IS NULL ORDER BY id_block DESC;";


  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E); 
    return vector<tuple<string, string, string> >();
  }
  vector<tuple<string, string, string> > txs;

  while (SQLITE_ROW == sqlite3_step(query)) {
    txs.push_back(make_tuple(sqlite3_column_string(query, 0),
                            sqlite3_column_string(query, 1),
                            sqlite3_column_string(query, 2)));
  }


  return txs;
}

/* get all unclassified tx type */
vector<tuple<string, string, string> > Database::get_tx_unclassified() {

  string select = "SELECT txid, filename, type, id_block FROM TX INNER JOIN " \
                  "ENVELOPE ON TX.id_envelope=ENVELOPE.id WHERE "\
                  "TX.type NOT LIKE 'text/plain%' AND "\
                  "TX.type NOT LIKE 'text/html%' AND "\
                  "TX.type NOT LIKE 'image/%' AND "\
                  "(ENVELOPE.prevtxid IS NULL OR ENVELOPE.prevtxid='') AND "    \
                  "(ENVELOPE.prevdatahash IS NULL OR ENVELOPE.prevdatahash='') "\
                  "UNION SELECT DISTINCT txid, NULL AS filename, type, id_block FROM TX " \
                  "WHERE type NOT LIKE 'text/plain%' AND "\
                  "type NOT LIKE 'text/html%' AND "\
                  "type NOT LIKE 'image/%' AND "\
                  "id_block != 0 AND "\
                  "id_envelope IS NULL ORDER BY id_block DESC;";



  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E); 
    return vector<tuple<string, string, string> >();
  }
  vector<tuple<string, string, string> > txs;

  while (SQLITE_ROW == sqlite3_step(query)) {
    txs.push_back(make_tuple(sqlite3_column_string(query, 0),
                            sqlite3_column_string(query, 1),
                            sqlite3_column_string(query, 2)));
  }

  return txs;
}

/* get the specific tx */
TX Database::get_tx(string txid) {

  string select = "SELECT id_block, id_envelope, type FROM TX WHERE txid='" + 
                  txid + "';";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return TX();
  }

  if (sqlite3_column_count(query) != 3) {
    log_err("Failed to select tx: " + txid + " sqlite column: " +
            itoa(sqlite3_column_count(query)), LOG_E);
    return TX();
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return TX();
  }

  TX tx(testnet,
        sqlite3_column_int(query, 0),         /* id_block    */
        sqlite3_column_int(query, 1),         /* id_envelope */
        txid,
        sqlite3_column_string(query, 2),      /* content     */
        Rpc::get_instance()->getdata(txid));

  sqlite3_finalize(query);

  return tx;
}

/* get all tx with data for the given block */
vector<string> Database::get_block_txs(unsigned block_id) {

  string select = "SELECT txid FROM TX WHERE id_block=" + itoa(block_id) + ";";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<string>();
  }
  vector<string> txs;

  while (SQLITE_ROW == sqlite3_step(query))
    txs.push_back(sqlite3_column_string(query, 0));

  return txs;
}

/* list all blocks with data txids */
vector<unsigned> Database::get_block_list() {

  string select = "SELECT DISTINCT id_block FROM TX;";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<unsigned>();
  }
  vector<unsigned> blocks;

  while (SQLITE_ROW == sqlite3_step(query))
    blocks.push_back(sqlite3_column_int(query, 0));

  return blocks;
}

/* inserts a new File int to the database */
bool Database::insert_file(vector<Envelope *> txids, string datahash) {
  
  stringstream ss;
  ss << "INSERT INTO MY_FILES (filename, datahash, content_type, compression, ";
  ss << "publickey, status) VALUES ('" << txids[0]->filename() << "', '";
  ss << datahash << "', '" << txids[0]->contenttype() << "', ";
  ss << txids[0]->compression() << ", '" << txids[0]->publickey();
  ss << "', " << FS_INIT << "); ";

  char *err_msg = NULL;
  if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &err_msg)) {
    log_err("Failed to insert file: " + string(err_msg), LOG_E);
    sqlite3_free(err_msg);
    return false;
  }
  ss.str("");

  unsigned file_id = get_max_my_files();
  
  for (unsigned i = 0; i < txids.size(); i++) {
    ss << "INSERT INTO FILE_TXIDS (file_id, partnumber, totalparts, ";
    ss << "signature, prevdatahash, datetime, version, data) VALUES (";
    ss << file_id << ", " << txids[i]->partnumber() << ", "; 
    ss << txids[i]->totalparts() << ", '" << txids[i]->signature() << "', '";
    ss << txids[i]->prevdatahash() << "', " << txids[i]->datetime() << ", ";
    ss << txids[i]->version() << ", '" << to_b64(txids[i]->data()) << "'); ";
  }

  if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &err_msg)) {
    log_err("Failed to insert file: " + string(err_msg), LOG_E);
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

/* update error, tx and prevtxid */
bool Database::update_file(unsigned partnumber, 
                           string datahash, 
                           string error, 
                           string txid) {

  string sql = "UPDATE FILE_TXIDS SET error='" + error + "', txid='" + txid + \
               "' WHERE partnumber=" + itoa(partnumber) + " AND file_id IN "  \
               "(SELECT id FROM MY_FILES WHERE datahash='" + datahash + "');";

  char *err_msg = NULL;
  if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &err_msg)) {
    log_err("Failed to update FILE_TXIDS: " + string(err_msg) + "sql: " + sql, LOG_E);
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

/* update file status */
bool Database::update_file_status(string datahash, int status) {

  string sql = "UPDATE MY_FILES SET status='" + itoa(status) + "' "        \
               "WHERE datahash='" + datahash + "';";

  char *err_msg = NULL;
  if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &err_msg)) {
    log_err("Failed to update MY_FILES: " + string(err_msg), LOG_E);
    sqlite3_free(err_msg);
    return false;
  }

  /* delete finished uploads */
  string delete_finished = "DELETE FROM FILE_TXIDS WHERE file_id IN (SELECT "\
                           "id FROM MY_FILES WHERE status=" + \
                           itoa(FS_FINISHED) + "); "\
                           "DELETE FROM MY_FILES WHERE status=" + \
                           itoa(FS_FINISHED) + ";";
    
  if (sqlite3_exec(db, delete_finished.c_str(), NULL, NULL, &err_msg)) 
    log_err("Failed to delete finished uploads: " + string(err_msg), LOG_W);


  return true;
}

/* returns whether the File with the given Hash exists */
bool Database::file_exists(string datahash) {
  
  string select = "SELECT COUNT(*) FROM MY_FILES WHERE datahash='" + datahash + \
                  "';";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }

  unsigned count = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return count > 0;
}

/* reads all txids of the given hash */
vector<pair<Envelope *, pair<string, string> > > Database::get_file(string datahash) {
  
  string select = "SELECT id, filename, content_type, compression, publickey " \
                  "FROM MY_FILES WHERE datahash='" + datahash + "';";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<pair<Envelope *, pair<string, string> > >();
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<pair<Envelope *, pair<string, string> > >();
  }

  Envelope *first = new Envelope;

  unsigned file_id = sqlite3_column_int(query, 0);
  first->set_filename(sqlite3_column_string(query, 1));
  first->set_contenttype(sqlite3_column_string(query, 2));

  int compression = sqlite3_column_int(query, 3);
  if (compression == ENV_BZIP2)
    first->set_compression(Envelope::Bzip2);
  else if (compression == ENV_XZ)
    first->set_compression(Envelope::Xz);
  else
    first->set_compression(Envelope::None);

  first->set_publickey(sqlite3_column_string(query, 4));
  sqlite3_finalize(query);
  query = NULL;

  
  select = "SELECT partnumber, totalparts, signature, prevdatahash, datetime, "\
           "version, data, txid, error FROM FILE_TXIDS WHERE file_id=" +       \
           itoa(file_id) + " ORDER BY partnumber ASC;";

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<pair<Envelope *, pair<string, string> > >();
  }

  if (sqlite3_column_count(query) != 9) {
    log_err("Failed to select tx: " + datahash + " sqlite column: " +
            itoa(sqlite3_column_count(query)), LOG_E);
    return vector<pair<Envelope *, pair<string, string> > >();
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return vector<pair<Envelope *, pair<string, string> > >();
  }

  first->set_partnumber(sqlite3_column_int(query, 0));
  first->set_totalparts(sqlite3_column_int(query, 1));
  first->set_signature(sqlite3_column_string(query, 2));
  first->set_prevdatahash(sqlite3_column_string(query, 3));
  first->set_datetime(sqlite3_column_int(query, 4));
  first->set_version(sqlite3_column_int(query, 5));
  first->set_data(b64_to_byte(sqlite3_column_string(query, 6)));

  string txid = sqlite3_column_string(query, 7);
  string error = sqlite3_column_string(query, 8);

  vector<pair<Envelope *, pair<string, string> > > txids;
  txids.push_back(make_pair(first, make_pair(txid, error)));

  while (SQLITE_ROW == sqlite3_step(query)) {
    Envelope *env = new Envelope;  

    env->set_compression(first->compression());
    env->set_partnumber(sqlite3_column_int(query, 0));
    env->set_totalparts(sqlite3_column_int(query, 1));
    env->set_signature(sqlite3_column_string(query, 2));
    env->set_prevdatahash(sqlite3_column_string(query, 3));
    env->set_datetime(sqlite3_column_int(query, 4));
    env->set_version(sqlite3_column_int(query, 5));
    env->set_data(b64_to_byte(sqlite3_column_string(query, 6)));
 
    string txid = sqlite3_column_string(query, 7);
    string error = sqlite3_column_string(query, 8);
 
    txids.push_back(make_pair(env, make_pair(txid, error)));
  }

  return txids;
}

/* returns the txid of the envelope part for the given envelope txid */
string Database::get_unchecked_first_txid(string txid) {
  

  string prevtxid = txid;
  int id_envelope = 0;
  do {

    sqlite3_stmt* query = NULL;
    string select = "SELECT ENVELOPE.prevtxid, TX.id_envelope FROM ENVELOPE INNER JOIN "  \
                    "TX ON TX.id_envelope=ENVELOPE.id WHERE "                 \
                    "TX.txid='" + prevtxid + "';";
 
    if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
      log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
      return "";
    }
 
    if (SQLITE_ROW != sqlite3_step(query)) {
      log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
      return "";
    }
 
    prevtxid    = sqlite3_column_string(query, 0);
    id_envelope = sqlite3_column_int(query, 1);
    sqlite3_finalize(query);
 
  } while (prevtxid != "");

  sqlite3_stmt* query = NULL;
  string select = "SELECT txid FROM TX WHERE id_envelope=" + 
                  itoa(id_envelope) + ";";
 
  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  txid = sqlite3_column_string(query, 0);
  sqlite3_finalize(query);

  return txid;
}

/* returns the txid of the first part for the given envelope datahash */
string Database::get_restricted_first_txid(string datahash) {
  

  string prevdatahash = datahash;
  int id_envelope = 0;
  do {

    sqlite3_stmt* query = NULL;
    string select = "SELECT ENVELOPE.prevdatahash, TX.id_envelope FROM ENVELOPE INNER JOIN "  \
                    "TX ON TX.id_envelope=ENVELOPE.id WHERE "                 \
                    "ENVELOPE.datahash='" + prevdatahash + "';";
 
    if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
      log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
      return "";
    }
 
    if (SQLITE_ROW != sqlite3_step(query)) {
      log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
      return "";
    }
 
    prevdatahash = sqlite3_column_string(query, 0);
    id_envelope  = sqlite3_column_int(query, 1);
    sqlite3_finalize(query);
 
  } while (prevdatahash != "");

  sqlite3_stmt* query = NULL;
  string select = "SELECT txid FROM TX WHERE id_envelope=" + 
                  itoa(id_envelope) + ";";
 
  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  string txid = sqlite3_column_string(query, 0);
  sqlite3_finalize(query);

  return txid;
}

/* returns the id_block for the given txid */
unsigned Database::get_id_block(string txid) {

  string select = "SELECT id_block FROM TX WHERE txid='" + txid + "';";
 
  sqlite3_stmt* query = NULL;
  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }
 
  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }
 
  unsigned id_block = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return id_block;
}

/* returns the datetime for the given Envelope txid */
time_t Database::get_datetime(string txid) {

  string select = "SELECT ENVELOPE.datetime FROM ENVELOPE INNER JOIN TX ON " \
                  "ENVELOPE.id=TX.id_envelope WHERE TX.txid='" + txid + "';";
 
  sqlite3_stmt* query = NULL;
  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }
 
  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return 0;
  }
 
  time_t datetime = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return datetime;
}

/* returns the datahash for the given Envelope txid */
string Database::get_datahash(string txid) {

  string select = "SELECT ENVELOPE.datahash FROM ENVELOPE INNER JOIN TX ON " \
                  "ENVELOPE.id=TX.id_envelope WHERE TX.txid='" + txid + "';";
 
  sqlite3_stmt* query = NULL;
  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  string datahash = sqlite3_column_string(query, 0);
  sqlite3_finalize(query);

  return datahash;
}

/* returns the newest verified start tx for the given envelope tx */
string Database::get_unchecked_newest_txid(string txid) {
  
  txid = get_unchecked_first_txid(txid);
  if (txid == "") return "";

  /* workaround only for testnet */ // TODO remove this method
  if (!testnet) return txid;


  sqlite3_stmt* query = NULL;
    
  for (;;) {
    string select = "SELECT TX.txid, TX.id_block FROM TX INNER JOIN "         \
                    "ENVELOPE ON TX.id_envelope=ENVELOPE.id WHERE "           \
                    "ENVELOPE.partnumber=1 AND ENVELOPE.prevtxid='" +         \
                    txid + "' ORDER BY TX.id_block DESC;";

    if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
      log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
      return "";
    }
 
    string nexttxid = "";
    while (SQLITE_ROW == sqlite3_step(query) && nexttxid == "") {
      
      if ((testnet && sqlite3_column_int(query, 1) > DTC_TESTNET_LIMIT) ||
          (!testnet && sqlite3_column_int(query, 1) > DTC_LIMIT)) {
        
        continue;
      }
      
      nexttxid = sqlite3_column_string(query, 0);
    }
 
    if (nexttxid == "") break;
    else txid = nexttxid;
  }
  return txid;
}

/* returns the newest verified start tx for the given envelope tx */
string Database::get_restricted_newest_txid(string txid) {
  
  txid = get_restricted_first_txid(get_datahash(txid));
  if (txid == "") return "";

  time_t datetime  = get_datetime(txid);
  string datahash  = get_datahash(txid);
  if (datahash == "" || datetime == 0) return "";

  sqlite3_stmt* query = NULL;
    
  string select = "SELECT TX.txid FROM TX INNER JOIN ENVELOPE ON "          \
                  "TX.id_envelope=ENVELOPE.id WHERE ENVELOPE.partnumber=1 " \
                  "AND ENVELOPE.version=2 "                                 \
                  "AND ENVELOPE.datetime > " + itoa(datetime)  + " "        \
                  "AND ENVELOPE.prevdatahash='" + datahash + "' ORDER BY "          \
                  "TX.id_block DESC;";

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }
 
  if (SQLITE_ROW != sqlite3_step(query)) {
    return txid;
  }

  string nexttxid = sqlite3_column_string(query, 0);
  sqlite3_finalize(query);
  
  return nexttxid;
}

/* returns the reassembled TX for a Envelope txid */
TX Database::reasemble_envelope(string txid) {
  
  if ((testnet && get_id_block(txid) > DTC_TESTNET_LIMIT) ||
      (!testnet && get_id_block(txid) > DTC_LIMIT)) {

    return restricted_reasemble_envelope(txid);
  } else {
    return unchecked_reasemble_envelope(txid);
  }
}

/* returns the reassembled TX for a Envelope txid */
TX Database::unchecked_reasemble_envelope(string txid) {
  
  txid = get_unchecked_newest_txid(txid);
  if (txid == "") return TX();

  TX base          = get_tx(txid);

  string nexttxid = txid;
  log_str("Envelope reassemble part 1: " + nexttxid);
  for (unsigned i = 2; nexttxid != ""; i++) {

    string select = "SELECT TX.txid, TX.id_block FROM "                       \
                    "TX INNER JOIN ENVELOPE ON TX.id_envelope=ENVELOPE.id "   \
                    "WHERE ENVELOPE.prevtxid='" + nexttxid + "' ORDER BY "    \
                    "TX.id_block DESC;";

    sqlite3_stmt* query = NULL;
    if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
      log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
      return TX();
    }
 
    nexttxid = "";
    TX next = TX();
    while (SQLITE_ROW == sqlite3_step(query) && next.empty()) {

      if ((testnet && sqlite3_column_int(query, 1) > DTC_TESTNET_LIMIT) ||
          (!testnet && sqlite3_column_int(query, 1) > DTC_LIMIT)) {
        
        continue;
      }

      nexttxid = sqlite3_column_string(query, 0);
      next     = get_tx(nexttxid);

      if (!next.is_envelope()) {
         log_err("invalid Envelope part " + itoa(i) + " found", LOG_W);
         nexttxid = "";
         next = TX();
      }
    }
    
    if (!next.empty()) {
      log_str("Envelope reassemble part " + itoa(i) + ": " + nexttxid);
      base.add_data(next.get_data());
    }
  }
  

  if (base.get_compression() == ENV_BZIP2) {
    string decompressed = bzip2_decompress(base.get_data());
    base.set_data(decompressed);
  }

  return base;
}


/* returns the reassembled TX for a Envelope txid */
TX Database::restricted_reasemble_envelope(string txid) {
  
  txid = get_restricted_newest_txid(txid);
  if (txid == "") return TX();

  TX base          = get_tx(txid);
  time_t datetime  = base.get_datetime();
  string datahash  = get_datahash(txid);
  string nexttxid  = txid;

  log_str("Envelope reassemble part 1: " + txid);
  for (unsigned i = 2; nexttxid != ""; i++) {

    string select = "SELECT TX.txid FROM "   \
                    "TX INNER JOIN ENVELOPE ON TX.id_envelope=ENVELOPE.id "   \
                    "WHERE ENVELOPE.partnumber=" + itoa(i) + " AND "          \
                    "ENVELOPE.version=2 AND "                                 \
                    "ENVELOPE.datetime > " + itoa(datetime) + " AND "         \
                    "ENVELOPE.prevdatahash='" + datahash + "' ORDER BY "      \
                    "TX.id_block DESC;";

    sqlite3_stmt* query = NULL;
    if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
      log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
      return TX();
    }
 
    TX next = TX();
    nexttxid = "";
    while (SQLITE_ROW == sqlite3_step(query) && next.empty()) {
      nexttxid = sqlite3_column_string(query, 0);
      next     = get_tx(nexttxid);

      if (!next.is_envelope()) {
         log_err("invalid Envelope part " + itoa(i) + " found", LOG_W);
         next     = TX();
         nexttxid = "";
      }
    }
    
    if (!next.empty()) {
      log_str("Envelope reassemble part " + itoa(i) + ": " + nexttxid);
      base.add_data(next.get_data());
      datahash = get_datahash(nexttxid);
      datetime = next.get_datetime();
    }
  }
  

  if (base.get_compression() == ENV_BZIP2) {
    string decompressed = bzip2_decompress(base.get_data());
    base.set_data(decompressed);
  }

  return base;
}


/* returns whether the given txid is an Envelope */
bool Database::is_envelope(string txid) {

  string select = "SELECT id_envelope FROM TX WHERE txid='" + txid + "';";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return false;
  }

  if (SQLITE_ROW != sqlite3_step(query))
    return false;

  unsigned id_envelope = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  return id_envelope > 0;

}

/* returns the txid for a given datahash */
string Database::gettxid_by_datahash(string datahash) {

  string select = "SELECT txid FROM TX INNER JOIN ENVELOPE ON TX.id_envelope="\
                  "ENVELOPE.id WHERE ENVELOPE.datahash='" + datahash + "';";

  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to step: " + string(sqlite3_errmsg(db)), LOG_E);
    return "";
  }

  string txid = sqlite3_column_string(query, 0);
  sqlite3_finalize(query);

  return txid;
}

/* creat dummy data */
#ifdef DEBUG

#define randr(rand, start, end) ((rand32(rand) % (end - start)) + start)

static inline string rand_str(uint32_t *rand, unsigned len) {
  
  static const string base = "1234567890qwertzuiop+asdfghjkl#yxcvbnm,.-<"\
                            "!§$%&/()=?QWERTZUIOP*ASDFGHJKLYXCVBNM;:_>";
  static const unsigned size = base.length();

  stringstream ss;

  for (unsigned i = 0; i < len; i++)
    ss << base[rand32(rand) % size];

  return ss.str();
}

void Database::create_dummy_data(uint64_t blocks, unsigned block_txids) {
  
  unsigned height = get_height();
  uint32_t rand = time(NULL);
  char *err_msg = NULL;

  for (uint64_t i = height + 1; i < (blocks + height); i++) {
    
    for (unsigned n = 0; n < block_txids; n++) {

      stringstream ss;
      ss << "INSERT INTO ENVELOPE (filename, compression, publickey,";
      ss << " signature, partnumber, totalparts, prevtxid, prevdatahash, ";
      ss << "datahash, datetime, version) VALUES (";
      ss << "\"" << rand_str(&rand, randr(&rand, 4, 60)) << "\", ";
      ss << rand32(&rand) % 3 << ", ";
      ss << "\"" << rand_str(&rand, 34) << "\", ";
      ss << "\"" << rand_str(&rand, 88) << "\", ";
      ss << rand32(&rand) % 10 << ", ";
      ss << rand32(&rand) % 10 << ", ";
      ss << "\"" << rand_str(&rand, 64) << "\", ";
      ss << "\"" << rand_str(&rand, 64) << "\", ";
      ss << "\"" << rand_str(&rand, 64) << "\", ";
      ss << rand32(&rand) << ", 2);";
      
      if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &err_msg)) {
        log_err("Failed to insert tx: " + string(err_msg) + "SQL: " + 
                ss.str(), LOG_EE);
      }
      
        
      ss.str("");
      ss << "INSERT INTO TX (id_block, id_envelope, txid, type) VALUES (";
      ss << i << ", " << get_max_envelope();
  
      ss << ", \"" <<  rand_str(&rand, 64) << "\", \"" << rand_str(&rand, randr(&rand, 10, 60)) << "\");";
  
      if (sqlite3_exec(db, ss.str().c_str(), NULL, NULL, &err_msg))
        log_err("Failed to insert tx: " + string(err_msg), LOG_EE);

    }
    log_str("creating dummy data: " + itoa(i - height) + " / " + itoa(blocks));
  }

  string select = "SELECT count(*) FROM TX;";
  sqlite3_stmt* query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return;
  }

  unsigned tx_count = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);


  select = "SELECT count(*) FROM ENVELOPE;";
  query = NULL;

  if (sqlite3_prepare_v2(db, select.c_str(), -1, &query, NULL) != SQLITE_OK) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return;
  }

  if (SQLITE_ROW != sqlite3_step(query)) {
    log_err("Failed to prepare select: " + string(sqlite3_errmsg(db)), LOG_E);
    return;
  }

  unsigned envelope_count = sqlite3_column_int(query, 0);
  sqlite3_finalize(query);

  log_str("Insertetd: " + itoa(blocks) + " with " + itoa(block_txids) + 
          " dummy txids each:\n" + "TX count: " + itoa(tx_count) + 
          "\nENVELOPE count: " + itoa(envelope_count));
}
#endif
