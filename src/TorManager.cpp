
#include "TorManager.hpp"
#include "sockets/SocketException.h"
#include <onions-common/Log.hpp>
#include <botan/hex_filt.h>
#include <botan/pipe.h>
#include <thread>
//#include <chrono>
//#include <netdb.h>
//#include <sys/wait.h>
//#include <errno.h>
//#include <string.h>
//#include <unistd.h>
//#include <stdio.h>


void TorManager::forkTor(int argc, char* argv[])
{
  /*
  temporarily commented out
  if (argc < 13)  // there are more reliable ways, but this works decently
  {
    Log::get().notice("Tor Browser not detected; won't manage Tor.");
    return;
  }

  //pid_t torP = startTor(argv);
  if (startTor(argv) <= 0)
  {
    Log::get().warn("Failed to start Tor, will run normal local listener.");
    return;
  }
  */

  manageTor();

  /*
    std::chrono::milliseconds pollTime(250);
    while (ProcessWatch::isRunning(torP))
      std::this_thread::sleep_for(pollTime);
  */

  // I learned in college that you have to kill your children before killing
  // yourself
  // kill(torP, SIGTERM);
}



pid_t TorManager::startTor(char** argv)
{
  argv[0] = const_cast<char*>("TorBrowser/Tor/torbin");
  pid_t torP = startProcess(argv);
  if (torP <= 0)
    return torP;

  // wait for Tor's control port to be available
  while (!isOpen(9151))
  {
    // https://stackoverflow.com/questions/5278582
    // if (waitpid(torP, NULL, WNOHANG) != 0)  // test for existence of Tor
    //  exit(0);  // if the Tor Browser was closed, then quit
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  Log::get().notice("Tor's control port is now available.");

  return torP;
}



void TorManager::manageTor()
{
  ClientSocket clientSocket("127.0.0.1", 9151);

  Log::get().notice("Attempting to authenticate to Tor...");
  authenticateToTor(clientSocket);
  Log::get().notice("Waiting for Tor to finish bootstrapping...");
  waitForBootstrap(clientSocket);
  Log::get().notice("Tor bootstrap complete.");
}



void TorManager::authenticateToTor(ClientSocket& clientSocket)
{
  try
  {
    std::string hash = getCookieHash(getCookiePath(clientSocket));
    Log::get().notice("Read authentication cookie.");
    clientSocket << "AUTHENTICATE " + hash + "\r\n";

    std::string response;
    clientSocket >> response;

    if (response == "250 OK\r\n")
      Log::get().notice("Authentication complete.");
    else
    {
      Log::get().notice("Tor replied: " + response);
      Log::get().warn("Unexpected answer from Tor!");
    }
  }
  catch (SocketException& e)
  {
    Log::get().warn("Communication error with Tor's control port! " +
                    e.description());
  }
}



void TorManager::waitForBootstrap(ClientSocket& clientSocket)
{
  std::string response;
  const std::string readyState = "BOOTSTRAP PROGRESS=100";

  while (response.find(readyState) == std::string::npos)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    clientSocket << "GETINFO status/bootstrap-phase\r\n";
    clientSocket >> response;
  }
}



std::string TorManager::getCookiePath(ClientSocket& clientSocket)
{
  try
  {
    clientSocket << "protocolinfo\r\n";

    std::string response;
    clientSocket >> response;

    std::string needle = "COOKIEFILE=";
    std::size_t pos = response.find(needle);
    if (pos == std::string::npos)
      Log::get().error("Unexpected response from Tor!");

    std::size_t pathBegin = pos + needle.size() + 1;
    std::size_t pathEnd = response.find("\"", pathBegin);
    return response.substr(pathBegin, pathEnd - pathBegin);
  }
  catch (SocketException& e)
  {
    Log::get().warn("Could not connect to Tor's control port! " +
                    e.description());
  }

  return "";
}



std::string TorManager::getCookieHash(const std::string& path)
{
  // https://stackoverflow.com/questions/2602013/
  std::fstream authFile(path);

  if (!authFile)
    Log::get().error("Unable to open cookie file!");

  std::string authBin((std::istreambuf_iterator<char>(authFile)),
                      std::istreambuf_iterator<char>());

  Botan::Pipe pipe(new Botan::Hex_Encoder(Botan::Hex_Encoder::Lowercase));
  pipe.process_msg(authBin);
  return pipe.read_all_as_string();
}



// start a child process based on the given commands
pid_t TorManager::startProcess(char** args)
{  // http://www.cplusplus.com/forum/lounge/17684/
   // http://www.thegeekstuff.com/2012/03/c-process-control-functions/

  Log::get().notice("Starting " + std::string(args[0]) + "...");
  pid_t pid = fork();

  if (pid >= 0)
  {
    if (pid == 0)
    {
      auto ret = execve(args[0], args, NULL);
      Log::get().warn("Failed to execute process. " +
                      std::string(strerror(errno)));
      return -1;
      // Pid after: " + std::to_string(pid) + "," + std::to_string(ret));
      // exit(1);
    }
    else
    {
      int status;
      Log::get().notice("Child process has pid " + std::to_string(pid));
      // wait(&status); /* wait for child to exit, and store child's exit status
      // */
      // Log::get().notice("Child exit code: " +
      // std::to_string(WEXITSTATUS(status)));
    }
  }
  else
  {
    Log::get().error("fork() failed!");
  }

  return pid;
}



// test if a localhost port is open or not
bool TorManager::isOpen(int port)
{  // https://theredblacktree.wordpress.com/2013/09/30/how-to-check-if-a-port-is-open-or-not-in-c-unixlinux/

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    Log::get().warn("Error opening socket!");
    return false;
  }

  struct hostent* server = gethostbyname("127.0.0.1");
  if (server == NULL)
  {
    Log::get().warn("ERROR, no such host");
    return false;
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy(&server->h_addr, &serv_addr.sin_addr.s_addr,
         static_cast<size_t>(server->h_length));

  serv_addr.sin_port = htons(port);
  bool isOpen = connect(sockfd, reinterpret_cast<struct sockaddr*>(&serv_addr),
                        sizeof(serv_addr)) >= 0;
  close(sockfd);
  return isOpen;
}


/*
bool TorManager::isRunning(pid_t p)
{  // http://www.unix.com/programming/121343-c-wexitstatus-question.html

  errno = 0;
  if (kill(p, 0) == 0)
    return true;
  else
  {
    if (errno == ESRCH)
      Log::get().notice("Child process " + std::to_string(p) + " has exited.");
    else
      Log::get().warn("Could not signal process " + std::to_string(p) + "!");

    return false;
  }
}
*/
