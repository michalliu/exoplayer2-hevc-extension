#include "HevcDec.h"
#include <memory.h>
#include <stdlib.h>

#define MAX_IMG_SIZE (2560 * 1600 * 3 / 2)
#define DECPADDING 64

int find_start_code (unsigned char *Buf, int zeros_in_startcode)
{
    int i;
    for (i = 0; i < zeros_in_startcode; i++)
        if(Buf[i] != 0)
            return 0;
    return Buf[i];
}

HevcDec::HevcDec()
{
    m_bIsInit = false;
    m_nDecImgNum = 0;
    m_nActiveDecWidth = 0;
    m_nActiveDecHeight = 0;
    m_bDecShareBuf = 0;
}

HevcDec::~HevcDec()
{
    if (m_bIsInit)
    {
        UnInit();
    }
}

int HevcDec::Init(HEVCDECPARAM stDecParam)
{
    int ret = true;
    if (!m_bIsInit) UnInit();

    m_stHevcDecParam.nWidth  = stDecParam.nWidth;
    m_stHevcDecParam.nHeight = stDecParam.nHeight;

    int thread_type       = 1;
    int nb_pthreads       = 1;
    openHevcHandle = libOpenHevcInit(nb_pthreads, thread_type);
    if (NULL == openHevcHandle)
    {
        ret = false;
        return ret;
    }

    m_pcOut = new unsigned char[MAX_IMG_SIZE+DECPADDING];
    m_bIsInit = true;

    return ret;
}

int HevcDec::UnInit()
{
    if ( !m_bIsInit )
    {
        return false;
    }
    if(m_pcOut)
	{
        delete []m_pcOut;
        m_pcOut = NULL;
	}

	libOpenHevcClose(openHevcHandle);

    m_nDecImgNum = 0;
    m_bIsInit = false;
    return true;
}


int HevcDec::DoCodec(unsigned char *pcInBuf, int nInLen, unsigned char **ppcOutBuf, int *pnOutLen)
{
    if (!m_bIsInit)
    {
        return false;
    }

    if (pcInBuf == NULL || nInLen < 4)
        return false;

    unsigned int currentIndex,packetEndIndex;
    int tmpflag0,tmpflag1;
    int ret;

    currentIndex      = 0;
    packetEndIndex = 4;
    while (packetEndIndex<nInLen)
    {
        int flag = 0;
        while(packetEndIndex<nInLen-4)
        {
            tmpflag0 = (pcInBuf[packetEndIndex] == 0) &&(pcInBuf[packetEndIndex+1]==0) && (pcInBuf[packetEndIndex+2]==0)&& (pcInBuf[packetEndIndex+3]==1);
            tmpflag1 = (pcInBuf[packetEndIndex] == 0) &&(pcInBuf[packetEndIndex+1]==0) && (pcInBuf[packetEndIndex+2]==1);
            if(tmpflag0||tmpflag1)
            {
                flag =1;
                break;
            }
            packetEndIndex++;
        }

        if(1!=flag)
        {
            packetEndIndex = nInLen;
        }
        tmpflag1 = 0;
        tmpflag0 = find_start_code(pcInBuf+currentIndex,3);
        if(1!=tmpflag0)
        {
            tmpflag1 = find_start_code(pcInBuf+currentIndex,2);
        }
        if(1== tmpflag0)
        {
            currentIndex +=4;
        }
        if(1== tmpflag1)
        {
            currentIndex +=3;
        }

        ret = libOpenHevcDecode(openHevcHandle, pcInBuf+currentIndex, packetEndIndex-currentIndex, 0);

	    if (ret>0)
		{
            int nbData;
            libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrameCpy.frameInfo);
            nbData = openHevcFrameCpy.frameInfo.nWidth * openHevcFrameCpy.frameInfo.nHeight;
            int nDataOffset = 0;
            if(m_bDecShareBuf)  nDataOffset = DECPADDING;
            openHevcFrameCpy.pvY = m_pcOut + nDataOffset;
            openHevcFrameCpy.pvU = m_pcOut + nDataOffset + nbData;
            openHevcFrameCpy.pvV = m_pcOut + nDataOffset + nbData + nbData/4;

            libOpenHevcGetOutputCpy(openHevcHandle, 1, &openHevcFrameCpy);
            ++m_nDecImgNum;
            m_nActiveDecWidth = openHevcFrameCpy.frameInfo.nWidth;
            m_nActiveDecHeight = openHevcFrameCpy.frameInfo.nHeight;

            if(m_bDecShareBuf)
            {
                DecInfoParam * pDecInfo = (DecInfoParam *)m_pcOut;
                pDecInfo->pYBuf = (unsigned char *)openHevcFrameCpy.pvY;
                pDecInfo->pUBuf = (unsigned char *)openHevcFrameCpy.pvU;
                pDecInfo->pVBuf = (unsigned char *)openHevcFrameCpy.pvV;
                pDecInfo->nYStride = m_nActiveDecWidth;
                pDecInfo->nUStride = m_nActiveDecWidth/2;
                pDecInfo->nVStride = m_nActiveDecWidth/2;
                pDecInfo->nWidth = m_nActiveDecWidth;
                pDecInfo->nHeight = m_nActiveDecHeight;
                pDecInfo->nDataFormat = def_InputType_I420;
                pDecInfo->nDataIsAligned = 1;
            }
            *ppcOutBuf = m_pcOut;
            *pnOutLen = m_nActiveDecWidth * m_nActiveDecHeight *3/2;
		}
		if(ret<0)
		{
			return false;
		}
        currentIndex = packetEndIndex;
        packetEndIndex +=3;

        if(0==flag)
        {
            break;
        }
    }

    return true;

}
#ifdef __cplusplus
extern "C" {
#endif
void* hevcDecCreate(int width, int height)
{
    HevcDec* pHevcDec = new HevcDec();
    if (pHevcDec == NULL) {
        return NULL;
    }

    HEVCDECPARAM stDecParam;
    stDecParam.nWidth = width;
    stDecParam.nHeight = height;
    pHevcDec->Init(stDecParam);

    return pHevcDec;
}

int hevcDecodeFrame(void* hHevcDec, unsigned char *pcInBuf, int nInLen, unsigned char **ppcOutBuf, int *pnOutLen)
{
    if (hHevcDec == NULL || nInLen < 0) {
        return -1;
    }

    HevcDec *hevcDec = (HevcDec *)hHevcDec;
    return hevcDec->DoCodec(pcInBuf, nInLen, ppcOutBuf, pnOutLen);
}

void hevcDecDestory(void* hHevcDec)
{
    HevcDec* pHevcDec = (HevcDec *)hHevcDec;
    if (pHevcDec != NULL)
    {
        pHevcDec->UnInit();
        delete pHevcDec;
        pHevcDec = NULL;
    }
}

#ifdef __cplusplus
}
#endif

