

#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <string>
#include <sstream>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <sqlite3.h>

#include "kts.hpp"

using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double>;
using TimePoint = std::chrono::time_point<Clock>;


namespace lib {

static uint64_t spanID = 0;
static sqlite3* db = nullptr;
static sqlite3_stmt* spanStmt = nullptr;
static sqlite3_stmt* eventStmt = nullptr;


struct Span {
    std::string name;
    std::string kind;
    TimePoint start;

    Span() = default;
    Span(const std::string &_name, const std::string &_kind, const TimePoint &_start) : name(_name), kind(_kind), start(_start) {}
};

struct Event {
    std::string name;
    std::string kind;

    Event() = delete;
    Event(const std::string &_name, const std::string &_kind) : name(_name), kind(_kind) {}
};

class Worker {
public:

    Worker() : done_(false) {
        std::cerr << __FILE__ << ":" << __LINE__ << " starting worker...\n";
        thread_ = std::thread(&Worker::process_queue, this);
    }

    ~Worker() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            std::cerr << __FILE__ << ":" << __LINE__ << " draining " << jobs.size() << " records...\n";
            done_ = true;
            condition.notify_one();
        }
        thread_.join();
    }

    void add_job(std::function<void()> job) {
        std::unique_lock<std::mutex> lock(mutex);
        jobs.push(job);
        condition.notify_one();
    }

private:
    void process_queue() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this] { return !jobs.empty() || done_; });
                if (done_ && jobs.empty()) {
                    return;
                }
                job = jobs.front();
                jobs.pop();
            }
            job(); // Execute job
        }
    }


    std::thread thread_;
    std::queue<std::function<void()>> jobs;
    std::mutex mutex;
    std::condition_variable condition;
    bool done_;

};

static std::unordered_map<uint64_t, Span> spans;
static std::vector<Span> regions;
static const char * KIND_PARFOR = "PARALLEL_FOR";
static const char * KIND_REGION = "REGION";
static const char * KIND_DEEPCOPY = "DEEPCOPY";

static std::unique_ptr<Worker> worker;

void init() {
    std::cerr << __FILE__ << ":" << __LINE__ << " init\n";
    worker = std::make_unique<Worker>();

    const char* sqlitePath = std::getenv("KTS_SQLITE_PATH");
    if (!sqlitePath) {
        sqlitePath = "kts.sqlite";
    }


    {
        int rc = sqlite3_open(sqlitePath, &db);
        if (rc) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            exit(1);
        }
    }

    // create Spans table
    {
        const char* createTableSQL = "CREATE TABLE IF NOT EXISTS Spans("
                                    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "Name TEXT NOT NULL,"
                                    "Kind TEXT NOT NULL,"
                                    "Start REAL NOT NULL,"
                                    "Stop REAL NOT NULL);";

        char* errMsg = 0;
        int rc = sqlite3_exec(db, createTableSQL, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        } else {
            std::cout << "Table created successfully" << std::endl;
        }
    }

    // create Events table
    {
        const char* createTableSQL = "CREATE TABLE IF NOT EXISTS Events("
                                    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "Name TEXT NOT NULL,"
                                    "Kind TEXT NOT NULL,"
                                    "Time REAL NOT NULL);";

        char* errMsg = 0;
        int rc = sqlite3_exec(db, createTableSQL, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        } else {
            std::cout << "Table created successfully" << std::endl;
        }
    }

    // prepare Span insertion statement
    {
        const char* insertSQL = "INSERT INTO Spans (Name, Kind, Start, Stop) VALUES (?, ?, ?, ?);";
        int rc = sqlite3_prepare_v2(db, insertSQL, -1, &spanStmt, 0);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            exit(1);
        }
    }

    // prepare Event insertion statement
    {
        const char* insertSQL = "INSERT INTO Events (Name, Kind, Time) VALUES (?, ?, ?);";
        int rc = sqlite3_prepare_v2(db, insertSQL, -1, &eventStmt, 0);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            exit(1);
        }
    }

    // begin transaction
    {
        char* errMsg = 0;
        int rc = sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", errMsg);
            sqlite3_free(errMsg);
            sqlite3_close(db);
            exit(1);
        }
    }

}

void finalize() {
    // drain worker
    worker = nullptr;

    // commit transaction
    {
        char* errMsg = 0;
        int rc = sqlite3_exec(db, "COMMIT", 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", errMsg);
            sqlite3_free(errMsg);
            sqlite3_close(db);
            exit(1);
        }
    }

    sqlite3_finalize(spanStmt);
    sqlite3_finalize(eventStmt);
    sqlite3_close(db);
    db = nullptr;
}

// returns a unique id
uint64_t begin_parallel_for(const char* name, const uint32_t devID) {
    uint64_t kID = spanID++;
    spans[kID] = Span(name, std::string(KIND_PARFOR) + " " + std::to_string(devID), Clock::now());
    return kID;
}

static void record_span(const Span &span, TimePoint &&stop) {
    worker->add_job([=](){
        sqlite3_bind_text(spanStmt, 1, span.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(spanStmt, 2, span.kind.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(spanStmt, 3, span.start.time_since_epoch().count());
        sqlite3_bind_double(spanStmt, 4, stop.time_since_epoch().count());
        int rc = sqlite3_step(spanStmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
            exit(1);
        }
        sqlite3_reset(spanStmt);
    });
}

static void record_event(const Event &event, const TimePoint &time) {
    worker->add_job([=](){
        sqlite3_bind_text(eventStmt, 1, event.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(eventStmt, 2, event.kind.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(eventStmt, 3, time.time_since_epoch().count());
        int rc = sqlite3_step(eventStmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
            exit(1);
        }
        sqlite3_reset(eventStmt);
    });
}

// accepts the return value of the corresponding begin_parallel_for
void end_parallel_for(const uint64_t kID) {
    record_span(spans[kID], Clock::now());
    spans.erase(kID);
}


void push_profile_region(const char *name) {
    spanID++;
    regions.emplace_back(name, KIND_REGION, Clock::now());
}
void pop_profile_region() {
    if (!regions.empty()) {
        record_span(regions.back(), Clock::now());
        regions.pop_back();
    }
}

void begin_deep_copy(const char *dstSpaceName, const char *dstName, const void *dst_ptr, const char *srcSpaceName, const char *srcName, const void *src_ptr, uint64_t size) {

    (void) dst_ptr;
    (void) srcName;
    (void) src_ptr;

    std::stringstream ss;
    ss 
      << srcName
      << "[" << srcSpaceName << "]"
      << "->" << dstName
      << "[" << dstSpaceName << "]"
      << "(" << size << ")";
    record_event(Event(ss.str(), KIND_DEEPCOPY), Clock::now());
}

}