# DTCBrowser - a local Datacoin (DTC) BlockChain access server
![ScreenShot](https://github.com/j0nn9/DTCBrowser/raw/master/preview.png)

---
## Key Features:
- extends the old [datacoin browser](https://github.com/foo1inge/datacoin-browser)
- fully backward compatible
- built-in main page with user friendly uploading
- easy access of all files stored in the datacoin blockchain
- support of large files as well as file updating
- written in pure C++
- binaries available for Windows and Linux

## Development status:
**Caution**: This project is currently in alpha status, use can lead to loss of coins:
Do not use with your main wallet!!

Any participation is welcome and encouraged.

Donations are also welcome: DATAcoinJdHGTXbEi8NhaivWtangLJ9L5x

## Getting started:
- Linux:
  - add something like:
  `rpcuser=<user>` <br />
  `rpcpassword=<password>` <br />
  `server=1` <br />
  to ~/.datacoin/datacoin.conf

  - run `./datacoin-http-server -u <user> -p <password>`

- Windows:
  - press windows-key + r
  - type: %AppData%
  - navigate to / create folder Datacoin
  - open / create file datacoin.conf
  - type:
  `rpcuser=<user>
  rpcpassword=<password>
  server=1`
  - save file
  - start datacoin wallet
  - edit start-datacoin-browser.bat
  - set `-u <user> and -p <password>` according to the settings in datacoin.conf
  - save file and run start-datacoin-browser.bat

---
## Develop web applications within the blockchain
1. get data from the blockchain: http://localhost:8080/dtc/get/TXHASH 
2. call datacoin daemon's methods: http://localhost:8080/dtc/rpc/METHOD(/ARGS)
    - getinfo
    - getmininginfo
    - getblockhash/INDEX
    - getblock/HASH
    - getrawtransaction/TXHASH
3. call local server's methods: http://localhost:8080/dtc/lsrpc/METHOD(/ARGS)
    - getenvelope/TXHASH (get infos about a file)
    - listtxids/text (list of all plain text files)
    - listtxids/image (list of all images)
    - listtxids/website (list of all websites)
    - listtxids/other (list of all other files)
    - getversion (returns the server version)
4. use relative links e.g. /dtc/get/TXHASH

## Datacoin file format
    // optional file indetifier
    optional string FileName
    
    // will be sent to web browser as is in HTTP header
    optional string ContentType
    
    // currentyl only bzip2 
    required CompressionMethod Compression

    // datacoin address used with signmessage
    optional string PublicKey

    // output of datacoind signmessage <PublicKey> <DataHash>
    optional string Signatur

    // Big files can be sent as several txes (PartNumber starts from one)
    optional uint32 PartNumber
    optional uint32 TotalParts 

    // DataHash is created over all available fields except signature
    optional string PrevDataHash

    // avoid replay attacks on updates
    optional uint32 DateTime

    // set to 2 for now
    optional uint32 version
    
    // the actual binary data
    required bytes Data



### Overview of envelope rules

 1. Only CompressionMethod and Data fields are required. Compression method can
    be set to "None" (means no compression).

 2. ContentType will help to sort data by content (all PGP key or all FOAF files) and
    will be provided to web browser as is in HTTP header. ContentType is optional.

 3. Files bigger than max tx size (> 128Kb) are to be sent in form of sequences.
    PartNumber, TotalParts, Signature and PrevDataHash fields are to be used.
    Signature proves that the second part is sent by the same person.

 4. To indicate that this file is a new version of some other file previously stored
    in blockchain, this new file has to be signed with datacoin address corresponding
    to public key of the file it updates.
    Signature and PrevDataHash fields are used to update old data.

    Note: Data without PublicKey can't be updated.

#### Big files

Due to a tx size limitation, big files are to be packed into a sequence of transactions:

    Part 1: {
      PublicKey  = datacoin address
      Signature  = datacoin signmessage <public> <hash of this part and all of its envelope fileds>
      PartNumber = 1
      DateTime   = <current date>
      version    = 2
      TotalParts = total number of txes in sequence
      .. other fields
    }

    Part 2 .. TotalParts: {
      PublicKey isn't defined
      Signature    = datacoin signmessage <public> <hash of this part and all of its envelope fileds>
      PartNumber   = 2
      PrevDataHash = <hash of part 1 and all of its envelope fileds>
      DateTime     = <has to be greater than part1.DateTime>
      version    = 2
      TotalParts = total number of txes in sequence
      ... other fields
    }

#### Updates

Files are addressed by Datacoin. File name is stored for user's convenience only. In order to update
already stored in blockchain file, an update must be sent following way:

    {
      PrevDataHash = DataHash of the FIRST Envelope file and part among all updates
      Signature    = datacoin signmessage <public> <hash of this part and all of its envelope fileds>
      PrevDataHash = <hash of part 1 and all of its envelope fileds>
      DateTime     = <has to be greater than <first file, first part>.DateTime>
      version      = 2
      ... other fields
    }

##### Updating something with big file

Big files are to be split into a sequence of txes. In this case first tx in sequence points
to the tx we are updating (with PrevDataHash and Signature). All other txes in the sequence will
follow rules provided above in "Big files" section.
