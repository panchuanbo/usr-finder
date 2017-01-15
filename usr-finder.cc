#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <algorithm>

#define BUFFER_SIZE 256

using namespace std;

static vector<string> usrMachines;
static mutex usrMachinesLock;

void findUser(const string &machine, const string &usr) {
  string cmd = "ssh -o ConnectTimeout=5 -q " + machine + " w -hus | awk '{print $1;}' | grep \"\\b" + usr + "\\b\"";
  FILE *output = popen(cmd.c_str(), "r");
  if (!output) return;

  while (true) {
    char buffer[BUFFER_SIZE];
    int res = fscanf(output, "%s", buffer);
    if (res == -1) { // basically check if file is empty or not
      printf("User %s not on Machine %s.\n", usr.c_str(), machine.c_str());
      break;
    }
    usrMachinesLock.lock();
    usrMachines.push_back(machine);
    usrMachinesLock.unlock();
    printf("Found %s on Machine %s.\n", usr.c_str(), machine.c_str()); // for thread-safety
    break;
  }
  pclose(output);
}

string getServer() {
  string response;
  for (size_t i = 0; i < usrMachines.size(); i++) {
    cout << i << ". " << usrMachines[i] << endl;
  }
  cout << "Which server? ";
  cin >> response;
  int index = atoi(response.c_str());
  while (index < 0 || index >= (int)usrMachines.size()) {
    cout << "invalid server, try again: ";
    cin >> response;
    index = atoi(response.c_str());
  }
  return usrMachines[index];
}

bool getYesNo() {
  string response;
  cin >> response;
  if (toupper(response[0]) == 'Y') return true;
  else return false;
}

string getMessage() {
  string message;
  cout << "What do you want to send? ";
  getline(cin, message);
  return message;
}

// argv[1] = list of machines
// argv[2] = user
int main(int argc, char** argv) {

  if (argc < 3) {
    cout << "Usage: ./usr-finder <machines txt file> <usr id>" << endl;
    return 0;
  }

  ifstream fin(argv[1]);
  string name = string(argv[2]);

  vector<string> machines;

  // not going to threadpool right now because the only TP I have is in the form of a completed
  // assignment that's still in use at my school; don't want to violate any honor codes here :)
  // so with that in mind, either use your own TP or make sure the # of machines is reasonable
  vector<thread> threads;

  string line;
  while (getline(fin, line)) machines.push_back(line);

  for (const string &m : machines) threads.push_back(thread(findUser, m, name));
  for (thread &t : threads) t.join();

  if (usrMachines.size() != 0) {
    cout << name << " is logged on! Would you like to send " << name << " a message? ";
    if (getYesNo()) {
      string server = getServer();
      cout << "Begin writing messages below. Ctrl+D to Quit." << endl;
      string cmd = "ssh -o ConnectTimeout=5 -q " + server + " write " + name;
      cout << cmd << endl;
      system(cmd.c_str());
    }
  } else {
    cout << name << " is probably not logged in/on another server :(" << endl;
  }

  return 0;
}
