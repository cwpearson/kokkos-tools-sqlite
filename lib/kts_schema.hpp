#pragma once

#include <functional>

namespace schema {
    struct Event {
        static constexpr const char * create_table_sql = "CREATE TABLE IF NOT EXISTS Events("
                                 "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "Rank INTEGER NOT NULL,"
                                 "Name TEXT NOT NULL,"
                                 "Kind TEXT NOT NULL,"
                                 "Time REAL NOT NULL);";
        static constexpr const char * insert_sql = "INSERT INTO Events (Rank, Name, Kind, Time) VALUES (?, ?, ?, ?);";

    };


    struct Span {
        static constexpr const char * create_table_sql = "CREATE TABLE IF NOT EXISTS Spans("
                                 "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "Rank INTEGER NOT NULL,"
                                 "Name TEXT NOT NULL,"
                                 "Kind TEXT NOT NULL,"
                                 "Start REAL NOT NULL,"
                                 "Stop REAL NOT NULL);";

        static constexpr const char * insert_sql = "INSERT INTO Spans (Rank, Name, Kind, Start, Stop) VALUES (?, ?, ?, ?, ?);";

        int rank;
        std::string name;
        std::string kind;
        double time;

    };


    using SpanCallback = std::function<int(const Span &span)>;



    template <typename Callback>
    void for_all_spans(sqlite3 *db, Callback &&c) {

        auto callback = [](void *data, int argc, char** argv, char** azColName) -> int {
            auto f = static_cast<std::function<int(const Span &span)> *>(data);
            const char *rankStr = argv[1];
            const char *name = argv[2];
            const char *kind = argv[3];
            const char *timeStr = argv[4];
            return (*f)(Span{std::atoi(rankStr),name,kind,std::atof(timeStr)});
        };

        // wrap in an std::function so callback knows what to cast userdata to
        std::function<int(const Span &span)> wrapped(c);

        char *errMsg;
        int rc = sqlite3_exec(db, "SELECT * FROM Spans;", callback, &wrapped,
                        &errMsg);
        if (rc) {
        std::cerr << "Can't exec: " << errMsg << std::endl;
        exit(1);
        }

    }
}