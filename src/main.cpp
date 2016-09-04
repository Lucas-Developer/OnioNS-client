
#include "TorManager.hpp"
#include "resolver/LocalServer.hpp"
#include <onions-common/Log.hpp>
//#include <onions-common/Utils.hpp>
#include <onions-common/jsonrpccpp/server/connectors/httpserver.h>
//#include <botan/botan.h>
//#include <popt.h>
#include <iostream>

// Botan::LibraryInitializer init("thread_safe");

/*
todo:
  switch to argtable
  allow custom command-line parameters
  make localserver use resolver
  connect to mirror at startup
  resolve remotely
  use multi-threading when intercepting circuits
  split Tor response per line, drop stuff at the beginning
  make 9151 dynamic
  logging at debug, make this in onions-common
*/


void listenForDomains()
{
  Log::get().notice("Starting the JSON-RPC server on port 9053...");
  jsonrpc::HttpServer httpserver(9053);
  LocalServer s(httpserver);
  s.StartListening();

  // wait for quit character.
  // I don't expect for the user to send this, so this is essentially blocking
  Log::get().notice("Domain resolver online. Listening for domains...");
  char c = 'c';
  while (c != 'q')
    c = getchar();

  Log::get().notice("Shutting down the JSON-RPC server...");
  s.StopListening();
}

int main(int argc, char** argv)
{
  /*
  char* logPath = NULL;
  bool license = false;
  short port = 9150;

  struct poptOption po[] = {
      {"output", 'o', POPT_ARG_STRING, &logPath, 0,
       "Specifies the filepath for event logging.", "<path>"},
      {"port", 'p', POPT_ARG_SHORT, &port, 0,
       "SOCKS port to use for Tor communication. The default is 9150.",
       "<port>"},
      {"license", 'L', POPT_ARG_NONE, &license, 0,
       "Print software license and exit.", NULL},
      POPT_AUTOHELP{NULL, 0, 0, NULL, 0, NULL, NULL}};

  if (!Utils::parse(
          poptGetContext(NULL, argc, const_cast<const char**>(argv), po, 0)))
  {
    std::cout << "Failed to parse command-line arguments. Aborting.\n";
    return EXIT_FAILURE;
  }

  if (license)
  {
    std::cout << "Modified/New BSD License" << std::endl;
    return EXIT_SUCCESS;
  }

  if (logPath && strcmp(logPath, "-") != 0)
    Log::setLogPath(std::string(logPath));
*/

  TorManager::get().forkTor(argc, argv);
  listenForDomains();

  Log::get().notice("You are terminated!");
  return EXIT_SUCCESS;
}
