/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *   Socket client/server example with threads
  *
  * (Fedora version)
  *
 **/
 
 #include <iostream>
 #include <thread>
 #include "FileSystem.h"
 #include "Socket.h"
 
 #define PORT 1235
 #define BUFSIZE 512
 FileSystem fs("disk.bin", 100);
 

 struct Request {
   char command;
   std::string message;
};

// This function will parse the incoming request according to the protocol defined in the intermediary
Request parseRequest(const std::string& input) {
   Request req;

   // This is to manage some of the rules of the protocol
   if (input.size() < 4 || input[0] != 'P' || input[1] != '/') {
       throw std::runtime_error("Invalid protocol");
   }

   req.command = input[2];

   // In this protocolo, message is optional, so we check if it exists before trying to access it
   if (input.size() > 4) {
       req.message = input.substr(4);
   }

   return req;
}
 
 /**
  *   Task each new thread will run
  *      Read string from socket
  *      Write it back to client
  *
  **/

void task(VSocket* client) {
   char buffer[BUFSIZE] = {0};

   client->Read(buffer, BUFSIZE);
   std::string input(buffer);

   std::cout << "Raw request: " << input << std::endl;

   try {
       auto req = parseRequest(input);

       if (req.command == 'G') {  // GET figure
        std::string raw = fs.readFile(req.message);

        Figure full = fs.readFigure(req.message);


        if (full.pieces.empty()) {
            client->Write("P/D/ERROR_404");
        } else {
            std::string result = full.serialize();
            client->Write(("P/D/" + result).c_str());
}
       }
       else if (req.command == 'R') { // Directory
           std::string result;

           for (const auto& file : fs.getDirectory().getFiles()) {
                result += file.name + "\n";
           }

           std::cout << "Directory contents:\n" << result << std::endl;

           client->Write(("P/D/" + result).c_str());
       }
       else if (req.command == 'Q') {
           client->Close();
           return;
       }
       else if (req.command == 'W') {
        size_t sep = req.message.find("|");

        if (sep == std::string::npos) {
            client->Write("P/D/ERROR_INVALID_WRITE");
        } else {
            std::string name = req.message.substr(0, sep);
            std::string data = req.message.substr(sep + 1);

            Figure fig = Figure::deserialize(name, data);
            fs.writeFigure(fig);
            client->Write("P/D/WRITE_SUCCESS");
        }
       }
       else {
           client->Write("P/D/UNKNOWN_COMMAND");
       }

   } catch (...) {
       client->Write("P/D/ERROR_PROTOCOL");
   }

   client->Close();
}
 
 
 /**
  *   Create server code
  *      Infinite for
  *         Wait for client conection
  *         Spawn a new thread to handle client request
  *
  **/
 int main( int argc, char ** argv ) {
    std::thread * worker;
    VSocket * s1, * client;
 
    s1 = new Socket( 's' );
 
    s1->Bind( PORT );		// Port to access this mirror server
    s1->MarkPassive( 5 );	// Set socket passive and backlog queue to 5 connections
 
    for( ; ; ) {
       client = s1->AcceptConnection();	 	// Wait for a client connection
       worker = new std::thread( task, client );
       worker->detach();
    }
 }