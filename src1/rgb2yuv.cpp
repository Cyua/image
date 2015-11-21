#include <cstdio>
#include <iostream>
using namespace std;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

//位图文件头定义;
typedef struct  tagBITMAPFILEHEADER{
//	WORD bfType;//单独读取，结构体中就不定义了
	DWORD bfSize;//文件大小
	WORD bfReserved1;//保留字
	WORD bfReserved2;//保留字
	DWORD bfOffBits;//从文件头到实际位图数据的偏移字节数
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	DWORD biSize;//信息头大小
	DWORD biWidth;//图像宽度
	DWORD biHeight;//图像高度
	WORD biPlanes;//位平面数，必须为1
	WORD biBitCount;//每像素位数
	DWORD  biCompression; //压缩类型
	DWORD  biSizeImage; //压缩图像大小字节数
	DWORD  biXPelsPerMeter; //水平分辨率
	DWORD  biYPelsPerMeter; //垂直分辨率
	DWORD  biClrUsed; //位图实际用到的色彩数
	DWORD  biClrImportant; //本位图中重要的色彩数
}BITMAPINFOHEADER; //位图信息头定义

//像素信息
typedef struct tagIMAGEDATA
{
	BYTE blue;
	BYTE green;
	BYTE red;
}DATA;

typedef struct tagYUV{
	BYTE Y;
	BYTE U;
	BYTE V;
}YUV;

BITMAPFILEHEADER strHead;
BITMAPINFOHEADER strInfo;

BYTE clip(int n)
{
	if(n>255)
		n=255;
	if(n<0)
		n=0;
	return n;
}
int main(){
	FILE *fpi;
	fpi=fopen("input.bmp","rb");
	if(fpi != NULL){
		//先读取文件类型
		WORD bfType;
		fread(&bfType,1,sizeof(WORD),fpi);
		if(0x4d42!=bfType) {
			cout<<"It's not a bmp!"<<endl;
			return 0;
		}
		//读取bmp文件的文件头和信息头
		fread(&strHead,1,sizeof(tagBITMAPFILEHEADER),fpi);
		fread(&strInfo,1,sizeof(tagBITMAPINFOHEADER),fpi);
        int h=strInfo.biHeight,w=strInfo.biWidth,size=strInfo.biSizeImage/3;
        DATA *imgdata=new DATA[size];
		fread(imgdata,1,sizeof(DATA)*size,fpi);//读取bmp数据信息
		YUV *trans = new YUV[size];
		fclose(fpi);

		//把rgb转变为yuv
		for(int i=0;i<size;i++)
		{
			trans[i].Y = ( (  66 * imgdata[i].red + 129 * imgdata[i].green +  25 * imgdata[i].blue + 128) >> 8) +  16;
			trans[i].U = ( ( -38 * imgdata[i].red -  74 * imgdata[i].green + 112 * imgdata[i].blue + 128) >> 8) + 128;
			trans[i].V = ( ( 112 * imgdata[i].red -  94 * imgdata[i].green -  18 * imgdata[i].blue + 128) >> 8) + 128;
		}

		//把彩色转为灰色
		DATA *gray = new DATA[size];
		for(int i=0;i<size;i++){
			gray[i].blue=trans[i].Y;
			gray[i].green=trans[i].Y;
			gray[i].red=trans[i].Y;
		}
		FILE *fpo1;
		fpo1=fopen("gray.bmp","wb");
		if(fpo1==NULL)
			cout<<"wrong"<<endl;
		fwrite(&bfType,1,sizeof(WORD),fpo1);
		fwrite(&strHead,1,sizeof(tagBITMAPFILEHEADER),fpo1);
		fwrite(&strInfo,1,sizeof(tagBITMAPINFOHEADER),fpo1); 
		fwrite(gray,1,sizeof(DATA)*size,fpo1);
		fclose(fpo1);
		delete[] gray;
		//灰度图写入完成

		//改变yuv的y值
		for(int i=0;i<size;i++){
			trans[i].Y+=10;
			if(trans[i].Y>255)
				trans[i].Y=255;
		}

		//将yuv转为rgb并导出文件“yuv2rgb.bmp”
		FILE *fpo2;
		fpo2=fopen("yuv2rgb.bmp","wb");
		DATA *newrgb = new DATA[size];
		if(fpo2==NULL)
			cout<<"wrong"<<endl;
		fwrite(&bfType,1,sizeof(WORD),fpo2);
		fwrite(&strHead,1,sizeof(tagBITMAPFILEHEADER),fpo2);
		fwrite(&strInfo,1,sizeof(tagBITMAPINFOHEADER),fpo2); 
		for(int i=0;i<size;i++){
			int C = trans[i].Y - 16;
			int D = trans[i].U - 128;
			int E = trans[i].V - 128;
			newrgb[i].red = clip(( 298 * C + 409 * E + 128) >> 8);
			newrgb[i].green = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8);
			newrgb[i].blue = clip(( 298 * C + 516 * D + 128) >> 8);

		}
		fwrite(newrgb,1,sizeof(DATA)*size,fpo2);
		fclose(fpo2);
		delete[] imgdata;
		delete[] trans;
		delete[] newrgb;
	}
	else
	{
		cout<<"Can't open the file!"<<endl;
		return 0;
	}
	return 0;
}