/**
 * Implementation of the Datacoin Browser's transaction representation class
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
#include "TX.h"
#include "utils.h"
#include "Rpc.h"

TX::TX() {
  this->id_block = 0;
  this->txid = "";
  this->content = "";
  this->data = "";
}
  
TX::TX(bool testnet,
       unsigned id_block, 
       bool parse_envelope, 
       string txid, 
       string content,
       string data) {

  this->testnet     = testnet;
  this->id_block    = id_block;
  this->txid        = txid;
  this->content     = content;
  this->data        = "";
  
  if (parse_envelope) {
    try {
      if (!env.ParseFromString(data)) 
        log_err("Parse Tx Envelope failed", LOG_W);
    } catch (...) {
      log_err("Parse Tx Envelope failed", LOG_W);
    }
  } else
    this->data = data;
}

/* getter methods */
unsigned TX::get_id_block()     { return id_block;    }
string   TX::get_txid()         { return txid;        }
string   TX::get_content()      { return content;     }
Envelope TX::get_envelope()     { return env;         }

string   TX::get_publickkey()   { 
  return (env.IsInitialized() ? env.publickey() : "");
}

string   TX::get_prevdatahash()   { 
  return (env.IsInitialized() ? env.prevdatahash() : "");
}

string   TX::get_data()         { 
  return (env.IsInitialized() ? env.data() : data); 
}

void     TX::add_data(string data) {
  if (env.IsInitialized())
    env.set_data(env.data() + data);
}

void     TX::set_data(string data) {
  if (env.IsInitialized())
    env.set_data(data);
}

unsigned TX::get_compression()  {
  
  if (env.compression() == Envelope::Bzip2)
    return ENV_BZIP2;
  else if (env.compression() == Envelope::Xz)
    return ENV_XZ;
  else
    return ENV_NONE;
}

time_t TX::get_datetime() {
  return (env.IsInitialized() ? env.datetime() : 0);
}

/* returns if this is an empty TX */
bool TX::empty() {
  return (id_block == 0 && txid == "");
}

/* returns whether this is a Envelope TXID */
bool TX::is_envelope() {
  return env.IsInitialized();
}

/* returns the datahash of this */
string TX::get_datahash() {

  if ((testnet  && id_block < DTC_TESTNET_LIMIT) ||
      (!testnet && id_block < DTC_LIMIT))
    return sha256sum(env.data());
  
  if (!is_envelope())
    return sha256sum(get_data());

  return get_datahash(&env);
}

string TX::get_datahash(Envelope *env) {

  if (env->version() != 2)
    return sha256sum(env->data());

  /* create the hash sum over every entry except the signature */
  string hash_data =  env->filename()          +
                      env->contenttype()       +
                      itoa(env->compression()) +
                      env->publickey()         +
                      itoa(env->partnumber())  +
                      itoa(env->totalparts())  +
                      env->prevtxid()          +
                      env->prevdatahash()      +
                      itoa(env->datetime())    +
                      itoa(env->version())     +
                      env->data();
                     
  return sha256sum(hash_data);
}

/* returns whether this is a valid TX */
bool TX::valid(string publickey) {

  if (empty() || !env.IsInitialized())
    return false;

  /* don't validate old envelopes */
  if ((testnet  && id_block < DTC_TESTNET_LIMIT) ||
      (!testnet && id_block < DTC_LIMIT))
    return true;

  return valid(&env, publickey, txid);
}

bool TX::valid(Envelope *env, string publickey, string txid) {
  
  Rpc *rpc = Rpc::get_instance();
    
  if (!rpc->verifymessage(publickey, env->signature(), get_datahash(env))) {
    log_err("invalid Envelope found: " + txid + " (" + 
            publickey + ", " + env->signature() + ", " + 
            get_datahash(env) + ")", LOG_W);

    return false;
  }

  return true;
}


/* returns a string describing this */
string TX::to_string() {
  
  stringstream ss;
  ss << "TDIX:      " << txid << endl;
  ss << "id_block:  " << id_block << endl;
  ss << "content:   " << content << endl;
  
  if (env.IsInitialized()) {
    ss << "filename:     " <<  env.filename() << endl;
    ss << "compression:  " << get_compression() << endl;
    ss << "publickey:    " << env.publickey() << endl;
    ss << "signature:    " << env.signature() << endl;
    ss << "partnumber:   " << env.partnumber() << endl;
    ss << "totalparts:   " << env.totalparts() << endl;
    ss << "prevtxid:     " << env.prevtxid() << endl;
    ss << "datahash:     " << get_datahash() << endl;
    ss << "prevdatahash: " << env.prevdatahash() << endl;
    ss << "datetime:     " << env.datetime() << endl;
    ss << "version:      " << env.version() << endl;
  }

  return ss.str();
}
