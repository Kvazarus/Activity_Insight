#ifndef ACTIVITY_INSIGHT_APPSTATS
#define ACTIVITY_INSIGHT_APPSTATS

#include <string>

struct AppStats {
    QString exe_filename;
    QString display_name;
    bool is_hidden;
    int category_id;
    int64_t total_time;
};

#endif //ACTIVITY_INSIGHT_APPSTATS
