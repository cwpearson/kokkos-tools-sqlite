

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <string>

#include <sqlite3.h>

#include "lib.hpp"

using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double>;
using TimePoint = std::chrono::time_point<Clock>;


namespace lib {

static uint64_t spanID = 0;


static sqlite3* db = nullptr;
static sqlite3_stmt* spanStmt = nullptr;


struct Span {
    std::string name;
    std::string kind;
    TimePoint start;

    Span() = default;
    Span(const std::string &_name, const std::string &_kind, const TimePoint &_start) : name(_name), kind(_kind), start(_start) {}
};


static std::unordered_map<uint64_t, Span> spans;
static const char * KIND_PARFOR = "PARALLEL_FOR";



void init() {
    std::cerr << __FILE__ << ":" << __LINE__ << " init\n";

    {
        int rc = sqlite3_open("kokkos.sqlite", &db);
        if (rc) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            exit(1);
        }
    }

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


    {
        const char* insertSQL = "INSERT INTO Users (Name, Kind, Start, Stop) VALUES (?, ?, ?, ?)";
        int rc = sqlite3_prepare_v2(db, insertSQL, -1, &spanStmt, 0);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            exit(1);
        }
    }


}

void finalize() {
    std::cerr << __FILE__ << ":" << __LINE__ << " finalize\n";
    sqlite3_finalize(spanStmt); // could move this to finalize
    sqlite3_close(db);
    db = nullptr;
}

// returns a unique id
uint64_t begin_parallel_for(const char* name, const uint32_t devID) {
    uint64_t kID = spanID++;
    spans[kID] = Span(name, std::string(KIND_PARFOR) + " " + std::to_string(devID), Clock::now());
    return kID;
}

// accepts the return value of the corresponding begin_parallel_for
void end_parallel_for(const uint64_t kID) {

    const TimePoint stop = Clock::now();
    const Span &span = spans[kID];



    sqlite3_bind_text(spanStmt, 1, span.kind.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(spanStmt, 2, span.kind.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(spanStmt, 3, span.start.time_since_epoch().count());
    sqlite3_bind_double(spanStmt, 4, stop.time_since_epoch().count());
    int rc = sqlite3_step(spanStmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Record inserted successfully" << std::endl;
    }
    sqlite3_reset(spanStmt);

    

    spans.erase(kID);
}

}