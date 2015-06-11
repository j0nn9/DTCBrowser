/**
 * Header of the Datacoin Browser's file uploading capability
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
#ifndef __STORE_FILE_PROCESS_H__
#define __STORE_FILE_PROCESS_H__
#include "Database.h"

/**
 * class to handle the process of storing a file in the blockchain
 */
class StoreFileProcess {

  private:
    
    /* the list of processed transactions and error information */
    vector<string> txids;

    /* the txid errors */
    vector<string> errors;

    /* the list of Envelopes to store */
    vector<Envelope *> envs;

    /* the txid of the first part of the previous file */
    string update_datahash;

    /* database reference */
    Database *db;

    /* sha256 sum of the file */
    string sha256;

    /* number of confirmations to wait until processing the next envelope */
    unsigned confirmations;

    /* thread object of this */
    pthread_t thread;

    /* load this from Database */
    void load();

  public:

    /**
     * initialize an StoreFileProcess
     * NOTE: compression has to be applied before 
     */
    StoreFileProcess(string file, 
                     string sha256,
                     string filename, 
                     string content_type,
                     unsigned compression,
                     string dtc_address,
                     unsigned confirmations,
                     string updatetxid = "");
    ~StoreFileProcess();

    /**
     * initialize an StoreFileProcess
     * NOTE: compression has to be applied before 
     */
    StoreFileProcess(string sha256,
                     unsigned confirmations);
                     
 
    /* processes the envelopes */
    static void *process(void *args);

    /* returns the txids of this with number of confirmations */
    vector<pair<string, int> > get_txids();

    /* return the errors of this */
    vector<string> get_errors();

    /* returns if this is stored */
    bool stored();

    /* returns the filename of the stored file */
    string get_filename();

    /* returns the number of needed confirmations */
    unsigned needed_confirmations();

    /* clear errors */
    void clear_errors();

    /* returns the byte size of this */
    unsigned byte_size();

    /* returns the data of this */
    string get_data();

    /* get data type */
    string get_content_type();

    /* get upload status */
    string get_upload_status();

    /* starts the upload process */
    void start_upload();

    /* aborts the upload process */
    void abort();

    /* returns the update txid */
    string get_upload_txid();
};

#endif /* __STORE_FILE_PROCESS_H__ */
