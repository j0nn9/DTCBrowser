/**
 * Header of the Datacoin Browser's RPC Back-End, which handles the 
 * connection with the datacoin daemon
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
#ifndef __STDC_FORMAT_MACROS 
#define __STDC_FORMAT_MACROS 
#endif
#ifndef __STDC_LIMIT_MACROS  
#define __STDC_LIMIT_MACROS  
#endif
#include "Rpc.h"
#include "utils.h"
#include <jansson.h>

using namespace std;

/* synchronization mutexes */
pthread_mutex_t Rpc::send_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Rpc::creation_mutex = PTHREAD_MUTEX_INITIALIZER;


/* this will be a singleton */
Rpc *Rpc::only_instance = NULL;

/* indicates if this was initialized */
bool Rpc::initialized = false;

/* indicates if curl was initialized */
bool Rpc::curl_initialized = false;

/* rpc timeout */
int Rpc::timeout = 60;

/* string stream for receiving */
stringstream *Rpc::recv_ss = new stringstream;

/* curl receive session handle */
CURL *Rpc::curl_recv = NULL;

/* curl send session handle */
CURL *Rpc::curl_send = NULL;

/**
 * curl callback function to send rpc commands to the server
 */
size_t curl_read(void *ptr, size_t size, size_t nmemb, void *user_data) {

  static unsigned int pos = 0;
  string *rpccmd = (string *) user_data;
  unsigned int len = rpccmd->length();

  if (pos < len) {
    unsigned int bytes = (size * nmemb >= len - pos) ? len - pos : size * nmemb;
    memcpy(ptr, rpccmd->c_str() + pos, bytes);
    
    pos += bytes;
    return bytes;
  }
  
  pos = 0;
  return 0;
}

/**
 * curl callback function to receive rpc responses from the server
 */
size_t curl_write(char *ptr, size_t size, size_t nmemb, void *user_data) {
  stringstream *ss = (stringstream *) user_data;
  
  if ((size * nmemb) > 0)
    ss->write(ptr, size * nmemb);

  return size * nmemb;
}

/**
 * initialize curl with the given 
 * username, password, url and port
 */
