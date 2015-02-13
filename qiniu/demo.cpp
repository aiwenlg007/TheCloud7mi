
#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <conio.h> 
#include <string.h>
#include <fstream>

#include <vector>
#include <map>

#include <atlenc.h> // for base64
#include "UrlCoder.h"


using  namespace std;
#ifdef __cplusplus
extern "C" {
#endif


#include ".\..\b64\b64.h" 
#include "conf.h"
#include "base.h"
#include "http.h"
#include "io.h"
#include "rs.h"
#include "resumable_io.h"

#ifdef __cplusplus  

}
#endif  

using namespace B64_NAMESPACE;
/*

static const char bucket[] = "testpublic";
static const char key[] = "helloworld00121221";
static const char domain[] = "testpublic.qiniudn.com";


static const  char   QINIU_ACCESS_KEY1[] = "7sEgop7h-Njukh-TvvbveCQ4wrLEYXSW7LAIAS9x";
static const  char   QINIU_SECRET_KEY1[] = "Nuw3OjxghrJzRo_9A4b05ynxHRYq15mg9fxRvGtB";
*/
static const char bucket[] = "thelumi";
static const char key[] = "Carpenters_Love_mp3_clip";
static const char domain[] = "7u2m5v.com1.z0.glb.clouddn.com";


static const  char   QINIU_ACCESS_KEY1[] = "V5Toae_jwiU_8v8hHGRk_wtP8JTBeEgY3rBP3wAx";
static const  char   QINIU_SECRET_KEY1[] = "nIC2_0zxKlE3Tf__s3DCH5kjc6puG7AVehyZsfXp";


typedef struct BulketInfo
{
	int iBulketIndex;
	int iBulkSize;
	unsigned int uCRC32;
	unsigned int uOffset;
	char szHost[512];
	char szCtx[512];
	char szCheckSum[128];
	BulketInfo()
	{
		iBulketIndex = 0;
		iBulkSize = 0;
		uCRC32 = 0;
		uOffset = 0;
		memset(szHost,0,512);
		memset(szCtx,0,512);
		memset(szCheckSum,0,128);
	} 
}BulketInfo,*PBulketInfo;

typedef vector<Qiniu_Rio_BlkputRet> BlkPutRetVec;
typedef vector<BulketInfo> BulketInfoVec;

BlkPutRetVec gBlkPutRetVec;
BulketInfoVec gBlkInfoVec;


Qiniu_Rio_PutExtra gExtra;
std::map<int,int> gRioBlkMap;
bool bNeedExtra = false;

