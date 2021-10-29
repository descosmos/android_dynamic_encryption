#ifndef MINIDL_SRC_LOG_H
#define MINIDL_SRC_LOG_H

#include <stdio.h>

#define LOGI(format, args...)   printf("[Info]  " format "\n", ##args)
#define LOGD(format, args...)   printf("[Debug] " format "\n", ##args)
#define LOGW(format, args...)   printf("[Warn]  " format "\n", ##args)
#define LOGE(format, args...)   printf("[Error] " format "\n", ##args)
#define PRINTF(format, args...) printf(           format,      ##args)

#endif // MINIDL_SRC_LOG_H