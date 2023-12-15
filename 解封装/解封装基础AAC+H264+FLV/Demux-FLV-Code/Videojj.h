#ifndef VIDEOJJ_H
#define VIDEOJJ_H

#include <vector>
#include <cstdint>
class CFlvParser;

typedef struct VjjSEI_s
{
	char *szUD;
	int nLen;
	int nTimeStamp; // ms
} VjjSEI;

class CVideojj
{
public:
	CVideojj();
	virtual ~CVideojj();

    int Process(uint8_t *pNalu, int nNaluLen, int nTimeStamp);

private:
	friend class CFlvParser;

	std::vector<VjjSEI> _vVjjSEI;
};

#endif // VIDEOJJ_H