// 获取文件大小
Qiniu_Int64 Qiniu_Get_PutFile_Size( const char* localFile)
{ 
	Qiniu_Int64 fsize;
	Qiniu_FileInfo fi;
	Qiniu_File* f;
	Qiniu_Error err = Qiniu_File_Open(&f, localFile);
	if (err.code != 200) {
		return 0;
	}
	err = Qiniu_File_Stat(f, &fi);
	if (err.code == 200) {
		fsize = Qiniu_FileInfo_Fsize(fi); 
	}
	Qiniu_File_Close(f);
	return fsize;
}
// 初始化文件Block 的Progress
static int InitExtra(
	Qiniu_Rio_PutExtra* self, Qiniu_Int64 fsize)
{
	size_t cbprog;
	int  blockCnt = Qiniu_Rio_BlockCount(fsize);

	memset(self, 0, sizeof(Qiniu_Rio_PutExtra)); 

	cbprog = sizeof(Qiniu_Rio_BlkputRet) * blockCnt;
	self->progresses = (Qiniu_Rio_BlkputRet*)malloc(cbprog);
	self->blockCnt = blockCnt;

	memset(self->progresses, 0, cbprog); 
	for (int i = 0; i< blockCnt; ++i)
	{
		self->progresses[i].offset = 0;
		self->progresses[i].ctx = (char*)malloc(512);
		memset((void *)self->progresses[i].ctx, 0, 512); 
		self->progresses[i].host = (char*)malloc(512);
		memset((void *)self->progresses[i].host, 0, 512); 
		self->progresses[i].checksum = (char*)malloc(128);
		memset((void *)self->progresses[i].checksum, 0, 128); 
	} 
	return 0;
}
// 每个Chunk 上传成功后的回调函数
static void notifyResult(void* self, int blkIdx, int blkSize, Qiniu_Rio_BlkputRet* ret) 
{
	//Qiniu_Rio_BlkputRet_Assign(&gExtra.progresses[blkSize],ret);
	gExtra.progresses[blkIdx].offset = ret->offset;
	if(NULL == gExtra.progresses[blkIdx].ctx ) 
	{
		gExtra.progresses[blkIdx].ctx = (char*)malloc(512);
		memset((void*)gExtra.progresses[blkIdx].ctx,0,512);
	}
	Qiniu_Rio_BlkputRet_Assign(&gExtra.progresses[blkIdx],ret);
	ofstream ofile;
	ofile.open("Rio.dat");
	char szWriteLine[2048] = {0};
	
	for (int i = 0; i < gExtra.blockCnt; ++i)
	{ 
		char szCtx[512] = "null";
		char szCrCheckSum[128] = "null";
		char szHost[512] = "null";
		if (NULL != gExtra.progresses[i].ctx && strlen(gExtra.progresses[i].ctx) > 0)
		{
			memcpy(szCtx, gExtra.progresses[i].ctx,512);
		}
		if (strlen(gExtra.progresses[i].host) > 0)
		{ 
			memcpy(szHost, gExtra.progresses[i].host,512);
		}
		if (strlen(gExtra.progresses[i].checksum) > 0)
		{
			memcpy(szCrCheckSum, gExtra.progresses[i].checksum,128); 
		}

		ofile<<i<<"\t"
			<<gExtra.progresses[i].offset<<"\t"
			<<szHost<<"\t"
			<<szCtx<<"\t"
			<<szCrCheckSum<<"\t" 
			<<gExtra.progresses[i].crc32<<"\t" 
			<<endl;
	
	}
	ofile.close();
}
bool Read_Rio_Dat()
{ 
	ifstream ifile;
	ifile.open("Rio.dat",ios_base::_Nocreate);
	ifile.seekg(0,ios_base::end);
	Qiniu_Uint32 fsize=ifile.tellg();
	if(!fsize || fsize <= 0)
	{
		cout<<"Empty file."<<endl;
	}
	else 
	{
		ifile.seekg(0);
		while(!ifile.eof())
		{    
			BulketInfo blkInfo;  
			ifile>>blkInfo.iBulketIndex  
				>>blkInfo.uOffset
				>>blkInfo.szHost
				>>blkInfo.szCtx 
				>>blkInfo.szCheckSum
				>>blkInfo.uCRC32;
			if (gExtra.blockCnt > 0  && blkInfo.uOffset >= 0  && blkInfo.iBulketIndex < gExtra.blockCnt && strlen(blkInfo.szHost) > 0) 
			{
				gExtra.progresses[blkInfo.iBulketIndex].offset = blkInfo.uOffset;
				memcpy((void*)gExtra.progresses[blkInfo.iBulketIndex].host,blkInfo.szHost,strlen(blkInfo.szHost));
				if(0 != strcmp(blkInfo.szCtx,"null"))
				{
					memcpy((void*)gExtra.progresses[blkInfo.iBulketIndex].ctx,blkInfo.szCtx,strlen(blkInfo.szCtx));
				}
				memcpy((void*)gExtra.progresses[blkInfo.iBulketIndex].checksum,blkInfo.szCheckSum,strlen(blkInfo.szCheckSum)); 
				gExtra.progresses[blkInfo.iBulketIndex].crc32 = blkInfo.uCRC32;
			}
		} 
	}  
	ifile.close();

	return (fsize > 0);
}
void Demo_Rio_FnNotify(void* recvr, int blkIdx, int blkSize, Qiniu_Rio_BlkputRet* ret)
{
	ofstream ofile;
	ofile.open("Rio.dat"/*,std::ios::app*/);
	ofile<<blkIdx<<"\t"
		<<blkSize<<"\t"
		<<ret->host<<"\t"
		<<ret->ctx<<"\t"
		<<ret->crc32<<"\t"
		<<ret->checksum<<"\t"
		<<ret->offset<<endl; 
	ofile.close();

	gBlkPutRetVec.push_back(*ret);
	printf("ret[%d]: %s,%s,%d,%d \n",blkIdx,ret->host,ret->ctx,ret->crc32,ret->checksum);
}
bool Read_Rio_Dat_old()
{ 
	ifstream ifile;
	ifile.open("Rio.dat",ios_base::_Nocreate);
	ifile.seekg(0,ios_base::end);
	int fsize=ifile.tellg();
	if(!fsize || fsize <= 0)
	{
		cout<<"Empty file."<<endl;
	}
	else 
	{
		ifile.seekg(0);
		while(!ifile.eof())
		{ 
			BulketInfo blkInfo;
			char szRn; 
			ifile>>blkInfo.iBulketIndex
				>>blkInfo.iBulkSize
				>>blkInfo.szHost
				>>blkInfo.szCtx
				>>blkInfo.uCRC32
				>>blkInfo.szCheckSum
				>>blkInfo.uOffset;
				
			gBlkInfoVec.push_back(blkInfo);
		} 
	}  
	ifile.close();
	 
	return (fsize > 0);
}
Qiniu_Error resumable_upload(Qiniu_Client* client, char* uptoken, const char* key, const char* localFile)
{
	Qiniu_Error err;
	Qiniu_Rio_PutExtra extra;
	Qiniu_Zero(extra);
	extra.bucket = bucket; 
	Qiniu_Rio_PutRet  ret;
	Qiniu_Zero(ret);
	extra.notify = notifyResult;
// 
// 	int iBlockCount = gBlkInfoVec[0].iBulkSize;
// 	int iBlockSize = sizeof(Qiniu_Rio_BlkputRet) * gBlkInfoVec;
// 	self->progresses = (Qiniu_Rio_BlkputRet*)malloc(cbprog);
	gExtra.bucket = bucket;
	gExtra.notify = notifyResult;
	err = Qiniu_Rio_PutFile(client, &ret, uptoken, key, localFile, &gExtra);
	if (ret.key  && ret.hash)
	{
		printf("file Key= %s ,hash = %s \n",ret.key,ret.hash);
	}

	return err;
}

