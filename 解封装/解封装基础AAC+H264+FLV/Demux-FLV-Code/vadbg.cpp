#include <fstream>

#include "vadbg.h"

using namespace std;

namespace vadbg
{
	void DumpString(std::string path, std::string str)
	{
		ofstream fin;
		fin.open(path.c_str(), ios_base::out);
		fin << str.c_str();
		fin.close();
	}

    void DumpBuffer(std::string path, uint8_t *pBuffer, int nBufSize)
	{
		ofstream fin;
		fin.open(path.c_str(), ios_base::out | ios_base::binary);
		fin.write((char *)pBuffer, nBufSize);
		fin.close();
	}
}
