/**
 * Header of the Datacoin Browser's HTTP-Server Back-End
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
#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__
#include <string>
#include <microhttpd.h>
#include "Database.h"
#include "StoreFileProcess.h"

#define PORT 8080

/* the current server version */
#define SERVER_VERSION 1

/* Datacoin Browser main page */
#define MAIN_PAGE "15ca9c05b69c785bfcc8abc31ede491898e455da3a27b9a8ac6caa3b5c67c227"

using namespace std;

typedef struct {
  int connectiontype;
  struct MHD_PostProcessor *postprocessor; 
  char *data;
  unsigned data_len;
  unsigned max_len;
  char filename[1024];
  char publickey[34];
  char updatetxid[64];
  unsigned confirmations;
} ConInfo;

class HTTPServer {
  
  private:
    
    /* this is a singleton */
    static HTTPServer *only_instance;

    /* synchronization mutexes */
    static pthread_mutex_t creation_mutex;
    static pthread_mutex_t post_mutex;

    /* indicates if this was initialized */
    static bool initialized;

    /* private constructor to allow only one instance of this */
    HTTPServer();

    /* private destructor */
    ~HTTPServer();

    /* the http server daemon */
    struct MHD_Daemon *daemon;

    /* indicates whether a file is uploading or not */
    static bool uploading;

    /* the current uploading file */
    static StoreFileProcess *upload_process;

    /* upload error if any */
    static string upload_error;

    /* the local directly */
    static string local_dir;
    
    /* clean up post function */
    static void cleanup(void *cls, 
                        struct MHD_Connection *connection, 
                        void **con_cls,
                        enum MHD_RequestTerminationCode toe);

    /* post processing call back function */
    static int iterate_post(void *coninfo_cls, 
                            enum MHD_ValueKind kind, 
                            const char *key,
                            const char *filename, 
                            const char *content_type,
                            const char *transfer_encoding, 
                            const char *data, 
                            uint64_t off, 
                            size_t size);

    /* the call back function which handles a connection */
    static int connection_callback(void *cls, struct MHD_Connection *connection, 
                                   const char *url, 
                                   const char *method, const char *version, 
                                   const char *upload_data, 
                                   size_t *upload_data_size, 
                                   void **con_cls);
    /**
     * send a local page
     */
    static int process_local_page(struct MHD_Connection *connection, const char *url);

    /* returns the data of a given envelope */
    static unsigned get_env_data(TX *tx, unsigned char **data);

    /** 
     * processes a /dtc/get request 
     */
    static int process_get(struct MHD_Connection *connection, const char *url);

    /**
     * process rpc method 
     */
    static int process_rpc(struct MHD_Connection *connection, const char *url);

    /**
     * process local server rpc
     */
    static int process_lsrpc(struct MHD_Connection *connection, const char *url);

  public:
    
    /* return the only instance of this */
    static HTTPServer *get_instance(string local_dir = "");

};
#endif /* __HTTPSERVER_H__ */