void main()
{
	
	InitExtra(&gExtra,Qiniu_Get_PutFile_Size("E:\\Lumi\\ShouldItMatter.mp3"));
	// if the file size is zero ,we need to initial the basic block progress
	if(Read_Rio_Dat())
	{
	    bNeedExtra = true;
	}
	 
	for (int i = 0 ; i < gExtra.blockCnt; ++i)
	{ 
		if ( strlen(gExtra.progresses[i].ctx) <= 0 || 0 ==  strcmp (gExtra.progresses[i].ctx,"null"))
		{
			free((void*)gExtra.progresses[i].ctx);
			gExtra.progresses[i].ctx = NULL;
		}
	}
	 

	

	Qiniu_Mac  qiniu_mac;
	qiniu_mac.accessKey = QINIU_ACCESS_KEY1;
	qiniu_mac.secretKey = QINIU_SECRET_KEY1;

	Qiniu_RS_PutPolicy  putPolicy;
	Qiniu_Zero(putPolicy);
	putPolicy.scope = "thelumi";
	char  *uptoken  = Qiniu_RS_PutPolicy_Token(&putPolicy, &qiniu_mac);

	Qiniu_Error err;
	Qiniu_Client client;
	Qiniu_Io_PutRet putRet;

	Qiniu_Zero(putRet);
	Qiniu_Client_InitNoAuth(&client, 1024);

	// err = Qiniu_Io_PutFile(&client, &putRet, uptoken, key,  "E:\\compresesed", NULL);

	/*
	resumable_upload的第三个参数是Key,但是提示要key进行utf8编码才行
	*/  
	err  = resumable_upload(&client, uptoken, /*NULL */key,  "E:\\Lumi\\ShouldItMatter.mp3");//E:\\compresesed.mp4

	cout <<err.message;
	cout << "*****************" << endl;
} 