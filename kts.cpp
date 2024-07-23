

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <sqlite3.h>

#include <dlfcn.h>

#include "kts.hpp"
#include "kts_pid.hpp"
#include "kts_schema.hpp"

using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double>;
using TimePoint = std::chrono::time_point<Clock>;

namespace lib {

static uint64_t spanID = 0;
static sqlite3 *db = nullptr;
static sqlite3_stmt *spanStmt = nullptr;
static sqlite3_stmt *eventStmt = nullptr;
static TimePoint profileStart; // when the profiling library was initialized, to
                               // normalize times
static int rank = -1;

struct Span {
  int rank;
  std::string name;
  std::string kind;
  Duration start;

  Span() = default;
  Span(int _rank, const std::string &_name, const std::string &_kind,
       const Duration &_start)
      : rank(_rank), name(_name), kind(_kind), start(_start) {}
};

struct Event {
  int rank;
  std::string name;
  std::string kind;

  Event() = delete;
  Event(int _rank, const std::string &_name, const std::string &_kind)
      : rank(_rank), name(_name), kind(_kind) {}
};

static std::unordered_map<uint64_t, Span> spans;
static std::vector<Span> regions;
static const char *KIND_PARFOR = "PARALLEL_FOR";
static const char *KIND_REGION = "REGION";
static const char *KIND_DEEPCOPY = "DEEPCOPY";
static const char *KIND_FENCE = "FENCE";
static const char *KIND_ALLOC = "ALLOC";
static const char *KIND_DEALLOC = "DEALLOC";

class Worker {
public:
  Worker() : stop_(true) {}

  template <typename F> void add_job(F &&f) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      jobs_.emplace_back(std::forward<F>(f));
    }
    cv_.notify_one();
  }

  void start() {
    thread_ = std::thread(&Worker::loop, this);
    stop_ = false;
  }

  void join() {
    std::cerr << __FILE__ << ":" << __LINE__ << " flush remaining records...\n";
    {
      std::unique_lock<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cv_.notify_one();
    thread_.join();
  }

private:
  void loop() {

    // std::cerr << __FILE__ << ":" << __LINE__ << " worker loop...\n";
    while (true) {
      std::vector<std::function<void()>> jobs;
      // std::cerr << __FILE__ << ":" << __LINE__ << " worker lock entry...\n";
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !jobs_.empty() || stop_; });
        if (stop_ && jobs_.empty()) {
          return;
        }
        // quickly take all existing jobs
        std::swap(jobs, jobs_);
      }
      // std::cerr << __FILE__ << ":" << __LINE__ << " worker release... " <<
      // jobs.size() << "\n";
      for (const auto &job : jobs) {
        job();
      }
    }
  }

  std::thread thread_;
  std::vector<std::function<void()>> jobs_;
  std::condition_variable cv_;
  std::mutex mutex_;

  bool stop_;
};

static Worker worker;

static void begin_transaction() {
  char *errMsg = 0;
  int rc = sqlite3_exec(db, "BEGIN IMMEDIATE", 0, 0, &errMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "begin_transaction: SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    sqlite3_close(db);
    exit(1);
  }
}

static void commit_transaction() {
  char *errMsg = 0;
  int rc = sqlite3_exec(db, "COMMIT", 0, 0, &errMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "commit_transaction: SQL error: %s\n", errMsg);
    sqlite3_free(errMsg);
    sqlite3_close(db);
    exit(1);
  }
}

void init() {
  std::cerr << "==== libkts.so: init ====\n";
  rank = kts_mpi_rank();
  std::cerr << __FILE__ << ":" << __LINE__ << " " << rank << "\n";
  worker.start();
  const char *sqlitePrefix = std::getenv("KTS_SQLITE_PREFIX");
  if (!sqlitePrefix) {
    sqlitePrefix = "kts_";
  }

  std::string sqlitePath =
      std::string(sqlitePrefix) + std::to_string(rank) + ".sqlite";
  {
    std::cerr << __FILE__ << ":" << __LINE__ << " open " << sqlitePath << "\n";
    int rc = sqlite3_open(sqlitePath.c_str(), &db);
    if (rc) {
      std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
  }

  begin_transaction();

  // create Spans table
  {

    char *errMsg = 0;
    int rc = sqlite3_exec(db, schema::Span::create_table_sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
      std::cerr << "SQL error: " << errMsg << std::endl;
      sqlite3_free(errMsg);
    }
  }

  // create Events table
  {
    char *errMsg = 0;
    int rc = sqlite3_exec(db, schema::Event::create_table_sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
      std::cerr << "SQL error: " << errMsg << std::endl;
      sqlite3_free(errMsg);
    }
  }

  commit_transaction();

  // prepare Span insertion statement
  {
    int rc = sqlite3_prepare_v2(db, schema::Span::insert_sql, -1, &spanStmt, 0);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
                << std::endl;
      sqlite3_close(db);
      exit(1);
    }
  }

  // prepare Event insertion statement
  {
    int rc = sqlite3_prepare_v2(db, schema::Event::insert_sql, -1, &eventStmt, 0);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
                << std::endl;
      sqlite3_close(db);
      exit(1);
    }
  }

  begin_transaction();
  profileStart = Clock::now();
}

