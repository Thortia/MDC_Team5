// TrainAndTest.cpp
#include <stack>
#include <string>
#include <string.h>
#include <fstream>
#include <iostream>

#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/ml/ml.hpp>

#include<iostream>
#include<sstream>

// global variables ///////////////////////////////////////////////////////////////////////////////
const int MIN_CONTOUR_AREA = 100;

const int RESIZED_IMAGE_WIDTH = 20;
const int RESIZED_IMAGE_HEIGHT = 30;

///////////////////////////////////////////////////////////////////////////////////////////////////

enum OPERAND { PLUS = 1, MINUS = 1, PRODUCT = 2, DIVIDE = 2, LEFT_BRAKET = 0, RIGHT_BRAKET=3};

class ContourWithData {
public:
	// member variables ///////////////////////////////////////////////////////////////////////////
	std::vector<cv::Point> ptContour;           // contour
	cv::Rect boundingRect;                      // bounding rect for contour
	float fltArea;                              // area of contour

												///////////////////////////////////////////////////////////////////////////////////////////////
	bool checkIfContourIsValid() {                              // obviously in a production grade program
		if (fltArea < MIN_CONTOUR_AREA) return false;           // we would have a much more robust function for 
		return true;                                            // identifying if a contour is valid !!
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	static bool sortByBoundingRectXPosition(const ContourWithData& cwdLeft, const ContourWithData& cwdRight) {      // this function allows us to sort
		return(cwdLeft.boundingRect.x < cwdRight.boundingRect.x);                                                   // the contours from left to right
	}

};

std::string CharToString(char c)
{
	std::string str(1, c);
	return str;
}
/*
int GetOperand(char s)
{
	int op = s;
	switch (op)
	{
	case 40:
		op = LEFT_BRAKET;
		break;
	case 41:
		op = RIGHT_BRAKET;
		break;
	case 42:
		op = PRODUCT;
		break;
	case 43:
		op = PLUS;
		break;
	case 45:
		op = MINUS;
		break;
	case 47:
		op = DIVIDE;
	    break;
	}
	return op;
}
*/
int GetOperand(std :: string s, int idx) {
	int op = s[idx];
	switch (op)
	{
	case 40:
		op = LEFT_BRAKET;
		break;
	case 41:
		op = RIGHT_BRAKET;
		break;
	case 42:
		op = PRODUCT;
		break;
	case 43:
		op = PLUS;
		break;
	case 45:
		op = MINUS;
		break;
	case 47:
		op = DIVIDE;
		break;
	}
	return op;

}
int GetOperand(std::string s) {
	int op = s[0];
	switch (op)
	{
	case 40:
		op = LEFT_BRAKET;
		break;
	case 41:
		op = RIGHT_BRAKET;
		break;
	case 42:
		op = PRODUCT;
		break;
	case 43:
		op = PLUS;
		break;
	case 45:
		op = MINUS;
		break;
	case 47:
		op = DIVIDE;
		break;
	}
	return op;

}

int calculate(std :: string& outputStr, std::stack<int>& calcStack) {
	int tox = 0;
	int idx = 0;
	int cnt = 0;
	while (outputStr[idx] != '\0')
	{
		
		if (
			(outputStr[idx] == '+') ||
			(outputStr[idx] == '-') ||
			(outputStr[idx] == '/') ||
			(outputStr[idx] == '*')
			)
		{
			
			switch (outputStr[idx])
			{
			case 42:
				if (calcStack.size() >= 2)
				{
					int num2 = calcStack.top();
					calcStack.pop();
					int num1 = calcStack.top();
					calcStack.pop();

					calcStack.push(num1*num2);
				}
				break;
			case 43:
				if (calcStack.size() >= 2)
				{
					int num2 = calcStack.top();
					calcStack.pop();
					int num1 = calcStack.top();
					calcStack.pop();

					calcStack.push(num1+num2);
				}
				break;
			case 45:
				if (calcStack.size() >= 2)
				{
					int num2 = calcStack.top();
					calcStack.pop();
					int num1 = calcStack.top();
					calcStack.pop();

					calcStack.push(num1-num2);
				}
				break;
			case 47:
				if (calcStack.size() >= 2)
				{
					int num2 = calcStack.top();
					calcStack.pop();
					int num1 = calcStack.top();
					calcStack.pop();

					calcStack.push(num1 / num2);

				}
				break;

			}
			idx++;
			
		}
		
		else
		{
			cnt = idx;
		
			while (outputStr[cnt] != '.')
			{
				cnt++;
			}
			std::string tmp = outputStr.substr(idx, cnt - idx);
			calcStack.push(stoi(tmp));
			idx += cnt - idx + 1;
		}

	
}
	tox = calcStack.top();
	calcStack.pop();

	return tox;
}

void ChangePostFix(std::string& inputStr, std::string& outputStr, std::stack<std::string>& opStack)
{
	int idx = 0;
	while (inputStr[idx] != '\0') {
		int curOperand = 0;
		if ((inputStr[idx] == '+') ||
			(inputStr[idx] == '-') ||
			(inputStr[idx] == '*') ||
			(inputStr[idx] == '/') ||
			(inputStr[idx] == '(') ||
			(inputStr[idx] == ')'))
		{
			curOperand = GetOperand(inputStr,idx);
			
			if (curOperand == LEFT_BRAKET)
			{
				opStack.push(CharToString(inputStr[idx]));
			}
			else if (curOperand == RIGHT_BRAKET)
			{
				while ((opStack.size() > 0) && (opStack.top() != CharToString('(')))
				{

					outputStr += opStack.top();
					opStack.pop();
				}

				if (opStack.size() > 0)
					opStack.pop();
			}
			else if (opStack.empty())
			{
				opStack.push(CharToString(inputStr[idx]));
			}
			else if (curOperand > GetOperand(opStack.top()))
			{
				opStack.push(CharToString(inputStr[idx]));
			}
			else if (curOperand <= GetOperand(opStack.top()))
			{
				do
				{
					outputStr += opStack.top();
					opStack.pop();
					if (opStack.empty())
						break;
				} while ((curOperand <= GetOperand(opStack.top(),idx)) != true);
				
				opStack.push(CharToString(inputStr[idx]));

			}
			idx++;
		}
		else
		{
			int cnt = idx;
			while ((inputStr[cnt] != '+') &&
				(inputStr[cnt] != '-') &&
				(inputStr[cnt] != '*') &&
				(inputStr[cnt] != '/') &&
				(inputStr[cnt] != '(') &&
				(inputStr[cnt] != ')') &&
				(inputStr[cnt] != '\0')
				)
			{
				cnt++;
			}
			outputStr += inputStr.substr(idx, cnt - idx);
			outputStr += CharToString('.');
			idx += cnt - idx;

		}
	}
	
	while (opStack.size() > 0)
	{
		outputStr += opStack.top();
		opStack.pop();

	}
}

void NumFormulaChange(std::string& s)
{
	int idx = 0;
	while (s[idx] != '\0') {
		if (s[0] == '-')
		{
			s.insert(0, "(0");
			int j = 0 + 4;
			while ((s[j] != '\0') &&
				(s[j] != '-') &&
				(s[j] != '+') &&
				(s[j] != '/') &&
				(s[j] != '*') &&
				(s[j] != '(') &&
				(s[j] != ')'))
			{
				j++;
			}
			s.insert(j, ")");
		}
		else if ((s[idx] == '*') &&
			(s[idx] == '/') &&
			(s[idx] == '+') &&
			(s[idx] == '(')
			)
		{
			if (s[idx + 1] == '-')
			{
				s.insert(idx + 1, "(0");
				int j = idx + 4;
				while ((s[j] != '\0') &&
					(s[j] != '-') &&
					(s[j] != '+') &&
					(s[j] != '/') &&
					(s[j] != '*') &&
					(s[j] != '(') &&
					(s[j] != ')'))
				{
					j++;
				}
				s.insert(j, ")");
			}
		}
		idx++;
	}
}
int Calc(std::string& inputStr, std::string& outputStr, std::stack<std::string> opStack, std::stack<int> calcStack)
{
	NumFormulaChange(inputStr);
	ChangePostFix(inputStr, outputStr, opStack);

	return calculate(outputStr, calcStack);
}

int cal(std::string& s) {
	int answer = 0;
	std::string outputStr;
	std::stack<std::string> opStack;
	std::stack<int> calcStack;

	answer = Calc(s, outputStr, opStack, calcStack);

	std::cout << answer << '\n';

	return answer;

}


std::string OddEven(int number) {
	if (number % 2 == 1 || number  %2 ==-1)
	{
		return "Odd";
	}
	else {
		return "Even";
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
	std::vector<ContourWithData> allContoursWithData;           // declare empty vectors,
	std::vector<ContourWithData> validContoursWithData;         // we will fill these shortly

																// read in training classifications ///////////////////////////////////////////////////

	cv::Mat matClassificationInts;      // we will read the classification numbers into this variable as though it is a vector

	cv::FileStorage fsClassifications("classifications.xml", cv::FileStorage::READ);        // open the classifications file

	if (fsClassifications.isOpened() == false) {                                                    // if the file was not opened successfully
		std::cout << "error, unable to open training classifications file, exiting program\n\n";    // show error message
		return(0);                                                                                  // and exit program
	}

	fsClassifications["classifications"] >> matClassificationInts;      // read classifications section into Mat classifications variable
	fsClassifications.release();                                        // close the classifications file

																		// read in training images ////////////////////////////////////////////////////////////

	cv::Mat matTrainingImagesAsFlattenedFloats;         // we will read multiple images into this single image variable as though it is a vector

	cv::FileStorage fsTrainingImages("images.xml", cv::FileStorage::READ);          // open the training images file

	if (fsTrainingImages.isOpened() == false) {                                                 // if the file was not opened successfully
		std::cout << "error, unable to open training images file, exiting program\n\n";         // show error message
		return(0);                                                                              // and exit program
	}

	fsTrainingImages["images"] >> matTrainingImagesAsFlattenedFloats;           // read images section into Mat training images variable
	fsTrainingImages.release();                                                 // close the traning images file

																				// train //////////////////////////////////////////////////////////////////////////////

	cv::Ptr<cv::ml::KNearest>  kNearest(cv::ml::KNearest::create());            // instantiate the KNN object

																				// finally we get to the call to train, note that both parameters have to be of type Mat (a single Mat)
																				// even though in reality they are multiple images / numbers
	kNearest->train(matTrainingImagesAsFlattenedFloats, cv::ml::ROW_SAMPLE, matClassificationInts);

	// test ///////////////////////////////////////////////////////////////////////////////

	cv::Mat matTestingNumbers = cv::imread("test12.png");            // read in the test numbers image

	if (matTestingNumbers.empty()) {                                // if unable to open image
		std::cout << "error: image not read from file\n\n";         // show error message on command line
		return(0);                                                  // and exit program
	}

	cv::Mat matGrayscale;           //
	cv::Mat matBlurred;             // declare more image variables
	cv::Mat matThresh;              //
	cv::Mat matThreshCopy;          //

	cv::cvtColor(matTestingNumbers, matGrayscale, cv::COLOR_BGR2GRAY);         // convert to grayscale c

																		// blur
	cv::GaussianBlur(matGrayscale,              // input image
		matBlurred,                // output image
		cv::Size(5, 5),            // smoothing window width and height in pixels
		0);                        // sigma value, determines how much the image will be blurred, zero makes function choose the sigma value

								   // filter image from grayscale to black and white
	cv::adaptiveThreshold(matBlurred,                           // input image
		matThresh,                            // output image
		255,                                  // make pixels that pass the threshold full white
		cv::ADAPTIVE_THRESH_GAUSSIAN_C,       // use gaussian rather than mean, seems to give better results
		cv::THRESH_BINARY_INV,                // invert so foreground will be white, background will be black
		11,                                   // size of a pixel neighborhood used to calculate threshold value
		2);                                   // constant subtracted from the mean or weighted mean

	matThreshCopy = matThresh.clone();              // make a copy of the thresh image, this in necessary b/c findContours modifies the image

	std::vector<std::vector<cv::Point> > ptContours;        // declare a vector for the contours
	std::vector<cv::Vec4i> v4iHierarchy;                    // declare a vector for the hierarchy (we won't use this in this program but this may be helpful for reference)

	cv::findContours(matThreshCopy,             // input image, make sure to use a copy since the function will modify this image in the course of finding contours
		ptContours,                             // output contours
		v4iHierarchy,                           // output hierarchy
		cv::RETR_EXTERNAL,                      // retrieve the outermost contours only
		cv::CHAIN_APPROX_SIMPLE);               // compress horizontal, vertical, and diagonal segments and leave only their end points

	for (int i = 0; i < ptContours.size(); i++) {               // for each contour
		ContourWithData contourWithData;                                                    // instantiate a contour with data object
		contourWithData.ptContour = ptContours[i];                                          // assign contour to contour with data
		contourWithData.boundingRect = cv::boundingRect(contourWithData.ptContour);         // get the bounding rect
		contourWithData.fltArea = cv::contourArea(contourWithData.ptContour);               // calculate the contour area
		allContoursWithData.push_back(contourWithData);                                     // add contour with data object to list of all contours with data
	}

	for (int i = 0; i < allContoursWithData.size(); i++) {                      // for all contours
		if (allContoursWithData[i].checkIfContourIsValid()) {                   // check if valid
			validContoursWithData.push_back(allContoursWithData[i]);            // if so, append to valid contour list
		}
	}
	// sort contours from left to right
	std::sort(validContoursWithData.begin(), validContoursWithData.end(), ContourWithData::sortByBoundingRectXPosition);

	std::string strFinalString;         // declare final string, this will have the final number sequence by the end of the program

	for (int i = 0; i < validContoursWithData.size(); i++) {            // for each contour

																		// draw a green rect around the current char
		cv::rectangle(matTestingNumbers,                            // draw rectangle on original image
			validContoursWithData[i].boundingRect,        // rect to draw
			cv::Scalar(0, 255, 0),                        // green
			2);                                           // thickness

		cv::Mat matROI = matThresh(validContoursWithData[i].boundingRect);          // get ROI image of bounding rect

		cv::Mat matROIResized;
		cv::resize(matROI, matROIResized, cv::Size(RESIZED_IMAGE_WIDTH, RESIZED_IMAGE_HEIGHT));     // resize image, this will be more consistent for recognition and storage

		cv::Mat matROIFloat;
		matROIResized.convertTo(matROIFloat, CV_32FC1);             // convert Mat to float, necessary for call to find_nearest

		cv::Mat matROIFlattenedFloat = matROIFloat.reshape(1, 1);

		cv::Mat matCurrentChar(0, 0, CV_32F);

		kNearest->findNearest(matROIFlattenedFloat, 1, matCurrentChar);     // finally we can call find_nearest !!!

		float fltCurrentChar = (float)matCurrentChar.at<float>(0, 0);

		strFinalString = strFinalString + char(int(fltCurrentChar));        // append current char to full string
	}

	std::cout << "\n\n" << "numbers read = " << strFinalString << "\n\n";       // show the full string
	
	int answer = cal(strFinalString);

	std::ofstream out("output.txt");
	std::string s = OddEven(answer);

	if (out.is_open()) {
		out << "number is : "<< strFinalString << "\n";
		out << "answer is : " << answer << "\n";
		out << s;
	}

	else {

		std::cout << "파일을 찾을 수 없습니다." << std::endl;
	}

	cv::imshow("matTestingNumbers", matTestingNumbers);     // show input image with green boxes drawn around found digits
	
	return(0);
	//cv::waitKey(0);                                         // wait for user key press

	
}

