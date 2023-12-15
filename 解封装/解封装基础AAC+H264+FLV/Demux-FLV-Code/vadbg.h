#ifndef VADBG_H
#define VADBG_H

#include <iostream>

namespace vadbg
{
	void DumpString(std::string path, std::string str);
    void DumpBuffer(std::string path, uint8_t *pBuffer, int nBufSize);
}


#endif // VADBG_H
