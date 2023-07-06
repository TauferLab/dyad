#ifndef STORAGE_GRAPH_INGESTER_H
#define STORAGE_GRAPH_INGESTER_H

#include "storage_view.h"
#include "ingester_methods.h"

struct ingester {
    int (*populate_storage_list)(storage_view_t*);
};
typedef struct ingester ingester_t;

ingester_t* create_ingester (ingester_methods_t mode);

void free_ingester (ingester_t** in);

#endif /* STORAGE_GRAPH_INGESTER_H */