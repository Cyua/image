#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <string>
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


BITMAPFILEHEADER strHead;
BITMAPINFOHEADER strInfo;
WORD bfType;
int h,w,size;
vector<vector<BYTE> > binarization;

void otsu(vector<vector<BYTE> >& src);
void printImage(vector<vector<BYTE> > src, string filename);
vector<vector<BYTE> > eroision(vector<vector<BYTE> > src);
vector<vector<BYTE> > delation(vector<vector<BYTE> > src);

int main(){
	FILE *fpi;
	fpi=fopen("input.bmp","rb");
	if(fpi != NULL){
		//先读取文件类型
		fread(&bfType,1,sizeof(WORD),fpi);
		if(0x4d42!=bfType) {
			cout<<"Error: The file is not a bmp image!"<<endl;
			return 0;
		}
		//读取bmp文件的文件头和信息头
		fread(&strHead,1,sizeof(tagBITMAPFILEHEADER),fpi);
		fread(&strInfo,1,sizeof(tagBITMAPINFOHEADER),fpi);
       	h=strInfo.biHeight;
       	w=strInfo.biWidth;
       	if(h%4!=0)
       		h=(h/4+1)*4;
       	if(h*w*3!=strInfo.biSizeImage){
       		cout<<"Error: please choose a image whose horizontal pixels can be divided by 4！"<<endl;
       		return 0;
       	}
       	size=strInfo.biSizeImage/3;
       	binarization.clear();
       	binarization.resize(h);
       	for(int i = 0; i < h; i++)
       		binarization[i].resize(w);
       	//read the data area
        DATA *imgdata=new DATA[size];
		fread(imgdata,1,sizeof(DATA)*size,fpi);//读取bmp数据信息
		fclose(fpi);

		// cout<<"horizantal: "<< strInfo.biWidth<<endl;
		// cout<<"vertical: "<< strInfo.biHeight<<endl;
		// cout<<"multiple: "<< strInfo.biWidth * strInfo.biHeight * 3<<endl;
		// cout<<"size: "<< strInfo.biSizeImage <<endl;

		for(int i=0; i<size; i++){
			BYTE gray = (imgdata[i].blue*11 + imgdata[i].red*30 + imgdata[i].green*59)/100;
			binarization[i/w][i%w]=gray;
		}

		otsu(binarization);
		printImage(binarization,"binarization.bmp");

		vector<vector<BYTE> > erosionData = eroision(binarization);
		printImage(erosionData,"erosion.bmp");

		vector<vector<BYTE> > delationData = delation(binarization);
		printImage(delationData,"delation.bmp");

		vector<vector<BYTE> > openingData = delation(erosionData);
		printImage(openingData,"opening.bmp");

		vector<vector<BYTE> > closingData = eroision(delationData);
		printImage(closingData,"closing.bmp");


	}
	else
	{
		cout<<"Can't open the file!"<<endl;
		return 0;
	}
	return 0;
}

void printImage(vector<vector<BYTE> > src, string filename){
	DATA* output = new DATA[size];
	for(int i=0; i<size; i++){
		BYTE tempByte = src[i/w][i%w];
		output[i].blue = tempByte;
		output[i].green = tempByte;
		output[i].red = tempByte;
	}
	FILE *fpo1;
	fpo1=fopen(filename.c_str(),"wb");
	if(fpo1==NULL)
		cout<<"wrong"<<endl;
	fwrite(&bfType,1,sizeof(WORD),fpo1);
	fwrite(&strHead,1,sizeof(tagBITMAPFILEHEADER),fpo1);
	fwrite(&strInfo,1,sizeof(tagBITMAPINFOHEADER),fpo1);
	fwrite(output,1,sizeof(DATA)*(size),fpo1);
	fclose(fpo1);
	delete[] output;
}

