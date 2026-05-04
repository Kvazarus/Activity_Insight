#ifndef ACTIVITY_INSIGHT_SRC_FOCUSSESSION_H_
#define ACTIVITY_INSIGHT_SRC_FOCUSSESSION_H_

#include <cstdint>

struct FocusSession {
    int category_id;
    int64_t session_datetime;
    int duration;
};

#endif //ACTIVITY_INSIGHT_SRC_FOCUSSESSION_H_