bool Rpc::init_curl(string userpass, string url, int timeout) {

  curl_global_init(CURL_GLOBAL_ALL);

  curl_recv = curl_easy_init();
  curl_send = curl_easy_init();
  if(curl_recv && curl_send) {
    curl_easy_setopt(curl_recv, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_recv, CURLOPT_ENCODING, "");
    curl_easy_setopt(curl_recv, CURLOPT_FAILONERROR, 0);
    curl_easy_setopt(curl_recv, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_recv, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(curl_recv, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(curl_recv, CURLOPT_WRITEDATA, (void *) recv_ss);
    curl_easy_setopt(curl_recv, CURLOPT_READFUNCTION, curl_read);
    curl_easy_setopt(curl_recv, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_recv, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl_recv, CURLOPT_POST, 1);
    curl_easy_setopt(curl_recv, CURLOPT_USERPWD, userpass.c_str());
    curl_easy_setopt(curl_recv, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

    curl_easy_setopt(curl_send, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_send, CURLOPT_ENCODING, "");
    curl_easy_setopt(curl_send, CURLOPT_FAILONERROR, 0);
    curl_easy_setopt(curl_send, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl_send, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(curl_send, CURLOPT_WRITEFUNCTION, curl_write);
    curl_easy_setopt(curl_send, CURLOPT_WRITEDATA, (void *) recv_ss);
    curl_easy_setopt(curl_send, CURLOPT_READFUNCTION, curl_read);
    curl_easy_setopt(curl_send, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_send, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl_send, CURLOPT_POST, 1);
    curl_easy_setopt(curl_send, CURLOPT_USERPWD, userpass.c_str());
    curl_easy_setopt(curl_send, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

    Rpc::timeout    = timeout;
    curl_initialized = true;
    return true;
  }
  
  return false;
}

/* run clean up */
void Rpc::cleanup() {
  pthread_mutex_lock(&creation_mutex);
  curl_easy_cleanup(curl_recv);
  curl_easy_cleanup(curl_send);
  curl_global_cleanup();
  delete only_instance;

  only_instance    = NULL;
  initialized      = false;
  curl_initialized = false;
  pthread_mutex_unlock(&creation_mutex);
}

/* return the only instance of this */
Rpc *Rpc::get_instance() {

  pthread_mutex_lock(&creation_mutex);
  if (!initialized) {
    only_instance = new Rpc();
    initialized   = true;
  }
  pthread_mutex_unlock(&creation_mutex);

  return (curl_initialized ? only_instance : NULL);
}

/* private constructor */
Rpc::Rpc() { }

/* destructor */
Rpc::~Rpc() { }

/* call the specific rpc method with the given args */
string Rpc::call_rpc(string method, string params) {

  stringstream data;
  data << "{\"jsonrpc\": \"1.0\", ";
  //data << "\"id\":\"" USER_AGENT "\", ";
  data << "\"method\": \"" << method << "\", ";
  data << "\"params\": [" << params << "] }";
  string content = data.str();

  /* building http header */
  char content_len[64];
  struct curl_slist *header = NULL;

  sprintf(content_len, "Content-Length: %lu", content.length());
  header = curl_slist_append(header, "Content-Type: application/json");
  header = curl_slist_append(header, content_len);
  header = curl_slist_append(header, "User-Agent: " USER_AGENT);
  header = curl_slist_append(header, "Accept:"); /* disable Accept hdr*/
  header = curl_slist_append(header, "Expect:"); /* disable Expect hdr*/

  pthread_mutex_lock(&send_mutex);
  if(curl_recv) {
    curl_easy_setopt(curl_recv, CURLOPT_READDATA, (void *) &content);
    curl_easy_setopt(curl_recv, CURLOPT_HTTPHEADER, header);
  } else {
    pthread_mutex_unlock(&send_mutex);
    return "";
  }

  CURLcode res;

  /* Perform the request, res will get the return code */ 
  res = curl_easy_perform(curl_recv);
  curl_slist_free_all(header);

  /* Check for errors */ 
  if(res != CURLE_OK)  {
    log_err("curl_easy_perform() failed(" + itoa(res) + "): " + string(curl_easy_strerror(res)) +
            "content: " + ((content.length() > 100) ? content.substr(0, 100) : content), LOG_E);
    pthread_mutex_unlock(&send_mutex);
    return "";
  }
  
  string ret = recv_ss->str().c_str();
  recv_ss->str("");
  pthread_mutex_unlock(&send_mutex);

  return ret;
}

/* returns the number of blocks */
unsigned Rpc::getblockcount() {

  string response = call_rpc("getblockcount", "");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return 0;
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return 0;
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_integer(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return 0;
  }

  unsigned blockcount = json_number_value(result);
  json_decref(root);

  return blockcount;
}

/* returns the number of blocks */
string Rpc::getblockhash(unsigned height) {

  string response = call_rpc("getblockhash", itoa(height));

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return "";
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return "";
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_string(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return "";
  }

  string blockhash = json_string_value(result);
  json_decref(root);

  return blockhash;
}

/* returns a vector with all transaction hashes 
 * from a block with the given index */
vector<string> Rpc::getblocktransactions(unsigned height) {

  string response = call_rpc("getblock", "\"" + getblockhash(height) + "\"");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return vector<string>();
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return vector<string>();
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_object(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return vector<string>();
  }

  json_t *jary = json_object_get(result, "tx");

  if (!json_is_array(jary)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return vector<string>();
  }

  vector<string> tx;
  json_t *obj;
  for (unsigned i = 0; i < json_array_size(jary); i++) {
    obj = json_array_get(jary, i);

    if (!json_is_string(obj)) {
      log_err("can not parse server response: " + response, LOG_E);
      json_decref(obj);
      return vector<string>();
    }

    tx.push_back(json_string_value(obj));
  }
  json_decref(root);

  return tx;
}

/* returns the base64 encoded transaction data */
string Rpc::getdata(string txhash) {

  string response = call_rpc("getdata", "\"" + txhash + "\"");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return "";
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return "";
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_string(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return "";
  }

  string txdata = json_string_value(result);
  json_decref(root);

  return b64_to_byte(txdata);
}


/* returns the transaction hash or error for the sended data */
pair<bool, string> Rpc::senddata(string data) {

  string b64 = to_b64(data);
  string response = call_rpc("senddata", "\"" + b64 + "\"");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return make_pair(false, "json error: " + string(error.text));
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return make_pair(false, "json error: " + string(error.text));
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_string(result)) {
    
    json_t *errmsg = json_object_get(root, "error");
    if (!json_is_object(errmsg)) {
      log_err("can not parse server response: " + response, LOG_E);
      json_decref(root);
      return make_pair(false, "json error: " + string(error.text));
    }

    json_t *message = json_object_get(errmsg, "message");
    if (!json_is_string(message)) {
      log_err("can not parse server response: " + response, LOG_E);
      json_decref(root);
      return make_pair(false, "json error: " + string(error.text));
    }

    string error_msg = json_string_value(message);
    json_decref(root);
    return make_pair(false, error_msg);
  }

  string tx = json_string_value(result);
  json_decref(root);

  return make_pair(true, tx);
}
    
/* signs the given message with the given address */
string Rpc::signmessage(string address, string message) {
  
  string response = call_rpc("signmessage", "\"" + address + "\", \"" + message + "\"");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return "";
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return "";
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_string(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return "";
  }

  string txdata = json_string_value(result);
  json_decref(root);

  return txdata;
}

/* verifies the given message */
bool Rpc::verifymessage(string address, string signature, string message) {

  if (address.length() != 34 || signature.length() != 88 || message.length() != 64)
    return false;
  
  string response = call_rpc("verifymessage", "\"" + address + "\", \"" +
                             signature + "\", \"" + message + "\"");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return false;
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return false;
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_boolean(result)) {
    log_err("can not parse server response(" + address + ", " + 
            signature + ", " + message + ": " + response, LOG_E);
    json_decref(root);
    return false;
  }

  bool res = json_is_true(result);
  json_decref(root);

  return res;
}

/* returns the number of confirmations for a given transaction */
int Rpc::tx_confirmations(string txid) {

  if (txid.length() != 64)
    return -1;

  string response = call_rpc("getrawtransaction", "\"" + txid + "\", 1");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return -1;
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return -1;
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_object(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return -1;
  }

  json_t *confs = json_object_get(result, "confirmations");

  if (!json_is_integer(confs)) {
    json_decref(root);
    return 0;
  }

  int confirmations = json_integer_value(confs);
  json_decref(root);

  return confirmations;
}


/* returns a list of accounts */
vector<string> Rpc::listaccounts() {

  string response = call_rpc("listaccounts", "");

  /* parse response */
  json_t *root;
  json_error_t error;

  root = json_loads(response.c_str(), 0, &error);

  if(!root) {
    log_str("jansson error: on line " + itoa(error.line) + ": " + 
            string(error.text) + ": " + response, LOG_E);
    return vector<string>();
  }

  if (!json_is_object(root)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return vector<string>();
  }
  json_t *result = json_object_get(root, "result");

  if (!json_is_object(result)) {
    log_err("can not parse server response: " + response, LOG_E);
    json_decref(root);
    return vector<string>();
  }
  
  const char *key;
  json_t *value;
  vector<string> accounts;

  json_object_foreach(result, key, value) {
    accounts.push_back(string(key));
  }

  json_decref(root);
  return accounts;
}

/* returns all addresses of all accounts */
vector<string> Rpc::listaddresses() {

  vector<string> addresses;
  vector<string> accounts = listaccounts();

  for (unsigned i = 0; i < accounts.size(); i++) {

    string response = call_rpc("getaddressesbyaccount", "\"" + accounts[i] + "\"");
 
    /* parse response */
    json_t *root;
    json_error_t error;
 
    root = json_loads(response.c_str(), 0, &error);
 
    if(!root) {
      log_str("jansson error: on line " + itoa(error.line) + ": " + 
              string(error.text) + ": " + response, LOG_E);
      return vector<string>();
    }
 
    if (!json_is_object(root)) {
      log_err("can not parse server response: " + response, LOG_E);
      json_decref(root);
      return vector<string>();
    }
    json_t *result = json_object_get(root, "result");
 
    if (!json_is_array(result)) {
      log_err("can not parse server response: " + response, LOG_E);
      json_decref(root);
      return vector<string>();
    }

    vector<string> tx;
    json_t *obj;
    for (unsigned n = 0; n < json_array_size(result); n++) {
      obj = json_array_get(result, n);
 
      if (!json_is_string(obj)) {
        log_err("can not parse server response: " + response, LOG_E);
        json_decref(obj);
        return vector<string>();
      }
 
      addresses.push_back(json_string_value(obj));
    }
    
    json_decref(root);
  }

  return addresses;
}

/* returns the json server answer for getinfo */
string Rpc::getinfo_json() {
  return call_rpc("getinfo", "");
}

/* returns the json server answer for getmininginfo */
string Rpc::getmininginfo_json() {
  return call_rpc("getmininginfo", "");
}

/* returns the json server answer for getblockhash */
string Rpc::getblockhash_json(unsigned height) {
  return call_rpc("getblockhash", itoa(height));
}

/* returns the json server answer for getblockhash */
string Rpc::getblock_json(string hash) {
  return call_rpc("getblock", "\"" + hash + "\"");
}

/* returns the json server answer for getrawtransaction */
string Rpc::getrawtransaction_json(string txid) {
  return call_rpc("getrawtransaction", "\"" + txid + "\", 1");
}