void otsu(vector<vector<BYTE> >& srcData){
	int slideWindow = 4;	//let the sildewindow be 1/16 image size
	int winH=h/slideWindow, winW=w/slideWindow;
	int modified=70;
	float histogram[256];
	float avgValue=0;
	int threshold;  
	for(int n = 0; n < slideWindow; n++){
		for(int m = 0; m < slideWindow; m++){
			
			memset(histogram,0,sizeof(histogram)); 
			for(int i = n*winH; i<std::min(n*winH+winH,h); i++){    
 				for(int j = m*winW; j<std::min(m*winW+winW,w); j++){
 					histogram[srcData[i][j]]++;
 				}
   			}    
   			avgValue=0;  
   			for(int i = 0; i < 256; i++){  
        		histogram[i] = histogram[i] / size;   
        		avgValue += i * histogram[i];  
    		}
			threshold=0;     
    		float maxVariance=0;    
    		float ww = 0, u = 0;    
    		for(int i = 0; i < 256; i++)   
    		{    
        		ww += histogram[i]; 
        		u += i * histogram[i];   
  
		        float t = avgValue * ww - u;    
        		float variance = t * t / (ww * (1 - ww) );    
        		if(variance > maxVariance)   
        		{    
            		maxVariance = variance;    
            		threshold = i;    
        		}    
    		}    
    		threshold-=modified;
    		for(int i = n*winH; i<std::min((int)(n*winH+winH),(int)h); i++){    
 				for(int j = m*winW; j<std::min((int)(m*winW+winW),(int)w); j++){
 					// cout<<(int)srcData[i][j]<<" ";
 					if(srcData[i][j]>=threshold){
 						// cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
 						srcData[i][j]=255;
 					}
 					else
 						srcData[i][j]=0;
 				}
   			}
		}
	}    
}

vector<vector<BYTE> > eroision(vector<vector<BYTE> > src){
	vector<vector<BYTE> > modified(h+2);
	for(int i = 0; i < modified.size(); i++)
		modified[i].resize(w+2);
	for(int i = 0; i < modified.size();i++){
		if(i==0||i==modified.size()-1){
			for(int j = 0; j < modified[i].size(); j++)
				modified[i][j]=255;
		}
		else{
			for(int j = 0; j < modified[i].size();j++){
				if(j==0||j==modified[i].size()-1)
					modified[i][j]=255;
				else
					modified[i][j]=src[i-1][j-1];
			}
		}
	}

	vector<vector<BYTE> > result(h);
	for(int i = 0; i < result.size(); i++)
		result[i].resize(w);
	for(int i = 0; i < h; i++){
		for(int j = 0; j < w; j++){
			int sum = modified[i][j]+modified[i][j+1]+modified[i][j+2]
			+modified[i+1][j]+modified[i+1][j+1]+modified[i+1][j+2]
			+modified[i+2][j]+modified[i+2][j+1]+modified[i+2][j+2];
			if(sum>0)
				result[i][j]=255;
			else
				result[i][j]=0;
		}
	}
	return result;
}

vector<vector<BYTE> > delation(vector<vector<BYTE> > src){
	vector<vector<BYTE> > modified(h+2);
	for(int i = 0; i < modified.size(); i++)
		modified[i].resize(w+2);
	for(int i = 0; i < modified.size();i++){
		if(i==0||i==modified.size()-1){
			for(int j = 0; j < modified[i].size(); j++)
				modified[i][j]=255;
		}
		else{
			for(int j = 0; j < modified[i].size();j++){
				if(j==0||j==modified[i].size()-1)
					modified[i][j]=255;
				else
					modified[i][j]=src[i-1][j-1];
			}
		}
	}

	vector<vector<BYTE> > result(h);
	for(int i = 0; i < result.size(); i++)
		result[i].resize(w);
	for(int i = 0; i < h; i++){
		for(int j = 0; j < w; j++){
			int sum = modified[i][j]+modified[i][j+1]+modified[i][j+2]
			+modified[i+1][j]+modified[i+1][j+1]+modified[i+1][j+2]
			+modified[i+2][j]+modified[i+2][j+1]+modified[i+2][j+2];
			if(sum/9==255)
				result[i][j]=255;
			else
				result[i][j]=0;
		}
	}
	return result;
}