/*
 * ImageProcessing.cpp
 *
 *  Created on: 26 de mai de 2020
 *      Author: rfarah
 */

#include <fstream>
#include <iostream>
#include "ImageProcessing.h"

namespace rfh {

ImageProcessing::ImageProcessing() {

}

ImageProcessing::~ImageProcessing() {
}

void ImageProcessing::readImage(string name) {
	ifstream inFile;

	inFile.open(name, ios::in | ios::binary);
	if (!inFile) {
		cout << "Error opening file" << endl;
		return;
	}

//	inFile.read(header.data(), header.size());
	for (auto &i : header) {
		i = inFile.get();
	}

	width = *(reinterpret_cast<int*>(&header[OFFSET_WIDTH]));
	height = *(reinterpret_cast<int*>(&header[OFFSET_HEIGHT]));
	bitDepth = static_cast<int>(header.at(OFFSET_BIT_DEPTH));

	if (bitDepth <= 8) {
		inFile.read(reinterpret_cast<char*>(colorTable.data()),
				colorTable.size());
	}

	buffer.resize(width * height);
	inFile.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

	inFile.close();
	if (!inFile) {
		cout << "Error closing file" << endl;
		return;
	}
}

void ImageProcessing::writeImage(string name) {
	ofstream outFile;

	outFile.open(name, ios::out | ios::binary);
	if (!outFile) {
		cout << "Error opening file" << endl;
		return;
	}

	outFile.write(reinterpret_cast<char*>(header.data()), header.size());
	//	for(auto &i: header) {
	//		i = outFile.get();
	//	}

	if (bitDepth <= 8) {
		outFile.write(reinterpret_cast<char*>(colorTable.data()),
				colorTable.size());
	}

	outFile.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

	outFile.close();
	if (!outFile) {
		cout << "Error closing file" << endl;
		return;
	}
}

//void ImageProcessing::copyImageBuffer(const vector<unsigned char> &buff) {
//	buffer = buff;
//}

void ImageProcessing::binarizeImage(int threshold) {
	for (auto &i : buffer) {
		i = ((int) i > threshold) ? WHITE : BLACK;
	}
}

void ImageProcessing::changeBrightness(int brightness) {
	for (auto &i : buffer) {
		int temp = i + brightness;
		temp = (temp > MAX_COLOR) ? MAX_COLOR : temp;
		temp = (temp < MIN_COLOR) ? MIN_COLOR : temp;
		i = temp;
	}
}

void ImageProcessing::computeHistogram(string name) {
	ofstream outFile;

	outFile.open(name, ios::out);
	if (!outFile) {
		cout << "Error opening file" << endl;
		return;
	}

	array<double, TOTAL_COLORS> iHist { };

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int index = buffer.at(x + y * width);
			++(iHist[index]);
		}
	}

	for (int i = 0; i < TOTAL_COLORS; ++i) {
		histogram[i] = (iHist[i] / (width * height));
	}

	for (auto &i : histogram) {
		outFile << "\n" << i;
	}
	outFile.close();
	if (!outFile) {
		cout << "Error closing file" << endl;
		return;
	}
}

void ImageProcessing::equalizeHistogram(string name) {
	computeHistogram(name);
	array<int, TOTAL_COLORS> histogramEq { };

	for (int i = 0; i < TOTAL_COLORS; ++i) {
		float sum = 0.0;
		for (int j = 0; j < i; ++j) {
			sum += histogram[j];
		}
		histogramEq[i] = (int) ((TOTAL_COLORS - 1) * sum /*+ 0.5*/);
	}

	for (int i = 0; i < width * height; ++i) {
		buffer[i] = histogramEq[buffer[i]];
	}
	computeHistogram("eq_" + name);
}

void ImageProcessing::rotateRight() {
	vector<vector<unsigned char>> imgTemp(width,
			vector<unsigned char>(height, 0));

	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			imgTemp[j][height - 1 - i] = buffer[i * height + j];
		}
	}

	copyVector2DToBuffer(imgTemp);
}

void ImageProcessing::rotateLeft() {
	vector<vector<unsigned char>> imgTemp(width,
			vector<unsigned char>(height, 0));

	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			imgTemp[height - 1 - j][i] = buffer[i * height + j];
		}
	}

	copyVector2DToBuffer(imgTemp);
}

void ImageProcessing::rotateUpSideDown() {
	vector<vector<unsigned char>> imgTemp(width,
			vector<unsigned char>(height, 0));

	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			imgTemp[height - 1 - i][width - 1 - j] = buffer[i * height + j];
		}
	}

	copyVector2DToBuffer(imgTemp);
}

void ImageProcessing::copyVector2DToBuffer(
		vector<vector<unsigned char>> &imgTemp) {
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			buffer[i * height + j] = imgTemp[i][j];
		}
	}
}

void ImageProcessing::rotateImage(int rotation) {
	switch (rotation) {
	case 0:
		rotateRight();
		break;
	case 1:
		rotateLeft();
		break;
	case 2:
		rotateUpSideDown();
		break;
	}
}

void ImageProcessing::negativeImage() {
	for (auto &i : buffer) {
		i = 255 - i;
	}
}

void ImageProcessing::convolve2D(const Mask &mask) {
	/* 2D Convolution mask
	 * -1 -1 -1 -1 -1
	 * -1 -1 -1 -1 -1
	 * -1 -1 24 -1 -1
	 * -1 -1 -1 -1 -1
	 * -1 -1 -1 -1 -1
	 * */

	vector<unsigned char> bufferOut { };

	bufferOut.resize(width * height);

	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			int val = 0;
			for (int m = 0; m < mask.rows; ++m) {
				for (int n = 0; n < mask.cols; ++n) {
					int ms = mask.data[m * mask.rows + n];
					int idx = i - m;
					int jdx = j - n;
					int im = 0;
					if (idx > 0 && jdx > 0) {
						im = buffer[idx * height + jdx];
					}
					val += ms * im;
				}
			}
			if (val > 255) {
				val = 255;
			}
			if (val < 0) {
				val = 0;
			}
			bufferOut[i * height + j] = val;
		}
	}

	for (int i = 0; i < width * height; ++i) {
		buffer[i] = bufferOut[i];
	}
}

void ImageProcessing::lineDetector(const int mask[][3]) {
	vector<unsigned char> bufferOut { };

	bufferOut.resize(width * height);

	for (int y = 1; y < height; ++y) {
		for (int x = 1; x < width; ++x) {
			int sum = 0;
			for (int i = -1; i < 2; ++i) {
				for (int j = -1; j < 2; ++j) {
					sum += buffer[x + i + (y + j) * width]
							* mask[(i + 1)][j + 1];
				}
			}
			if (sum > 255) {
				sum = 255;
			}
			if (sum < 0) {
				sum = 0;
			}
			bufferOut[x + y * width] = sum;
		}
	}

	for (int i = 0; i < width * height; ++i) {
		buffer[i] = bufferOut[i];
	}
}

void ImageProcessing::print() {
	cout << "height: " << height << " width: " << width << " bitDepth: "
			<< bitDepth << endl;
}

} /* namespace rfh */
