#include <bits/stdc++.h>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
using namespace std;

// global float to store uptime
float uptime;
float memTotal;

// struct to contain the unprocessed data
struct Process {
  string sheet[52];
};

// vector to store unprocessed processes
vector<Process> process_vector;

// struct to contain processed data
struct Output {
  u_int PID;
  string command;
  char state;
  float percCPU;
  float percMem;
  u_long VSZ;
  u_long RSS;
  u_int CPU;
};

// vector to output processed data
vector<Output> ps;

// Overloaded compare operator returns true if p1's pid is less than p2
bool compPID(Output &p1, Output &p2) {
  if (p1.PID < p2.PID)
    return true;
  return false;
}

// Overloaded compare operator returns true if p1's CPU usage is greater than p2
bool compCPU(Output &p1, Output &p2) {
  if (p1.percCPU > p2.percCPU)
    return true;
  return false;
}

// Overloaded compare operator returns true if p1's mem usage is greater than p2
bool compMem(Output &p1, Output &p2) {
  if (p1.percMem > p2.percMem)
    return true;
  return false;
}

// Overloaded compare operator returns true if p1's command ascii value is less
// than p2
bool compCom(Output &p1, Output &p2) {
  if (p1.command < p2.command)
    return true;
  return false;
}

// processLine takes a string and processes it to be added into a process sheet
// array.
void processLine(string line) {
  Process proc;
  // This for loop helps process the line by
  // deleting characters that make it more difficult
  // to be parsed: '(', ')' and when a command contains
  // a space it will be changed ' ' -> '-' (line 69 & 70)
  for (u_int i = 0; i < line.length(); ++i) {
    if (line[i] == '(') {
      line.erase(i, 1);
      int counter = 0;
      do {
        if (line[i + counter] == ' ') {
          line[i + counter] = '-';
        }
        counter++;
      } while (line[i + counter] != ')');
    }
    if (line[i] == ')') {
      line.erase(i, 1);
    }
  }

  // This stringstream parses the line by space and adds then into
  // a structure called proc containing an array of strings sized 52
  // that process is then pushed onto the process vector
  istringstream iss(line);
  int counter = 0;
  for (string s; iss >> s;) {
    proc.sheet[counter] = s;
    counter++;
  }
  process_vector.push_back(proc);
}

int main(int argc, const char *argv[]) {
  do {
    // reset process_vector, ps and uptime
    process_vector.resize(0);
    ps.resize(0);
    uptime = 0;
    // initialize file, directory and other related information
    FILE *file = NULL;
    char *line = NULL;
    size_t line_buf_size = 0;
    ssize_t line_size;
    DIR *dp = opendir("/proc");
    struct dirent *ep;
    // makes sure the directory was successfuly opened
    if (dp != NULL) {
      // while this is a folder available to read
      while ((ep = readdir(dp))) {
        long int pid = strtol(ep->d_name, NULL, 10);
        if ((ep->d_type == DT_DIR) && (pid > 0)) {
          // printf("directory name: %s\n", ep->d_name);
          file = fopen(("/proc/" + string(ep->d_name) + "/stat").c_str(), "r");
          if (file == NULL) {
            perror("Error opening file");
          } else {
            // read line
            while ((line_size = getline(&line, &line_buf_size, file)) != -1) {
              // printf("Retrieved line of length %zu:\n", line_size);
              // printf("%s", line);
              processLine(line);
            }
            fclose(file);
          }
        }
      }
      closedir(dp);
      file = fopen("/proc/uptime", "r");
      if (file == NULL) {
        perror("Error opening file");
      } else {
        while ((line_size = getline(&line, &line_buf_size, file)) != -1) {

          istringstream iss(line);
          int counter = 0;
          for (string s; iss >> s;) {
            if (counter == 0) {
              uptime = atoi(s.c_str());
            }
            counter++;
          }
        }
      }
      file = fopen("/proc/meminfo", "r");
      if (file == NULL) {
        perror("Error opening file");
      } else {
        while ((line_size = getline(&line, &line_buf_size, file)) != -1) {

          istringstream iss(line);
          int counter = 0;
          for (string s; iss >> s;) {
            if (counter == 0) {
              memTotal = atoi(s.c_str());
            }
            counter++;
          }
        }
      }
    } else {
      perror("Couldn't open the directory");
      exit(-1);
    }

    long phys_pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    long phys_mem_size = phys_pages * page_size;

    for (int i = 0; i < process_vector.size(); ++i) {
      Output processOutput;
      float process_time;
      float real_time;
      processOutput.PID = atoi(process_vector[i].sheet[0].c_str());
      processOutput.command = process_vector[i].sheet[1];
      processOutput.state = process_vector[i].sheet[2][0];
      processOutput.VSZ = atoi(process_vector[i].sheet[22].c_str());
      processOutput.RSS = atoi(process_vector[i].sheet[23].c_str());
      processOutput.CPU = atoi(process_vector[i].sheet[38].c_str());
      processOutput.percMem = (float)(processOutput.RSS * getpagesize() * 100) /
                              (float)phys_mem_size;
      process_time =
          (atoi(process_vector[i].sheet[13].c_str()) / sysconf(_SC_CLK_TCK)) +
          (atoi(process_vector[i].sheet[14].c_str()) / sysconf(_SC_CLK_TCK));
      real_time = uptime - (atoi(process_vector[i].sheet[21].c_str()) /
                            sysconf(_SC_CLK_TCK));
      processOutput.percCPU = process_time * 100 / real_time;
      ps.push_back(processOutput);
    }

    string arg = (string)argv[1];
    if (arg == "-cpu") {
      sort(ps.begin(), ps.end(), compCPU);
    } else if (arg == "-mem") {
      sort(ps.begin(), ps.end(), compMem);
    } else if (arg == "-pid") {
      sort(ps.begin(), ps.end(), compPID);
    } else if (arg == "-com") {
      sort(ps.begin(), ps.end(), compCom);
    } else {
      cout << "Error: invalid argument(s)." << endl;
      return (-1);
    }
    int running = 0;
    float totalCPU = 0;
    for (int q = 0; q < ps.size(); ++q) {
      if (ps[q].state == 'R') {
        running++;
      }
      totalCPU += ps[q].percCPU;
    }

    struct sysinfo si;
    sysinfo(&si);
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    cout << "[" << ps.size() << " total no. proccesses][" << running
         << " proccesses running][" << si.totalram << " physical memory]["
         << si.totalram - si.freeram << " memory used][" << totalCPU
         << " total % used]" << endl;
    cout << setw(6) << "PID" << setw(30) << "COMMAND" << setw(6) << "STATE"
         << setw(15) << "%CPU" << setw(15) << "%MEM" << setw(22) << "VSZ"
         << setw(10) << "RSS" << setw(6) << "CORE";
    cout << endl;
    for (u_int i = 0; i < w.ws_row - 3; ++i) {
      cout << setw(6) << ps[i].PID;
      cout << setw(30) << ps[i].command;
      cout << setw(6) << ps[i].state;
      cout << setw(15) << ps[i].percCPU;
      cout << setw(15) << ps[i].percMem;
      cout << setw(22) << ps[i].VSZ;
      cout << setw(10) << ps[i].RSS;
      cout << setw(6) << ps[i].CPU;
      cout << endl;
    }
    sleep(1);
    system("clear");
  } while (1);
}