void finalize() {
  std::cerr << "==== libkts.so: finalize ====\n";

  worker.join();
  commit_transaction();
  sqlite3_finalize(spanStmt);
  sqlite3_finalize(eventStmt);
  sqlite3_close(db);
  db = nullptr;
}

static void record_span(const Span &span, Duration &&stop) {

  worker.add_job([=] {
    const char *name = span.name.c_str();
    if (!name) {
      name = "<null name>";
    }
    sqlite3_bind_int(spanStmt, 1, span.rank);
    sqlite3_bind_text(spanStmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(spanStmt, 3, span.kind.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(spanStmt, 4, span.start.count());
    sqlite3_bind_double(spanStmt, 5, stop.count());

    int rc = sqlite3_step(spanStmt);

    if (rc != SQLITE_DONE) {
      std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
      std::cerr << "Span was:"
                << " " << span.name << " " << span.kind << " "
                << span.start.count() << " " << stop.count() << "\n";
      exit(1);
    }
    sqlite3_reset(spanStmt);
  });
}

static void record_event(const Event &event, Duration &&time) {

  worker.add_job([=] {
    const char *name = event.name.c_str();
    if (!name) {
      name = "<null name>";
    }
    sqlite3_bind_int(eventStmt, 1, event.rank);
    sqlite3_bind_text(eventStmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(eventStmt, 3, event.kind.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(eventStmt, 4, time.count());

    Duration nextDelayMs = std::chrono::milliseconds(1);
    int rc = sqlite3_step(eventStmt);

    if (rc != SQLITE_DONE) {
      std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
      std::cerr << "Event was:"
                << " " << event.name << " " << event.kind << " " << time.count()
                << "\n";
      exit(1);
    }
    sqlite3_reset(eventStmt);
  });
}

// returns a unique id
uint64_t begin_parallel_for(const char *name, const uint32_t devID) {
  uint64_t kID = spanID++;
  spans[kID] = Span(
      rank, name, std::string(KIND_PARFOR) + "[" + std::to_string(devID) + "]",
      Clock::now() - profileStart);
  return kID;
}

// accepts the return value of the corresponding begin_parallel_for
void end_parallel_for(const uint64_t kID) {
  record_span(spans[kID], Clock::now() - profileStart);
  spans.erase(kID);
}

void push_profile_region(const char *name) {
  spanID++;
  regions.emplace_back(rank, name, KIND_REGION, Clock::now() - profileStart);
}
void pop_profile_region() {
  if (!regions.empty()) {
    record_span(regions.back(), Clock::now() - profileStart);
    regions.pop_back();
  }
}

void begin_deep_copy(const char *dstSpaceName, const char *dstName,
                     const void *dst_ptr, const char *srcSpaceName,
                     const char *srcName, const void *src_ptr, uint64_t size) {

  (void)dst_ptr;
  (void)srcName;
  (void)src_ptr;

  std::stringstream ss;
  ss << srcName << "[" << srcSpaceName << "]"
     << "->" << dstName << "[" << dstSpaceName << "]"
     << "(" << size << ")";
  record_event(Event(rank, ss.str(), KIND_DEEPCOPY),
               Clock::now() - profileStart);
}

// returns a unique id
uint64_t begin_fence(const char *name, const uint32_t devID) {
  uint64_t kID = spanID++;
  spans[kID] = Span(rank, name,
                    std::string(KIND_FENCE) + "[" + std::to_string(devID) + "]",
                    Clock::now() - profileStart);
  return kID;
}

// accepts the return value of the corresponding begin_fence
void end_fence(const uint64_t kID) {
  record_span(spans[kID], Clock::now() - profileStart);
  spans.erase(kID);
}

void allocate_data(const char *spaceName, const char *name, void *ptr,
                   size_t size) {
  record_event(Event(rank, name, KIND_ALLOC), Clock::now() - profileStart);
}
void deallocate_data(const char *spaceName, const char *name, void *ptr,
                     size_t size) {
  record_event(Event(rank, name, KIND_DEALLOC), Clock::now() - profileStart);
}

} // namespace lib