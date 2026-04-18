#include "DatabaseManager.h"

#include <string>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("activity_insight.db");
    bool ok = db.open();
    if (!ok) {
        throw std::runtime_error("Can't create and open database");
    }
    QSqlQuery q(db);
    q.exec("PRAGMA foreign_keys = ON;");
}

void DatabaseManager::init() {
    QSqlQuery q(db);

    // Позже можно добавить таблицу связанную с window title
    std::vector<QString> init_queries = {
        // может быть потом настроить рандомизацию color
        R"(
            CREATE TABLE IF NOT EXISTS categories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                color TEXT DEFAULT '#808080',
                is_productive BOOL DEFAULT 1
            );
        )",
        // is_hidden на случай если юзер решит что-то скрыть
        R"(
            CREATE TABLE IF NOT EXISTS applications (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                exe_filename TEXT UNIQUE NOT NULL,
                display_name TEXT NOT NULL,
                is_hidden BOOL DEFAULT 0,
                category_id INTEGER DEFAULT 1,
                FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET DEFAULT
            );
        )",
        R"(
            CREATE TABLE IF NOT EXISTS activity_logs (
                app_exe_filename TEXT NOT NULL,
                log_date DATE DEFAULT (date('now', 'localtime')),
                window_title TEXT,
                time_in_seconds INTEGER NOT NULL
            );
        )",
        R"(
            CREATE TABLE IF NOT EXISTS daily_stats (
                stat_date DATE NOT NULL,
                app_id INTEGER NOT NULL,
                total_time INTEGER NOT NULL,
                PRIMARY KEY (stat_date, app_id),
                FOREIGN KEY (app_id) REFERENCES applications(id)
            );
        )",
        R"(
            CREATE TABLE IF NOT EXISTS focus_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                start_time INTEGER NOT NULL,
                end_time INTEGER,
                planned_duration INTEGER,
                status TEXT CHECK( status IN ('COMPLETED', 'INTERRUPTED', 'FAILED', 'RUNNING') ) DEFAULT 'RUNNING'
            );
        )"
    };

    for (auto &query : init_queries) {
        if (!q.exec(query)) {
            qDebug() << "Error during database initialization";
            qDebug() << "Query: " << query;
            qDebug() << "Error: " << q.lastError().text();
        }
    }

    // Проверка на самый первый запуск программы
    if (q.exec("SELECT count(*) from categories")) {
        if (q.next() && q.value(0).toInt() == 0) {
            if (!q.exec(R"(
                INSERT INTO categories (name, color, is_productive)
                VALUES ('Uncategorized', '#9E9E9E', 1),
                       ('Work', '#2196F3', 1),
                       ('Learning', '#4CAF50', 1),
                       ('Entertainment', '#FF9800', 0)
            )")) {
                qDebug() << "Error during database initialization";
                qDebug() << "Error: " << q.lastError().text();
            }
        }
    } else {
        qDebug() << "Error during database initialization";
        qDebug() << "Error: " << q.lastError().text();
    }
}

void DatabaseManager::insertActivityLog(const WindowData &window_data) {
    QSqlQuery q(db);

    q.prepare("INSERT OR IGNORE INTO applications (exe_filename, display_name) "
              "VALUES (:exe_filename, :display_name)");
    q.bindValue(":exe_filename", QString::fromStdWString(window_data.exe_filename));
    q.bindValue(":display_name", QString::fromStdWString(window_data.display_name));
    if (!q.exec()) {
        qDebug() << "Error during insertion a new application";
    }

    q.prepare("INSERT INTO activity_logs (app_exe_filename, window_title, time_in_seconds) "
              "VALUES (:exe_filename, :window_title, :time)");
    q.bindValue(":exe_filename", QString::fromStdWString(window_data.exe_filename));
    q.bindValue(":window_title", QString::fromStdWString(window_data.window_title));
    q.bindValue(":time", window_data.time);
    if (!q.exec()) {
        qDebug() << "Error during insertion an activity log";
    }
}

