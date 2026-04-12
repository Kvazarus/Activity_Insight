#ifndef ACTIVITY_INSIGHT_SRC_CATEGORY_H_
#define ACTIVITY_INSIGHT_SRC_CATEGORY_H_

#include <QString>

struct Category {
    int id;
    QString name;
    QString color;
    bool is_productive;
};

#endif //ACTIVITY_INSIGHT_SRC_CATEGORY_H_
