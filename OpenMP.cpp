#define _USE_MATH_DEFINES
//
#include <iostream>
#include <omp.h>
#include <math.h>
#include <ctime>
#include "C:\Users\napa$$$\Desktop\OpenMP\OpenMP\FreeImage.h"
#pragma comment(lib, "C:\\Users\\napa$$$\\Desktop\\OpenMP\\OpenMP\\FreeImage.lib")
//
using namespace std;
//
struct pixel
{
	int r = 0;
	int g = 0;
	int b = 0;
};
//
FIBITMAP* LoadImg(const char* lpszPathName, int flag)
{
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(lpszPathName, 0);

	if (fif == FIF_UNKNOWN) fif = FreeImage_GetFIFFromFilename(lpszPathName);

	if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif))
	{
		FIBITMAP* dib = FreeImage_Load(fif, lpszPathName, flag);
		return dib;
	}

	return NULL;
}
//
pixel** ConvertionToPixels(FIBITMAP* dib, int& height, int& weight)
{
	int count = 0;
	pixel** pixels;
	BYTE* data = FreeImage_GetBits(dib);
	height = FreeImage_GetHeight(dib); weight = FreeImage_GetWidth(dib);
	pixels = new pixel * [height];
	for (int i = 0; i < height; i++)
	{
		pixels[i] = new pixel[weight];
		for (int j = 0; j < weight; j++)
		{
			pixels[i][j].b = data[count++];
			pixels[i][j].g = data[count++];
			pixels[i][j].r = data[count++];
		}
	}
	return pixels;
}
//
double Gausse(int x, int y, double sigma)
{
	return exp(-(pow(x, 2) + pow(y, 2)) / (pow(sigma, 2) * 2)) / ((3.14) * 2 * pow(sigma, 2));
}
//
double** CoreForConvolution(double sigma)
{
	int lenght = ceil(6 * sigma);
	if (lenght % 2 == 0) lenght++;

	double** pixels = new double* [lenght];
	double sum = 0;
	for (int i = 0; i < lenght; i++)
		pixels[i] = new double[lenght];

	#pragma parallel for reduction(+:sum)
	for (int i = 0; i < lenght; i++)
	{
		for (int j = 0; j < lenght; j++)
		{
			pixels[i][j] = Gausse((-lenght / 2 + j), (lenght / 2 - i), sigma);
			sum += pixels[i][j];
		}
	}
	for (int i = 0; i < lenght; i++)
	{
		for (int j = 0; j < lenght; j++)
		{
			pixels[i][j] /= sum;
		}
	}
	return pixels;
}
//
pixel** Convolution(pixel** pixels, double** Core, int height, int weight, int sigma)
{
	int lenght = ceil(6 * sigma);
	double r_result = 0, g_result = 0, b_result = 0;
	if (lenght % 2 == 0) lenght++;

	pixel** result_pixels; pixel temp;
	result_pixels = new pixel * [height];
	for (int row = 0; row < height; row++)
	{
		result_pixels[row] = new pixel[weight];
	}

	#pragma omp parallel for
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < weight; col++)
		{
			int t_row = row - lenght / 2, t_col = col - lenght / 2;
			double r_result = 0, g_result = 0, b_result = 0;
			for (int i = 0; i < lenght; i++)
			{
				t_row++;
				for (int j = 0; j < lenght; j++)
				{
					if (t_col >= 0 && t_row >= 0 && t_col < weight && t_row < height)
					{
						r_result += pixels[t_row][t_col].r * Core[i][j];
						g_result += pixels[t_row][t_col].g * Core[i][j];
						b_result += pixels[t_row][t_col].b * Core[i][j];
					}
					t_col++;
				}
			}
			result_pixels[row][col].r = round(r_result);
			result_pixels[row][col].g = round(g_result);
			result_pixels[row][col].b = round(b_result);
		}
	}

	return result_pixels;
}
//
BYTE* GetByte(pixel** result_pixels, int height, int weight)
{
	BYTE* newdate = new BYTE[(height * weight * 3)];
	int  index = 0;
	for (int i = height - 1; i >= 0; i--)
	{
		for (int j = 0; j < weight; j++)
		{
			newdate[index] = result_pixels[i][j].b;
			newdate[index + 1] = result_pixels[i][j].g;
			newdate[index + 2] = result_pixels[i][j].r;
			index += 3;
		}
	}
	return newdate;
}
//
void SaveImg(FIBITMAP* dib, BYTE* newdate, int height, int weight, const char* output_filename)
{
	FREE_IMAGE_FORMAT out_fif = FreeImage_GetFIFFromFilename(output_filename);

	if (out_fif != FIF_UNKNOWN)
	{
		int z = FreeImage_GetBPP(dib);
		FIBITMAP* newone = FreeImage_ConvertFromRawBits(newdate, weight, height, weight * 3, z, 0, 0, 0);

		FreeImage_Save(out_fif, newone, output_filename, 0);
	}
	FreeImage_Unload(dib);
}
//
int main() 
{
	const char* input_filename = "photo.jpg";
	const char* output_filename = "result_of_blure.jpg";
	int height, weight;
	//
	FIBITMAP* dib = LoadImg(input_filename, 0);
	pixel** pixels = ConvertionToPixels(dib, height, weight);
	//
	double sigma;
	cout << "enter sigma: ";
	cin >> sigma;
	cout << endl << "loading..." << endl;
	//
	double **Core = CoreForConvolution(sigma);
	unsigned int time = clock();
	pixel** result_pixels = Convolution(pixels, Core, height, weight, sigma);
	//
	cout << endl << "ready!" << endl << endl << "time: " << (clock() - time) / 1000.0 << " sec" << endl;
	//
	for (int i = 0; i < sizeof(pixels); i++) delete pixels[i];
	delete pixels;
	for (int i = 0; i < sizeof(Core); i++) delete Core[i];
	delete Core;
	//
	BYTE* newdate = GetByte(result_pixels, height, weight);
	//
	for (int i = 0; i < sizeof(result_pixels); i++) delete result_pixels[i];
	delete result_pixels;
	//
	SaveImg(dib, newdate, height, weight, output_filename);
	delete newdate;
	//
	return 0;
}