void DatabaseManager::updateDailyStats() {
    if (db.transaction()) {
        QSqlQuery q(db);

        q.prepare(R"(
            INSERT INTO daily_stats (stat_date, app_id, total_time)
            SELECT
                al.log_date,
                app.id,
                SUM(al.time_in_seconds)
            FROM activity_logs al
            JOIN applications app ON al.app_exe_filename = app.exe_filename
            GROUP BY al.log_date, app.id
            ON CONFLICT (stat_date, app_id)
            DO UPDATE SET total_time = daily_stats.total_time + excluded.total_time;
        )");

        if (!q.exec()) {
            qDebug() << "Error when updating daily stats";
            db.rollback();
            return;
        }

        q.prepare("DELETE FROM activity_logs");
        if (!q.exec()) {
            qDebug() << "Error when truncating activity logs";
            db.rollback();
            return;
        }

        if (!db.commit()) {
            qDebug() << "Failed to commit";
            db.rollback();
        }
    } else {
        qDebug() << "Failed to start a transaction";
    }
}

std::vector<AppStats> DatabaseManager::getUpdatedDailyAppStats(const std::string& date_from, const std::string& date_to) {
    updateDailyStats();

    QString q_date_from;
    QString q_date_to;
    if (date_from == "today") {
        q_date_from = QDate::currentDate().toString("yyyy-MM-dd");
    } else {
        q_date_from = QString::fromStdString(date_from);
    }
    if (date_to == "today") {
        q_date_to = QDate::currentDate().toString("yyyy-MM-dd");
    } else {
        q_date_to = QString::fromStdString(date_to);
    }

    QSqlQuery q(db);

    q.prepare(R"(
        SELECT a.exe_filename, a.display_name, a.is_hidden, a.category_id, SUM(ds.total_time) as total_time_sum
        FROM daily_stats ds
        JOIN applications a ON a.id = ds.app_id
        WHERE NOT a.is_hidden AND ds.stat_date BETWEEN ? AND ?
        GROUP BY ds.app_id
        ORDER BY total_time_sum DESC
    )");
    q.addBindValue(q_date_from);
    q.addBindValue(q_date_to);
    if (!q.exec()) {
        qDebug() << "Failed to get AppStats data: " << q.lastError().text();
        return {};
    }

    std::vector<AppStats> res;
    while (q.next()) {
        res.push_back({q.value(0).toString(), q.value(1).toString(),
                         q.value(2).toBool(), q.value(3).toInt(), q.value(4).toLongLong()});
    }
    return res;
}

std::vector<std::pair<int64_t, Category>> DatabaseManager::getUpdatedDailyCategoriesStats(const std::string &date_from,
                                                                      const std::string &date_to) {
    updateDailyStats();

    QString q_date_from;
    QString q_date_to;
    if (date_from == "today") {
        q_date_from = QDate::currentDate().toString("yyyy-MM-dd");
    } else {
        q_date_from = QString::fromStdString(date_from);
    }
    if (date_to == "today") {
        q_date_to = QDate::currentDate().toString("yyyy-MM-dd");
    } else {
        q_date_to = QString::fromStdString(date_to);
    }

    QSqlQuery q(db);

    q.prepare(R"(
        SELECT c.id, c.name, c.color, c.is_productive, SUM(ds.total_time) as total_time_sum
        FROM daily_stats ds
        JOIN applications a ON a.id = ds.app_id
        JOIN categories c ON c.id = a.category_id
        WHERE NOT a.is_hidden AND ds.stat_date BETWEEN ? AND ?
        GROUP BY c.id, c.name, c.color, c.is_productive
        ORDER BY total_time_sum DESC
    )");
    q.addBindValue(q_date_from);
    q.addBindValue(q_date_to);
    if (!q.exec()) {
        qDebug() << "Failed to get Categories stats data: " << q.lastError().text();
        return {};
    }

    std::vector<std::pair<int64_t, Category>> res;
    while (q.next()) {
        res.push_back({q.value(4).toInt(), {q.value(0).toInt(), q.value(1).toString(),
                                              q.value(2).toString(), q.value(3).toBool()}});
    }
    return res;
}



std::unordered_map<int, Category> DatabaseManager::getCategories() {
    std::unordered_map<int, Category> categories;
    QSqlQuery q(db);

    if (!q.exec("SELECT id, name, color, is_productive FROM categories")) {
        qDebug() << "Error when getting categories";
        db.rollback();
        return {};
    }

    while (q.next()) {
        categories[q.value(0).toInt()] = {q.value(0).toInt(), q.value(1).toString(),
                       q.value(2).toString(), q.value(3).toBool()};
    }
    return categories;
}

