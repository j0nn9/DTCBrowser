/**
 * Datacoin Browser's main entry point
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
#include "Database.h"
#include "Rpc.h"
#include "HTTPServer.h"
#include "envelope.pb.h"
#include "utils.h"
#include <unistd.h>
#include <iomanip>
#include "StoreFileProcess.h"

#ifdef DEBUG
#define HELP \
"      --scan-all                 scan the whole blockchain                \n"\
"      --testnet                  connect to testnet                       \n"\
"      --local [DIR]              run additional local server in DIR       \n"\
"      --tx-types                 list all tx types                        \n"\
"      --tx-type [TYPE]           list all txids with the given type       \n"\
"  -s  --store [FILE]             store FILE in the blockchain             \n"\
"  -u  --update [TXID]            the TXID of the file to update           \n"\
"  -l  --list-blocks              list all blocks with data txids          \n"\
"  -b  --block-txids [NUM]        list all data txids of the given block id\n"\
"  -i  --tx-info [HASH]           get tx info for the given HASH           \n"\
"  -d  --tx-data [HASH]           get tx data for th given HASH            \n"\
"      --sha256 [FILE]            create the sha256 sum form file          \n"\
"      --magic [FILE]             prints the content type of file          \n"\
"  -h  --help                     print this message                       \n"
#else
#define HELP \
"  DTCBrowser  Copyright (C)  2015  Jonny Frey  <j0nn9.fr39@gmail.com>     \n"\
"                                                                          \n"\
"  -u  --rpc-user                 datacoin rpc username                    \n"\
"  -p  --rpc-passwd               datacoin rpc password                    \n"\
"      --scan-all                 scan the whole blockchain                \n"\
"      --testnet                  connect to testnet                       \n"\
"      --local [DIR]              run additional local server in DIR       \n"\
"      --license                  print license information                \n"\
"  -h  --help                     print this message                       \n"
#endif

#define LICENSE \
"    DTCBrowser is a local Datacoin (DTC) BlockChain access server         \n"\
"                                                                          \n"\
"    Copyright (C)  2015  Jonny Frey  <j0nn9.fr39@gmail.com>               \n"\
"                                                                          \n"\
"    This program is free software: you can redistribute it and/or modify  \n"\
"    it under the terms of the GNU General Public License as published by  \n"\
"    the Free Software Foundation, either version 3 of the License, or     \n"\
"    (at your option) any later version.                                   \n"\
"                                                                          \n"\
"    This program is distributed in the hope that it will be useful,       \n"\
"    but WITHOUT ANY WARRANTY; without even the implied warranty of        \n"\
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         \n"\
"    GNU General Public License for more details.                          \n"\
"                                                                          \n"\
"    You should have received a copy of the GNU General Public License     \n"\
"    along with this program.  If not, see <http://www.gnu.org/licenses/>. \n"


int main(int argc, char *argv[]) {

  if (has_arg("--license")) {
    cout << LICENSE;
    exit(EXIT_SUCCESS);
  }

  if (has_arg("-h", "--help")        || 
      !has_arg("-u", "--rpc-user")   ||
      !has_arg("-p", "--rpc-passwd")) {

    cout << HELP;
    exit(EXIT_SUCCESS);
  }

#ifdef DEBUG
  if (has_arg("--magic")) {
    cout << file_content(read_file(get_arg("--magic"))) << endl;
    exit(EXIT_SUCCESS);
  }

  if (has_arg("--sha256")) {
    cout << "SHA:  " << sha256sum(bzip2_compress(read_file(get_arg("--sha256")))) << endl;
    cout << "DATA: " << to_hex(bzip2_compress(read_file(get_arg("--sha256")))) << endl;
    cout << "BIN:  \"" << bzip2_compress(read_file(get_arg("--sha256"))) << "\"" << endl;
    cout << "DEC:  \"" << bzip2_decompress(bzip2_compress(read_file(get_arg("--sha256")))) << "\"" << endl;
    exit(EXIT_SUCCESS);
  }
#endif
  
  string port = "11777";
  string db_file = "dtc.db";
  bool testnet = false;

  if (has_arg("--testnet")) {
    port = "11776";
    db_file = "dtc-testnet.db";
    testnet = true;
  }

  Rpc::init_curl(get_arg("-u", "--rpc-user") + ":" + get_arg("-p", "--rpc-passwd"),
                 "localhost:" + port);


  if (has_arg("--scan-all")) {
    Database::get_instance(db_file.c_str(), false, testnet);
  } else {
    Database::get_instance(db_file.c_str(), true, testnet);
  }

  if (has_arg("--local"))
    HTTPServer::get_instance(get_arg("--local"));
  else
    HTTPServer::get_instance();

#ifdef DEBUG
  if (has_arg("--tx-types")) {
    vector<string> txs = Database::get_instance()->get_file_types();

    for (unsigned i = 0; i < txs.size(); i++)
      cout << txs[i] << endl;

    exit(EXIT_SUCCESS);
  }

  if (has_arg("--tx-type")) {
    vector<tuple<string, string, string> > txs;
    txs = Database::get_instance()->get_tx_type(get_arg("--tx-type"));

    for (unsigned i = 0; i < txs.size(); i++)
      cout << get<0>(txs[i]) << endl;

    exit(EXIT_SUCCESS);
  }

  if (has_arg("-l", "--list-blocks")) {
    vector<unsigned> blocks = Database::get_instance()->get_block_list();
    for (unsigned i = 1; i < blocks.size(); i++) {
      cout << setw(6) << blocks[i] << ", ";
      
      if (i % 10 == 0)
        cout << endl;
    }

    cout << endl;
    exit(EXIT_SUCCESS);
  }

  if (has_arg("-b", "--block-txids")) {
    vector<string> txs = Database::get_instance()->get_block_txs(get_i_arg("-b", "--block-txids"));

    for (unsigned i = 0; i < txs.size(); i++)
      cout << txs[i] << endl;

    exit(EXIT_SUCCESS);
  }

  if (has_arg("-i", "--tx-info")) {
    TX tx = Database::get_instance()->get_tx(get_arg("-i", "--tx-info"));
    cout << tx.to_string();

    exit(EXIT_SUCCESS);
  }

  if (has_arg("-d", "--tx-data")) {
    TX tx = Database::get_instance()->get_tx(get_arg("-d", "--tx-data"));
    write(STDOUT_FILENO, tx.get_data().c_str(), tx.get_data().length());
    exit(EXIT_SUCCESS);
  }

  if (has_arg("-s", "--store")) {
    string fname = get_arg("-s", "--store");
    string file = read_file(fname);
    string data = bzip2_compress(file);
    string hash = sha256sum(file);
    string content = file_content(file);
    string updatetxid = (has_arg("-u", "--update") ? get_arg("-u", "--update") : "");
    vector<string> addresses = Rpc::get_instance()->listaddresses();

    cout << "Storing File: " << fname << " (" << data.length() << ")" << endl;
    cout << "  hash:     " << hash << endl << "  content: " << content << endl;
    cout << "  updating: " << updatetxid << endl;
    cout << "  address:  " << addresses[0] << endl << endl;

    StoreFileProcess process(data,
                             hash, 
                             fname, 
                             content, 
                             ENV_BZIP2, 
                             addresses[0],
                             1,
                             updatetxid);
    
    do {
      sleep(1);
      vector<pair<string, int> > txids = process.get_txids();
      vector<string> errors            = process.get_errors();
      for (unsigned i = 0; i < errors.size(); i++) {
        cout << "[" << i << "] ";
        if (i < txids.size())
          cout << txids[i].first << ": " << txids[i].second << " ";
        cout << "errors: " << errors[i] << endl;
       }
       cout << endl;
    } while (!process.stored());
  }
#endif 

  sleep(10000000);
  Rpc::cleanup();
  Database::cleanup();
  return 0;
}

