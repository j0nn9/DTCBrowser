/**
 * Header of the Datacoin Browser's RPC Back-End
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
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;
#define USER_AGENT "DataCoinHttpServer"

class Rpc {

  public:

    /* return the only instance of this */
    static Rpc *get_instance();

    /* run clean up */
    static void cleanup();

    /**
     * initialize curl with the given 
     * username, password, url, and port
     */
    static bool init_curl(string userpass, string url, int timeout = 60);

    /* returns the number of blocks */
    unsigned getblockcount();

    /* returns the hash of the block with the given index */
    string getblockhash(unsigned height);

    /* returns a vector with all transaction hashes 
     * from a block with the given index */
    vector<string> getblocktransactions(unsigned height);

    /* returns the base64 encoded transaction data */
    string getdata(string txhash);

    /* returns the transaction hash or error for the sended data */
    pair<bool, string> senddata(string data);
    
    /* signs the given message with the given address */
    string signmessage(string address, string message);

    /* verifies the given message */
    bool verifymessage(string address, string signature, string message);

    /* returns the number of confirmations for a given transaction */
    int tx_confirmations(string txid);

    /* returns a list of accounts */
    vector<string> listaccounts();

    /* returns all addresses of all accounts */
    vector<string> listaddresses();

    /* returns the json server answer for getinfo */
    string getinfo_json();

    /* returns the json server answer for getmininginfo */
    string getmininginfo_json();

    /* returns the json server answer for getblockhash */
    string getblockhash_json(unsigned height);

    /* returns the json server answer for getblockhash */
    string getblock_json(string hash);

    /* returns the json server answer for getrawtransaction */
    string getrawtransaction_json(string txid);

  private:

    /* curl receive session handle */
    static CURL *curl_recv;

    /* curl send session handle */
    static CURL *curl_send;


    /* private constructor to allow only one instance */
    Rpc();

    /* private destructor */
    ~Rpc();

    /* synchronization mutexes */
    static pthread_mutex_t send_mutex;       
    static pthread_mutex_t creation_mutex;

    /* this will be a singleton */
    static Rpc *only_instance;

    /* indicates if this was initialized */
    static bool initialized;

    /* indicates if curl was initialized */
    static bool curl_initialized;

    /* rpc timeout */
    static int timeout;

    /* string stream for receiving */
    static stringstream *recv_ss;
    
    /* call the specific rpc method with the given args */
    string call_rpc(string method, string params);
};
