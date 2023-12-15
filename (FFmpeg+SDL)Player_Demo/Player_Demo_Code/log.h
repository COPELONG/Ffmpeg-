#ifndef LOG_H
#define LOG_H

/***
 *
 * 主要用来打印显示数据
 *
 */

#include <string>



void LogInit();
void Serialize(const char* fmt, ...);


#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define makePrefix(fmt) std::string(__FUNCTION__).append("() - ").append(fmt).c_str()

#define LogDebug(fmt, ...)	Serialize(makePrefix(fmt), ##__VA_ARGS__)
#define LogInfo(fmt, ...)	Serialize(makePrefix(fmt), ##__VA_ARGS__)
#define LogNotice(fmt, ...)	Serialize(makePrefix(fmt), ##__VA_ARGS__)
#define LogError(fmt, ...)	Serialize(makePrefix(fmt), ##__VA_ARGS__)


#define FunEntry(...) LogDebug(" Entry... " ##__VA_ARGS__)
#define FunExit(...) LogDebug(" Exit... " ##__VA_ARGS__)


#endif // LOG_H
