#ifndef ACTIVITY_INSIGHT_DATABASE_MANAGER
#define ACTIVITY_INSIGHT_DATABASE_MANAGER

#include <string>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "WindowData.h"

class DatabaseManager {
 private:
    QSqlDatabase db;
 public:
    DatabaseManager() {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("activity_insight.db");
        bool ok = db.open();
        if (!ok) {
            throw std::runtime_error("Can't create and open database");
        }
    }

    void Init() {
        QSqlQuery q(db);

        // Позже можно добавить таблицу связанную с window title
        std::vector<QString> init_queries = {
            // может быть потом настроить рандомизацию color
            R"(
            CREATE TABLE IF NOT EXISTS categories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                color TEXT DEFAULT '#808080',
                is_productive BOOL DEFAULT 0
            );
        )",
        // Если такая категория уже есть, то из-за уникальности имен выражение проигнорируется
            R"(
            INSERT OR IGNORE INTO categories (name, color)
            VALUES ('Uncategorized', '#595959')
        )",
        // is_hidden на случай если юзер решит что-то скрыть
            R"(
            CREATE TABLE IF NOT EXISTS applications (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                exe_filename TEXT UNIQUE,
                display_name TEXT NOT NULL,
                is_hidden BOOL DEFAULT 0,
                category_id INTEGER DEFAULT 1,
                FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET DEFAULT
            )
        )",
            R"(
            CREATE TABLE IF NOT EXISTS activity_logs (
                app_exe_filename TEXT NOT NULL,
                log_date DATE DEFAULT (date('now', 'localtime')),
                window_title TEXT,
                time_in_seconds INTEGER NOT NULL
            )
        )",
            R"(
            CREATE TABLE IF NOT EXISTS daily_stats (
                stat_date DATE NOT NULL,
                app_id INTEGER NOT NULL,
                PRIMARY KEY (stat_date, app_id),
                FOREIGN KEY (app_id) REFERENCES applications(id)
            )
        )",
            R"(
            CREATE TABLE IF NOT EXISTS focus_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                start_time INTEGER NOT NULL,
                end_time INTEGER,
                planned_duration INTEGER,
                status TEXT CHECK( status IN ('COMPLETED', 'INTERRUPTED', 'FAILED', 'RUNNING') ) DEFAULT 'RUNNING'
            )
        )"
        };

        for (auto& query : init_queries) {
            if (!q.exec(query)) {
                qDebug() << "Error during database initialization";
                qDebug() << "Query: " << query;
                qDebug() << "Error: " << q.lastError().text();
            }
        }
    }

//    InsertActivityLog(WindowData& window_data) {
//          INSERT OR INGORE app?
//    }

};

#endif // ACTIVITY_INSIGHT_DATABASE_MANAGER