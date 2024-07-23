#include <algorithm>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <sqlite3.h>

#include "kts_schema.hpp"

static void help(std::ostream &os) {
  os << "Generate a chrome about://tracing json file from a saved trace";
}

// https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.yr4qxyxotyw

static const char *PHASE_INSTANT = "i";
static const char *PHASE_BEGIN = "B";
static const char *PHASE_END = "E";

struct DurationEvent {
  std::string name;
  std::string cat;
  std::string ph;
  double ts;
  int pid;

  DurationEvent(const int pid_, const char *name_, const char *cat_,
                const char *ph_, double ts_)
      : pid(pid_), name(name_), cat(cat_), ph(ph_), ts(ts_) {}
};

static void read_trace(json &eventArray, const std::string &path) {
  std::cerr << __FILE__ << ":" << __LINE__ << " open " << path << "\n";

  sqlite3 *db = nullptr;

  int rc = sqlite3_open(path.c_str(), &db);
  if (rc) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }

  auto event_callback = [](void *data, int argc, char **argv,
                           char **azColName) -> int {
    json *eventArray = static_cast<json *>(data);

    // std::string idStr = argv[0];
    const char *rankStr = argv[1];
    const char *name = argv[2];
    const char *kind = argv[3];
    const char *timeStr = argv[4];

    // time comes in as real seconds. output expects microseconds
    const double timeUs = std::atof(timeStr) * 1000000;

    const int rank = std::atoi(rankStr);

    eventArray->emplace_back(json{{"name", name},
                                  {"cat", kind},
                                  {"ph", PHASE_INSTANT},
                                  {"ts", std::to_string(timeUs).c_str()},
                                  {"pid", rank},
                                  {"args", json({})}});
    return 0;
  };

  // converts database entries into DurationEvents, which later need to be
  // sorted by timestamp before they are dumped to json
  auto span_callback = [](void *data, int argc, char **argv,
                          char **azColName) -> int {
    std::vector<DurationEvent> *durEvents =
        static_cast<std::vector<DurationEvent> *>(data);

    // std::string idStr = argv[0];
    const char *rankStr = argv[1];
    const char *name = argv[2];
    const char *kind = argv[3];
    const char *startStr = argv[4];
    const char *stopStr = argv[5];

    const int rank = std::atoi(rankStr);

    // time comes in as real seconds. output expects microseconds
    const double startUs = std::atof(startStr) * 1000000;
    const double stopUs = std::atof(stopStr) * 1000000;

    durEvents->emplace_back(rank, name, kind, PHASE_BEGIN, startUs);
    durEvents->emplace_back(rank, name, kind, PHASE_END, stopUs);
    return 0;
  };

  std::cerr << __FILE__ << ":" << __LINE__ << " convert events\n";
  {
    // pass eventArray in as void * userData
    char *errMsg;
    rc = sqlite3_exec(db, "SELECT * FROM Events;", event_callback, &eventArray,
                      &errMsg);
    if (rc) {
      std::cerr << "Can't exec: " << errMsg << std::endl;
      exit(1);
    }
  }
  std::cerr << __FILE__ << ":" << __LINE__ << " read spans\n";
  std::vector<DurationEvent> durEvents;
  {
    // pass eventArray in as void * userData
    char *errMsg;
    rc = sqlite3_exec(db, "SELECT * FROM Spans;", span_callback, &durEvents,
                      &errMsg);
    if (rc) {
      std::cerr << "Can't exec: " << errMsg << std::endl;
      exit(1);
    }
  }
  std::cerr << __FILE__ << ":" << __LINE__ << " sort spans\n";
  std::sort(durEvents.begin(), durEvents.end(),
            [](const DurationEvent &a, const DurationEvent &b) {
              return a.ts < b.ts;
            });
  std::cerr << __FILE__ << ":" << __LINE__ << " convert spans\n";
  for (const DurationEvent &de : durEvents) {
    eventArray.emplace_back(json{{"name", de.name},
                                 {"cat", de.cat},
                                 {"ph", de.ph},
                                 {"ts", std::to_string(de.ts).c_str()},
                                 {"pid", de.pid}});
  }

  sqlite3_close(db);
  db = nullptr;
}

int main(int argc, char **argv) {

  json traceEvents = json::array();

  for (int i = 1; i < argc; ++i) {
    read_trace(traceEvents, argv[i]);
  }

  std::cerr << __FILE__ << ":" << __LINE__ << " dump JSON...\n";
  json j;
  j["traceEvents"] = traceEvents;
  j["displayTimeUnit"] = "ms";
  j["otherData"] = json{{"version", "kts chrome-tracing"}};
  j["stackFrames"] = json({});
  j["samples"] = json::array();
  std::ofstream("test.json", std::ios::binary) << std::setw(4) << j << "\n";
}