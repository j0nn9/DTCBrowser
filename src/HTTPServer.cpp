/**
 * Implementation of the Datacoin Browser's HTTP-Server Back-End
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
#include <magic.h>
#include <iostream>
#include <string>
#include "HTTPServer.h"
#include "Rpc.h"
#include "utils.h"
#include "TX.h"
#include <tuple>
#include <functional>

using namespace std;

/* standard favicon url */
#define FAVICON_URL "/dtc/get/73b7c7165a51f604040e17681705a39145392691a0447266b9074a2ad9140327"
#define FAVICON_URL_TESTNET "/dtc/get/7fe043a3ae95092a0d76bd2f5ec95e264efd7857d723e164fa882f9bdd92b57a"

/* checks the given url string for equality */
#define url_equal(url, pattern)                                         \
  (_url_equal(url, pattern, string("/dtc")) ||                          \
   _url_equal(url, pattern, string("/dtc-testnet")))

/* checks url to begin with */
#define url_begin_with(url, pattern)                                    \
  (_url_begin_with(url, pattern, string("/dtc")) ||                     \
   _url_begin_with(url, pattern, string("/dtc-testnet")))

/* checks url to begin with pattern, ans also has a specific length */
#define url_len(url, pattern, len)                                      \
  (_url_len(url, pattern, len, string("/dtc")) ||                       \
   _url_len(url, pattern, len, string("/dtc-testnet")))

/* checks the given url string for equality */
#define _url_equal(url, pattern, net_prefix)                            \
  (strlen(url) == net_prefix.length() + strlen(pattern) &&              \
   !strncmp(url + net_prefix.length(), pattern, strlen(pattern)))

/* checks url to begin with */
#define _url_begin_with(url, pattern, net_prefix)                       \
  (strlen(url) > net_prefix.length() + strlen(pattern) &&               \
   !strncmp(url + net_prefix.length(), pattern, strlen(pattern)))

/* checks url to begin with pattern, ans also has a specific length */
#define _url_len(url, pattern, len, net_prefix)                         \
  (strlen(url) == len &&                                                \
   !strncmp(url + net_prefix.length(), pattern, strlen(pattern)))

/* checks the given url string for equality */
#define stdurl_equal(url, pattern) \
  (strlen(url) == strlen(pattern) && !strncmp(url, pattern, strlen(pattern)))

/* checks url to begin with */
#define stdurl_begin_with(url, pattern) \
  (strlen(url) > strlen(pattern) && !strncmp(url, pattern, strlen(pattern)))

/* checks url to begin with pattern, ans also has a specific length */
#define stdurl_len(url, pattern, len) \
  (strlen(url) == len && !strncmp(url, pattern, strlen(pattern)))

#define POST_REQ 1
#define GET_REQ  2
#define POSTBUFFERSIZE 1048576

/* synchronization mutexes */
pthread_mutex_t HTTPServer::creation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t HTTPServer::post_mutex = PTHREAD_MUTEX_INITIALIZER; 

/* this will be a singleton */
HTTPServer *HTTPServer::only_instance = NULL;

/* indicates if this was initialized */
bool HTTPServer::initialized = false;

/* indicates whether a file is uploading or not */
bool HTTPServer::uploading = false;

/* the current uploading file */
StoreFileProcess *HTTPServer::upload_process = NULL;

/* upload error if any */
string HTTPServer::upload_error = "";

/* the local directly */
string HTTPServer::local_dir = "";

/* the web server port */
unsigned HTTPServer::port = SERVER_PORT;
    

/* return the only instance of this */
HTTPServer *HTTPServer::get_instance(string local_dir, unsigned port) {

  pthread_mutex_lock(&creation_mutex);
  if (!initialized) {

    if (local_dir.length() > 0 && local_dir[local_dir.length() - 1] != '/')
      local_dir = local_dir + "/";

    HTTPServer::local_dir = local_dir;
    HTTPServer::port      = port;
    only_instance = new HTTPServer();
    initialized   = true;
  }
  pthread_mutex_unlock(&creation_mutex);

  return only_instance;
}


/* private constructor to allow only one instance of this */
HTTPServer::HTTPServer() {
  daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, port, NULL, NULL, 
                            &connection_callback, NULL, 
                            MHD_OPTION_NOTIFY_COMPLETED, &cleanup, NULL, 
                            MHD_OPTION_END);
}     

