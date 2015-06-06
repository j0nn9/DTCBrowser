/**
 * Header of the Datacoin Browser's transaction representation class
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
#ifndef __TX_H__
#define __TX_H__
#include "envelope.pb.h"

using namespace std;

#define ENV_NONE  0
#define ENV_BZIP2 1
#define ENV_XZ    2

#define DTC_TESTNET_LIMIT 47000
#define DTC_LIMIT         800000

class TX {
  
  private:

    /* indicates that we are running in testnet mode */
    bool testnet;

    /* the block id of this */
    unsigned id_block;

    /* the txid of this */
    string txid;

    /* content type of this */
    string content;

    /* the data of this (only set if this is no Envelope) */
    string data;

    /* the Envelope of this (if this is one) */
    Envelope env;

  public :
    
    TX();
    
    TX(bool testnet,
       unsigned id_block, 
       bool parse_envelope, 
       string txid, 
       string content,
       string data);

    /* getter methods */
    unsigned get_id_block();
    string   get_txid();
    string   get_content();
    Envelope get_envelope();
    string   get_publickkey();
    string   get_prevdatahash();
    string   get_data();
    void     add_data(string data);
    void     set_data(string data);
    unsigned get_compression();
    time_t   get_datetime();

    /* returns if this is an empty TX */
    bool empty();

    /* returns whether this is a Envelope TXID */
    bool is_envelope();

    /* returns the datahash of this */
    string get_datahash();
    static string get_datahash(Envelope *env);

    /* returns whether this is a valid TX */
    bool valid(string publickey = "");
    static bool valid(Envelope *env, string publickey = "", string txid = "");

    /* returns a string describing this */
    string to_string();
    
};

#endif /* __TX_H__ */
