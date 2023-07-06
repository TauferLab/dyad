#ifndef LINUX_MOUNTINFO_INGESTER_H
#define LINUX_MOUNTINFO_INGESTER_H

#include "storage_view.h"

int populate_from_linux_proc_mounts (storage_view_t* view);

#endif /* LINUX_MOUNTINFO_INGESTER_H */