/* private destructor */
HTTPServer::~HTTPServer() {

}

/**
 * send a local page
 */
int HTTPServer::process_local_page(struct MHD_Connection *connection, const char *url) {

  if (local_dir == "")
    return MHD_NO;

  struct MHD_Response *response;
  int ret;
  string file = "NO FILE";

  if (strlen(url) == 1 && !strncmp(url, "/", 1))
    file = read_file(local_dir + "index.html");
  else if (strlen(url) > 1 && !strncmp(url, "/", 1)) 
    file = read_file(local_dir + string(url));

  response = MHD_create_response_from_buffer(file.length(),
                                            (void*) file.c_str(), MHD_RESPMEM_MUST_COPY);

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  log_str("push: localpage");
  return ret;
}

/** 
 * processes a /dtc/get request 
 */
int HTTPServer::process_get(struct MHD_Connection *connection, const char *url) {

  Database *db = Database::get_instance();
  struct MHD_Response *response;
  int ret = MHD_NO;
  TX tx = TX();
  string txid = ""; 

  if (strlen(url) == 9 + 64)
    txid = string(url + 9, 64);

  if (strlen(url) == 1 && !strncmp(url, "/", 1))
    txid = MAIN_PAGE;
  
  if (db->is_envelope(txid))
    tx = db->reasemble_envelope(txid);
  else
    tx = db->get_tx(txid);

  if (!tx.empty()) {

    response = MHD_create_response_from_buffer(tx.get_data().length(),
                                               (void*) tx.get_data().c_str(), 
                                               MHD_RESPMEM_MUST_COPY);

    MHD_add_response_header(response, "Content-Type", tx.get_content().c_str());

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    log_str("push: " + string(tx.get_content())); 
  }

  return ret;
}

/**
 * process rpc method 
 */
