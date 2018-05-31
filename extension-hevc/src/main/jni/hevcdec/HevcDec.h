#include "openHevcWrapper.h"


typedef struct HEVCDecParam
{
    int nWidth;
    int nHeight;

    HEVCDecParam()
    {
        nWidth = 0;
        nHeight = 0;
    }
} HEVCDECPARAM;


typedef struct tagDecInfoParam{
    unsigned char *pYBuf;
    unsigned char *pUBuf;
    unsigned char *pVBuf;
    int nYStride;
    int nUStride;
    int nVStride;
    int nWidth;
    int nHeight;
    int nDataFormat;
    int nDataIsAligned;
}DecInfoParam;

enum emInputType
{
	def_InputType_I420    		= 0,
	def_InputType_NV21	        = 1,
	def_InputType_YV12	        = 2,
	def_InputType_NV12	        = 3,
	def_InputType_NoCamera      = 100,
};

class HevcDec
{
public:
    HevcDec();
    virtual ~HevcDec();

public:
    virtual int Init(HEVCDECPARAM stDecParam);
    virtual int UnInit();
    virtual int DoCodec(unsigned char *pcInBuf, int nInLen, unsigned char **ppcOutBuf, int *pnOutLen);
protected:
    int m_bIsInit;
    int m_nDecImgNum;
    int m_nActiveDecWidth;
    int m_nActiveDecHeight;

    HEVCDECPARAM m_stHevcDecParam;
    unsigned char *m_pcOut;
    int m_bDecShareBuf;
    OpenHevc_Frame openHevcFrame;
    OpenHevc_Frame_cpy openHevcFrameCpy;
    OpenHevc_Handle openHevcHandle;
};

#ifdef __cplusplus
extern "C" {
#endif
void* hevcDecCreate(int width, int height);
int hevcDecodeFrame(void* hHevcDec, unsigned char *pcInBuf, int nInLen, unsigned char **ppcOutBuf, int *pnOutLen);
void hevcDecDestory(void* hHevcDec);
#ifdef __cplusplus
}
#endif