std::vector<int64_t> DatabaseManager::getWeekUpdatedDailyStats(QDate &date_from, QDate &date_to) {
    updateDailyStats();

    if (date_from.daysTo(date_to) != 6) {
        qDebug() << "Gap between dates does not equal week";
        return std::vector<int64_t>(7, 0);
    }

    QSqlQuery q(db);

    q.prepare(R"(
        SELECT ds.stat_date, SUM(ds.total_time) as total_time_sum
        FROM daily_stats ds
        JOIN applications a ON a.id = ds.app_id
        WHERE NOT a.is_hidden AND ds.stat_date BETWEEN ? AND ?
        GROUP BY ds.stat_date
        ORDER BY ds.stat_date ASC
    )");
    q.addBindValue(date_from.toString("yyyy-MM-dd"));
    q.addBindValue(date_to.toString("yyyy-MM-dd"));
    if (!q.exec()) {
        qDebug() << "Failed to get week daily data: " << q.lastError().text();
        return std::vector<int64_t>(7, 0);
    }

    std::vector<int64_t> res(7, 0);
    while (q.next()) {
        QDate date = q.value(0).toDate();
        int64_t index = date_from.daysTo(date);

        if (index < 0 || index >= 7) {
            qDebug() << "Wrong index week daily stats data";
            return std::vector<int64_t>(7, 0);
        }

        res[index] = q.value(1).toLongLong();
    }
    return res;
}

std::vector<int64_t> DatabaseManager::getWeekUpdatedAppStats(const QString &exe_filename, QDate &date_from, QDate &date_to) {
    updateDailyStats();

    if (date_from.daysTo(date_to) != 6) {
        qDebug() << "Gap between dates does not equal week";
        return std::vector<int64_t>(7, 0);
    }

    QSqlQuery q(db);

    q.prepare(R"(
        SELECT ds.stat_date, ds.total_time
        FROM daily_stats ds
        JOIN applications a ON a.id = ds.app_id
        WHERE a.exe_filename = ? AND ds.stat_date BETWEEN ? AND ?
        ORDER BY ds.stat_date ASC
    )");
    q.addBindValue(exe_filename);
    q.addBindValue(date_from.toString("yyyy-MM-dd"));
    q.addBindValue(date_to.toString("yyyy-MM-dd"));
    if (!q.exec()) {
        qDebug() << "Failed to get week app data: " << q.lastError().text();
        return std::vector<int64_t>(7, 0);
    }

    std::vector<int64_t> res(7, 0);
    while (q.next()) {
        QDate date = q.value(0).toDate();
        int64_t index = date_from.daysTo(date);

        if (index < 0 || index >= 7) {
            qDebug() << "Wrong index week app data";
            return std::vector<int64_t>(7, 0);
        }
        res[index] = q.value(1).toLongLong();
    }
    return res;
}

void DatabaseManager::updateAppInfo(const AppStats &app_stats) {
    QSqlQuery q(db);

    q.prepare(R"(
        UPDATE applications
        SET display_name = ?, is_hidden = ?, category_id = ?
        WHERE exe_filename = ?
    )");
    q.addBindValue(app_stats.display_name);
    q.addBindValue(app_stats.is_hidden);
    q.addBindValue(app_stats.category_id);
    q.addBindValue(app_stats.exe_filename);

    if (!q.exec()) {
        qDebug() << "Failed to update App info: " << q.lastError().text();
    }
}

std::vector<std::pair<QString, QString>> DatabaseManager::getHiddenApps() {
    QSqlQuery q(db);

    q.prepare(R"(
        SELECT exe_filename, display_name
        FROM applications
        WHERE is_hidden = 1
    )");

    if (!q.exec()) {
        qDebug() << "Error when getting hidden apps";
        return {};
    }

    std::vector<std::pair<QString, QString>> hidden_apps;
    while (q.next()) {
        hidden_apps.emplace_back(q.value(0).toString(), q.value(1).toString());
    }
    return hidden_apps;
}

void DatabaseManager::restoreHiddenApp(const QString &exe_filename) {
    QSqlQuery q(db);

    q.prepare(R"(
        UPDATE applications
        SET is_hidden = 0
        WHERE exe_filename = ?
    )");
    q.addBindValue(exe_filename);

    if (!q.exec()) {
        qDebug() << "Error when restoring hidden app";
    }
}