int HTTPServer::process_rpc(struct MHD_Connection *connection, const char *url) {

  char *data = (char *) calloc(5,1);
  data[0] = 'N';
  data[1] = 'U';
  data[2] = 'L';
  data[3] = 'L';

  if (strlen(url) >= 16 && strlen(url) <= 17 && !strncmp(url, "/dtc/rpc/getinfo", 16))
    data = strdup(Rpc::get_instance()->getinfo_json());

  if (strlen(url) >= 22 && strlen(url) <= 23 && !strncmp(url, "/dtc/rpc/getmininginfo", 22))
    data = strdup(Rpc::get_instance()->getmininginfo_json());

  if (strlen(url) >= 22 && !strncmp(url, "/dtc/rpc/getblockhash/", 22))
    data = strdup(Rpc::get_instance()->getblockhash_json(atoi(url + 22)));

  if (strlen(url) == 82 && !strncmp(url, "/dtc/rpc/getblock/", 18))
    data = strdup(Rpc::get_instance()->getblock_json(url + 18));

  if (strlen(url) == 93 && !strncmp(url, "/dtc/rpc/getrawtransaction/", 27))
    data = strdup(Rpc::get_instance()->getrawtransaction_json(string(url + 27, 64)));

  log_str("push application/json: " + itoa(strlen(data)));
  struct MHD_Response *response = MHD_create_response_from_buffer(strlen(data),
                                            (void*) data, MHD_RESPMEM_MUST_FREE);

  MHD_add_response_header(response, "Content-Type", "application/json;");

  int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

/**
 * process local server rpc
 */
int HTTPServer::process_lsrpc(struct MHD_Connection *connection, const char *url) {

  Database *db = Database::get_instance();
  struct MHD_Response *response;
  int ret;

  if (url_len(url, "/lsrpc/getenvelope/", 87)) {

    stringstream ss;
    ss << "{ \"result\": {";

    TX tx = db->get_tx(string(url + 23, 64));
    if (!tx.empty() && tx.get_envelope().IsInitialized()) {

      ss << " \"Compression\":\"" << tx.get_compression() << "\"";
   
      if (tx.get_envelope().publickey() != "")
        ss << ", \"PublicKey\":\"" << str_repleace(tx.get_envelope().publickey(), "\"", "\\\"") << "\"";

      if (tx.get_envelope().signature() != "")
        ss << ", \"Signature\":\"" << str_repleace(tx.get_envelope().signature(), "\"", "\\\"") << "\"";

      if (tx.get_envelope().partnumber() != 0) 
        ss << ", \"PartNumber\":\"" << tx.get_envelope().partnumber() << "\"";

      if (tx.get_envelope().totalparts() != 0) 
        ss << ", \"TotalParts\":\"" << tx.get_envelope().totalparts() << "\"";

      if (tx.get_envelope().prevtxid() != "")
        ss << ", \"PrevTxId\":\"" << str_repleace(tx.get_envelope().prevtxid(), "\"", "\\\"") << "\"";

      if (tx.get_content() != "")
        ss << ", \"ContentType\":\"" << str_repleace(tx.get_content(), "\"", "\\\"") << "\"";

      if (tx.get_envelope().filename() != "")
        ss << ", \"FileName\":\"" << str_repleace(tx.get_envelope().filename(), "\"", "\\\"") << "\"";
   
    }

    ss << "} }";
    char *data = strdup(ss.str());

    response = MHD_create_response_from_buffer(strlen(data),
                                                (void*) data, MHD_RESPMEM_MUST_COPY);

    MHD_add_response_header(response, "Content-Type", "application/json;");
   
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
  }

  if (url_begin_with(url, "/lsrpc/listtxids/")) {

    stringstream ss;
    ss << "{ \"result\": [";
    vector<tuple<string, string, string> > txids;

    if (url_equal(url, "/lsrpc/listtxids/text"))
      txids = db->get_tx_type("text/plain");
    else if (url_equal(url, "/lsrpc/listtxids/image"))
      txids = db->get_tx_type("image/");
    else if (url_equal(url, "/lsrpc/listtxids/website"))
      txids = db->get_tx_type("text/html");
    else if (url_equal(url, "/lsrpc/listtxids/other"))
      txids = db->get_tx_unclassified();
      
    for (unsigned i = 0; i < txids.size(); i++) {
      
      ss << "{ \"hash\": \"" << get<0>(txids[i]) << "\", \"FileName\": ";
      
      if (get<1>(txids[i]) != "") 
        ss << "\"" << str_repleace(get<1>(txids[i]), "\"", "\\\"") << "\", ";
      else
        ss << "\"Unnamed File\", ";

      ss << "\"ContentType\": \"" << get<2>(txids[i]) << "\" }";
      
      if (i != txids.size() - 1)
        ss << ", ";
      
    }

    ss << "] }";

    response = MHD_create_response_from_buffer(ss.str().length(),
                                              (void*) ss.str().c_str(), MHD_RESPMEM_MUST_COPY);
   
    MHD_add_response_header(response, "Content-Type", "application/json;");
   
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
  }

  if (url_equal(url, "/lsrpc/getuploadstatus")) {
    
    stringstream ss;
    ss << "{ \"result\": { \"txids\": [";

    if (upload_process != NULL) {

      
      vector<pair<string, int> > txids = upload_process->get_txids();
      vector<string> errors            = upload_process->get_errors();

      for (unsigned i = 0; i < errors.size(); i++) {
        if (errors[i].length() > 0) {
          upload_error = errors[i];
          upload_process->clear_errors();
          break;
        }
      }

      for (unsigned i = 0; i < txids.size(); i++) {
        ss << " { \"txid\": \"" << txids[i].first << "\", ";
        ss << "\"confirmations\": " << itoa(txids[i].second);
        ss << " }";

        if ((i + 1) < txids.size() && (i + 1) < errors.size())
          ss << ",";
      }
    } 
    ss << " ]";

    if (upload_process != NULL) {
      string filename = upload_process->get_filename();
      filename = str_repleace(filename, "\"", "\\\"");
      ss << ", \"filename\": \"" << filename << "\"";
      ss << ", \"max_confirmations\": \"" << upload_process->needed_confirmations() << "\"";
    }

   ss << ", \"error\": \"";
    if (upload_error != "") {
      ss << upload_error;
      upload_error = "";
    }

    ss << "\" } }";
    char *data = strdup(ss.str());

    response = MHD_create_response_from_buffer(strlen(data),
                                                (void*) data, MHD_RESPMEM_MUST_FREE);

    MHD_add_response_header(response, "Content-Type", "application/json;");
   
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
  }

  if (url_equal(url, "/lsrpc/listaddresses")) {
    
    stringstream ss;
    ss << "{ \"result\": [";

    vector<string> addresses = Rpc::get_instance()->listaddresses();

    for (unsigned i = 0; i < addresses.size(); i++) {
      ss << " \"" << addresses[i] << "\"";

      if ((i + 1) < addresses.size())
        ss << ",";
    }

    ss << " ] }";
    char *data = strdup(ss.str());

    response = MHD_create_response_from_buffer(strlen(data),
                                                (void*) data, MHD_RESPMEM_MUST_FREE);

    MHD_add_response_header(response, "Content-Type", "application/json;");
   
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
  }

  if (url_equal(url, "/lsrpc/getversion")) {
    
    string result = "{ \"result\": " + itoa(SERVER_VERSION) + " }";
    char *data = strdup(result);

    response = MHD_create_response_from_buffer(strlen(data),
                                                (void*) data, MHD_RESPMEM_MUST_FREE);

    MHD_add_response_header(response, "Content-Type", "application/json;");
   
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
  }

  return MHD_NO;
}

/* clean up post function */
void HTTPServer::cleanup(void *cls, 
                         struct MHD_Connection *connection, 
                         void **con_cls,
                         enum MHD_RequestTerminationCode toe) {

  (void) connection;
  (void) cls;
  (void) toe;
  ConInfo *con_info = (ConInfo *) *con_cls;
  Database *db = Database::get_instance();
  
  if (con_info == NULL) 
    return;

  if (con_info->connectiontype == POST_REQ) {
    MHD_destroy_post_processor(con_info->postprocessor);        

    string fname      = string(con_info->filename);
    string file       = string(con_info->data, con_info->data_len);
    string data       = bzip2_compress(file);

    if (fname.length() < 1 || file.length() < 1) {
      upload_error = "invalid file";
      log_err(upload_error, LOG_W);
    }

    unsigned compression = ENV_BZIP2;
    
    /* on error use original file */
    if (data.substr(0, 3) == "BZ_" || data.length() > file.length()) {
      data = file;
      compression = ENV_NONE;
    }

    string hash       = sha256sum(file);
    string content    = file_content(file);
    string updatetxid = string(con_info->updatetxid);
    string publickey  = string(con_info->publickey);

    log_str("Storing File: " + fname + " (" + itoa(data.length()) + ")\n" +
            "  hash:     " + hash + "\n  content: " + content +
            "\n  updating: " + updatetxid +
            "\n  address:  " + publickey + "\n");

    for (unsigned i = 0; i < fname.length(); i++) {
      char c = (char) fname[i];

      if (c > 122 || (c < 97 && c != 95 && c > 90) || 
          (c < 65 && c > 57) || (c < 48 && c != 32 && c != 45 && c != 46)) {
        
        upload_error = "Filename contains illegal characters (allowed: A-z, 0-9, ., -, _, space)";
        break;
      }
    }

    if (updatetxid.length() == 64 && db->get_tx(updatetxid).empty()) {
      upload_error = "updatetxid: " + updatetxid + " could not be found";
      log_err(upload_error, LOG_W);
    }

    if (upload_error.length() == 0) {
      try {
        upload_process = new StoreFileProcess(data,
                                              hash, 
                                              fname, 
                                              content, 
                                              compression, 
                                              publickey,
                                              con_info->confirmations,
                                              updatetxid);
      } catch (const char *e) {
        log_err(e, LOG_E); 
        upload_error = string(e);
        upload_process = NULL;
      }
    }
    
    log_str("File Upload Finished: " + itoa(con_info->data_len));
    uploading = false;
  }
                          
 free (con_info->data); 
 free (con_info);
 *con_cls = NULL; 
}

/* post processing call back function */
int HTTPServer::iterate_post(void *coninfo_cls, 
                             enum MHD_ValueKind kind, 
                             const char *key,
                             const char *filename, 
                             const char *content_type,
                             const char *transfer_encoding, 
                             const char *data, 
                             uint64_t off, 
                             size_t size) {
  (void) kind;
  ConInfo *con_info = (ConInfo *) coninfo_cls;
  log_str("key:               " + string((key == NULL) ? "NULL" : key));
  log_str("filename:          " + string((filename == NULL) ? "NULL" : filename));
  log_str("content_type:      " + string((content_type == NULL) ? "NULL" : content_type));
  log_str("transfer_encoding: " + string((transfer_encoding == NULL) ? "NULL" : transfer_encoding));
  log_str("off:               " + itoa(off));
  log_str("size:              " + itoa(size));
  if (size < 50)
    log_str("data:              " + string(data));

  if (key != NULL && !strncmp(key, "file", 4)) {
    if (off + size > con_info->max_len) {
      char *ptr = (char *) realloc(con_info->data, con_info->max_len * 2);
      if (ptr == NULL) {
        log_err("realloc failed", LOG_E);
        return MHD_NO;
      }
 
      con_info->data = ptr;
      con_info->max_len *= 2;
    }
 
    if (filename != NULL && con_info->filename[0] == 0 && strlen(filename) < 1024)
      memcpy(con_info->filename, filename, strlen(filename));
 
    if (size > 0) {
      memcpy(con_info->data + off, data, size);
      con_info->data_len = off + size;
    }
  }

  if (key != NULL && !strncmp(key, "confirmations", 13)) {
    con_info->confirmations = atoi(string(data, size).c_str());
  }

  if (key != NULL && !strncmp(key, "publickey", 9) && size == 34) {
    memcpy(con_info->publickey, data, size);
  }

  if (key != NULL && !strncmp(key, "update_txid", 11) && size == 64) {
    memcpy(con_info->updatetxid, data, size);
  }
  
  return MHD_YES;
}

/* the call back function which handles a connection */
int HTTPServer::connection_callback(void *cls, struct MHD_Connection *connection, 
                                    const char *url, 
                                    const char *method, const char *version, 
                                    const char *upload_data, 
                                    size_t *upload_data_size, 
                                    void **con_cls) {

  (void) cls;
  (void) method;
  (void) version;
  (void) upload_data;
  (void) upload_data_size;
  log_str("get: " + string(url));

  if (*con_cls == NULL && !strncmp(method, "POST", 4)) {
    
    pthread_mutex_lock(&post_mutex);
    if (!uploading && upload_process != NULL && upload_process->stored()) {
      delete upload_process;
      upload_process = NULL;
      upload_error   = "";
    }        

    if (uploading || upload_process != NULL) {
      pthread_mutex_unlock(&post_mutex);
      upload_error = "only one simultaneous upload possible";
      goto process_get_req;
    }


    ConInfo *con_info  = (ConInfo *) calloc(1, sizeof(ConInfo));
    con_info->data     = (char *) malloc(1024*1024);
    con_info->data_len = 0;
    con_info->max_len  = 1024 * 1024;
    con_info->confirmations = 5;

    if (con_info == NULL) {
      pthread_mutex_unlock(&post_mutex);
      upload_error = "internal server error";
      return MHD_NO;
    }

    con_info->postprocessor = MHD_create_post_processor(connection, 
                                                        POSTBUFFERSIZE, 
                                                        iterate_post, 
                                                        (void *) con_info);   

    if (con_info->postprocessor == NULL) {
      free (con_info->data); 
      free (con_info); 
      pthread_mutex_unlock(&post_mutex);
      return MHD_NO;
    }
    con_info->connectiontype = POST_REQ;

    *con_cls = (void *) con_info; 
    pthread_mutex_unlock(&post_mutex);
    return MHD_YES;
  }

  if (!strcmp(method, "POST")) {
    ConInfo *con_info = (ConInfo *) *con_cls;

    if (*upload_data_size != 0) {

      pthread_mutex_lock(&post_mutex);
      uploading = true;
      MHD_post_process(con_info->postprocessor, upload_data, 
                       *upload_data_size);

      *upload_data_size = 0;
      pthread_mutex_unlock(&post_mutex);
      return MHD_YES;
    } else
      goto process_get_req;
  }

  if (!strcmp(method, "GET")) {

    /* entry point for redirected post requests */
    process_get_req:

    if (stdurl_equal(url, "/")) {
      if (local_dir != "")
        return process_local_page(connection, url);
      else
        return process_get(connection, url);
    }

    /* process get tx request */
    if (url_len(url, "/get/", 73)) {
      return process_get(connection, url);
 
    /* process rpc request */
    } else if (url_begin_with(url, "/rpc/")) {
      return process_rpc(connection, url);
 
    /* process get tx request */
    } else if (url_begin_with(url, "/lsrpc/")) {
      return process_lsrpc(connection, url);
 
    } else if (stdurl_equal(url, "/favicon.ico")) {
      int ret = process_get(connection, FAVICON_URL);
 
      if (ret == MHD_NO)
        ret = process_get(connection, FAVICON_URL_TESTNET);
    
      return ret;
    /* send local file */
    } else {
      return process_local_page(connection, url);
    }
  }


  return MHD_NO;
}
