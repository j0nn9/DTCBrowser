/**
 * Header of the Datacoin Browser's database Back-End
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
#ifndef __DATABASE_H__
#define __DATABASE_H__
#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include "envelope.pb.h"
#include "TX.h"
#include <tuple>
#include <functional>

using namespace std;

/* file status */
#define FS_INIT 1
#define FS_SENDING 2
#define FS_SENDED 3
#define FS_FINISHED 4

class Database {
  
  private:
    
    /* the database object of this */
    sqlite3* db;

    /* this is a singleton */
    static Database *only_instance;

    /* synchronization mutexes */
    static pthread_mutex_t creation_mutex;

    /* all currently known testnet data blocks */
    static unsigned testnet_data_blocks[];

    /* all currently known data blocks */
    static unsigned data_blocks[];

    /* indicates if this was initialized */
    static bool initialized;

    /* scan fast or slow */
    bool scann_fast;

    /* indicates that this should continue running */
    bool running;

    /* indicates we running testnet */
    bool testnet;

    /* database thread object */
    pthread_t thread;

    /* private constructor to allow only one instance of this */
    Database(const char *path, bool scann_fast, bool testnet);

    /* private destructor */
    ~Database();

    /* the database refresh thread */
    static void *db_thread(void *);

    /**
     * inserts an array of txids into the database 
     */
    unsigned db_insert(unsigned block, vector<string> txids);

    /* returns the last insert envelope id */
    unsigned get_max_envelope();

    /* returns the last insert txid */
    unsigned get_max_my_files();

    /* returns the txid for the given tx hash */
    unsigned get_tx_id(string hash);

  public:
    
    /* return the only instance of this */
    static Database *get_instance(const char *path = NULL, 
                                  bool scann_fast = true, 
                                  bool testnet = false);

    /* returns the last parsed block */
    unsigned get_height();

    /* parse all unparsed blocks from datacoin */
    void parse_blocks();

    /* parse the given blocks */
    void parse_specific_blocks(vector<unsigned> blocks);

    /* run clean up */
    static void cleanup();

    /* get a list with the different file types */
    vector<string> get_file_types();

    /* get all tx that are like type* */
    vector<tuple<string, string, string> > get_tx_type(string type);

    /* get all unclassified tx type */
    vector<tuple<string, string, string> > get_tx_unclassified();

    /* get the specific tx */
    TX get_tx(string hash);

    /* get all tx with data for the given block */
    vector<string> get_block_txs(unsigned block_id);

    /* list all blocks with data txids */
    vector<unsigned> get_block_list();

    /* update error, tx and prevtxid */
    bool update_file(unsigned partnumber, 
                     string datahash, 
                     string error, 
                     string txid);

    /* returns the status for the given file */
    unsigned get_file_status(string datahash);

    /* update file status */
    bool update_file_status(string datahash, int status);

    /* inserts a new File int to the database */
    bool insert_file(vector<Envelope *> txids, string datahash);

    /* returns whether the File with the given Hash exists */
    bool file_exists(string datahash);

    /* reads all txids of the given hash */
    vector<pair<Envelope *, pair<string, string> > > get_file(string datahash);

    /* returns the txid of the first part for the given envelope txid 
     * if parse == true then txid doesn't have to be in the database 
     */
    string get_unchecked_first_txid(string txid);
    string get_restricted_first_txid(string txid);

    /* returns the id_block for the given txid */
    unsigned get_id_block(string txid);

    /* returns the datetime for the given Envelope txid */
    time_t get_datetime(string txid);

    /* returns the datahash for the given Envelope txid */
    string get_datahash(string txid);
      
    /* returns the newest verified start tx for the given envelope tx */
    string get_unchecked_newest_txid(string txid);
    string get_restricted_newest_txid(string txid);

    /* returns the reassembled TX for a Envelope txid */
    TX reasemble_envelope(string txid);
    TX unchecked_reasemble_envelope(string txid);
    TX restricted_reasemble_envelope(string txid);

    /* returns whether the given txid is an Envelope */
    bool is_envelope(string txid);
          
    /* returns the txid for a given datahash */
    string gettxid_by_datahash(string datahash);

    #ifdef DEBUG
    void create_dummy_data(uint64_t blocks, unsigned block_txids);
    #endif
};
#endif /* __DATABASE_H__ */
