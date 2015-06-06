/**
 * Implementation of the Datacoin Browser's file uploading capability
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
#include "StoreFileProcess.h"
#include "Rpc.h"
#include "utils.h"
#include "envelope.pb.h"
#include <unistd.h>
#include <time.h>


/* load this from Database */
void StoreFileProcess::load() {
  
  vector<pair<Envelope *, pair<string, string> > > txids = db->get_file(sha256);

  if (txids.empty())
    throw "failed to load file from database";

  for (unsigned i = 0; i < txids.size(); i++) {
    envs.push_back(txids[i].first);
    this->txids.push_back(txids[i].second.first);
    this->errors.push_back(txids[i].second.second);
  }

  if (envs[0]->prevdatahash().length() == 64)
    update_datahash = envs[0]->prevdatahash();
  else
    update_datahash = "";
}

/**
 * initialize an StoreFileProcess
 * NOTE: compression has to be applied before 
 */
StoreFileProcess::StoreFileProcess(string file, 
                                   string sha256,
                                   string filename, 
                                   string content_type,
                                   unsigned compression,
                                   string dtc_address,
                                   unsigned confirmations,
                                   string updatetxid) {

  this->sha256 = sha256;
  this->confirmations = confirmations;
  this->db = Database::get_instance();

  if (updatetxid.length() == 64) {
    string basetxid = db->get_restricted_first_txid(db->get_datahash(updatetxid));

    if (basetxid.length() != 64)
      throw "updatetxid not found";

    TX base = db->get_tx(updatetxid);

    if (base.empty())
      throw "updatetxid not found";

    dtc_address     = base.get_publickkey();
    update_datahash = db->get_datahash(basetxid);

    if (update_datahash.length() != 64)
      throw "update datahash not found";
  }

  if (db->file_exists(sha256)) { 
    load();
  } else {
    
    /* 128KB datalimit is measured on the b64 encoded data */
    const unsigned part_len = 95*1024;
    unsigned parts          = (file.length() + part_len - 1) / part_len;
    time_t datetime         = time(NULL);
   
    for (unsigned i = 0; i < parts; i++) {
    
      Envelope *env = new Envelope();
      env->set_partnumber(i + 1);
      env->set_totalparts(parts);
      env->set_datetime(datetime + i);
      env->set_version(2);
   
      env->set_data(string(file.c_str() + i * part_len, 
                   (((i + 1) * part_len < file.length()) ? part_len : 
                                          file.length() - i * part_len)));

      if (compression == ENV_BZIP2)
        env->set_compression(Envelope::Bzip2);
      else if (compression == ENV_XZ)
        env->set_compression(Envelope::Xz);
      else
        env->set_compression(Envelope::None);

      
      if (i == 0) {
        env->set_filename(filename);
        env->set_contenttype(content_type);

        if (update_datahash.length() != 64)
          env->set_publickey(dtc_address);
        else
          env->set_prevdatahash(update_datahash);

      } else {
        env->set_prevdatahash(TX::get_datahash(envs.back()));
      }
   
      Rpc *rpc = Rpc::get_instance();
      string signature = rpc->signmessage(dtc_address, TX::get_datahash(env));
      if (signature.length() != 88)
        throw "Failed to create signature, is wallet unlocked?";

      env->set_signature(signature);

      envs.push_back(env);
      txids.push_back("");
      errors.push_back("");

      if (!TX::valid(env, dtc_address))
        throw "Failed to verify signature";
    }
   
    db->insert_file(envs, sha256);
  }
  pthread_create(&thread, NULL, process, (void *) this);
}

StoreFileProcess::~StoreFileProcess() {
  
  pthread_join(thread, NULL);
  for (unsigned i = 0; i < envs.size(); i++)
    delete envs[i];
}

/**
 * initialize an StoreFileProcess
 * NOTE: compression has to be applied before 
 */
StoreFileProcess::StoreFileProcess(string sha256,
                                   unsigned confirmations) {

  this->sha256 = sha256;
  this->confirmations = confirmations;
  this->db = Database::get_instance();
  if (db->file_exists(sha256)) { 
    load();
  } else {
    throw "Failed to load file (not found)";
  }
  pthread_create(&thread, NULL, process, (void *) this);
}

/* processes the envelopes */
void *StoreFileProcess::process(void *args) {
  
  StoreFileProcess *self = (StoreFileProcess *) args;
  Rpc *rpc = Rpc::get_instance();
  Database *db = Database::get_instance();

  string publickey  = self->envs[0]->publickey();
  string updatetxid = "";

  if (self->update_datahash.length() == 64) {
    string basetxid = db->get_restricted_first_txid(self->update_datahash);
    TX base         = db->get_tx(basetxid);
    updatetxid      = base.get_txid();
    publickey       = base.get_publickkey();
  }


  if (db->get_file_status(self->sha256) == FS_INIT)
    db->update_file_status(self->sha256, FS_SENDING); 
  
  if (db->get_file_status(self->sha256) == FS_SENDING) {
    for (unsigned i = 0; i < self->envs.size(); i++) {

      /* skip to start if an error occurred */
      for (; i > 0 && self->txids[i - 1].size() != 64; i--)
        sleep(1);
      
      /* skip existing */
      if (self->txids[i].size() == 64) continue;
 
      string prevtxid = "";
 
      if (i > 0) {
        prevtxid = self->txids[i - 1];
 
        while (rpc->tx_confirmations(prevtxid) < (int) self->confirmations)
          sleep(1);

      } else if (self->update_datahash.length() == 64) {

        while (rpc->tx_confirmations(updatetxid) < (int) self->confirmations)
          sleep(1);
      }

      if (!TX::valid(self->envs[i], publickey)) {
        self->errors[i] = "could not verify signature";
        sleep(1);
        continue;
      }
 
      string data;
      self->envs[i]->SerializeToString(&data);
      log_str("sending part " + itoa(i + 1) + ": " + itoa(data.length()));
      pair<bool, string> result = rpc->senddata(data);
 
      if (result.first)
        self->txids[i] = result.second;
      else
        self->errors[i] = result.second;
 
      db->update_file(self->envs[i]->partnumber(), self->sha256, self->errors[i], self->txids[i]);
    }
    db->update_file_status(self->sha256, FS_SENDED); 
  }

  if (db->get_file_status(self->sha256) == FS_SENDED) {
    while (rpc->tx_confirmations(self->txids.back()) < (int) self->confirmations)
      sleep(1);

    db->update_file_status(self->sha256, FS_FINISHED);
  }
  return NULL;
}

/* returns the txids of this with number of confirmations */
vector<pair<string, int> > StoreFileProcess::get_txids() {

  Rpc *rpc = Rpc::get_instance();
  vector<pair<string, int> > txids;

  for (unsigned i = 0; i < this->txids.size(); i++)
    txids.push_back(make_pair(this->txids[i], rpc->tx_confirmations(this->txids[i])));

  return txids;
}

/* return the errors of this */
vector<string> StoreFileProcess::get_errors() {
  return vector<string>(errors.begin(), errors.end());
}

/* returns if this is stored */
bool StoreFileProcess::stored() {
  return db->get_file_status(sha256) == FS_FINISHED;
}

/* returns the filename of the stored file */
string StoreFileProcess::get_filename() {
  if (envs.empty() || !envs[0]->has_filename())
    return "";
 
  return envs[0]->filename();
}

/* returns the number of needed confirmations */
unsigned StoreFileProcess::needed_confirmations() {
  return confirmations;
}

/* clear errors */
void StoreFileProcess::clear_errors() {
  for (unsigned i = 0; i < errors.size(); i++)
    errors[i] = "";
}
