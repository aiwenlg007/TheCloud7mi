
#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <conio.h> 
#include <string.h>
#include <fstream>

#include <vector>
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
void ReadRioDat()
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
		/*	ifile>>blkInfo.iBulketIndex>>szRn
				>>blkInfo.iBulkSize>>szRn
				>>blkInfo.szHost>>szRn
				>>blkInfo.szCtx>>szRn
				>>blkInfo.uCRC32>>szRn
				>>blkInfo.uCheckSum>>szRn;
*/
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
}
Qiniu_Error resumable_upload(Qiniu_Client* client, char* uptoken, const char* key, const char* localFile)
{
	Qiniu_Error err;
	Qiniu_Rio_PutExtra extra;
	Qiniu_Zero(extra);
	extra.bucket = bucket; 
	Qiniu_Rio_PutRet  ret;
	Qiniu_Zero(ret);
	extra.notify = Demo_Rio_FnNotify;
// 
// 	int iBlockCount = gBlkInfoVec[0].iBulkSize;
// 	int iBlockSize = sizeof(Qiniu_Rio_BlkputRet) * gBlkInfoVec;
// 	self->progresses = (Qiniu_Rio_BlkputRet*)malloc(cbprog);

	err = Qiniu_Rio_PutFile(client, &ret, uptoken, key, localFile, &extra);
	if (ret.key  && ret.hash)
	{
		printf("file Key= %s ,hash = %s \n",ret.key,ret.hash);
	}

	return err;
}

void main()
{

	ReadRioDat();


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
	err  = resumable_upload(&client, uptoken, /*NULL */key,  "E:\\compresesed.mp4");

	cout <<err.message;
	cout << "*****************" << endl;
} 