/*
===========================================================================

Created by Stefan Friese on 26.04.2018

===========================================================================
*/
// 
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "nlms_types.h" // added types

#define RGB_COLOR 255
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

double *xSamples;		// Input color values from PPM
mldata_t *mlData = NULL;	// Machine learning realted data
point_t *points = NULL;		// Graphing			

							/* *Graph building* */
static imagePixel_t * rdPPM(char *fileName); 			// Read PPM file format
void mkPpmFile(char *fileName, imagePixel_t *image);		// Writes PPM file
int ppmColorChannel(FILE* fp, imagePixel_t *image, 		// Writes colorChannel from PPM file to log file
	char *colorChannel, mldata_t *mlData);
void colorSamples(FILE* fp, mldata_t *mlData); 			// Stores color channel values in xSamples

														/* *File handling* */
char * mkFileName(char* buffer, 				// Date+suffix as filename					
	size_t max_len, int suffixId);
char *fileSuffix(int id);					// Filename ending of logs
char *fileHeader(int id);					// Header inside the logfiles
void bufferLogger(char *buffer, point_t points[]);		// Writes points to graph template
void mkSvgGraph(point_t points[], char *templatePath);				// Parses graph template and calls bufferLogger()
void weightsLogger(double *weights, int suffix);		// Writes updated weights to a file

														/* *rand seed* */
double r2(void);						// Random val between 0 and 1
double rndm(void);

/* *args parser* */
void usage(char **argv); 						// Help text called by args parser

												/* *math* */
mldata_t * init_mldata_t(unsigned windowSize, unsigned samplesCount, double learnrate);
double sum_array(double x[], int length);

void localMean(mldata_t *mlData, point_t points[]);			// First,
void directPredecessor(mldata_t *mlData, point_t points[]);		// Second,
void differentialPredecessor(mldata_t *mlData, point_t points[]);	// Third filter implementation
void standardNLMS(mldata_t *mlData, point_t points[]);

double windowXMean(int _arraylength, int xCount);		// Returns mean value of given window


int main(int argc, char **argv) {
	char *colorChannel = (char *)malloc(sizeof(char) * 32);
	char *inputfile = (char *)malloc(sizeof(char) * 32);
	unsigned *seed = NULL;
	unsigned k, include = 0;
	unsigned windowSize = 5;
	unsigned samplesCount = 512;
	char *stdcolor = "green", xBuffer[512];
	colorChannel = stdcolor;
	unsigned int uint_buffer[1], windowBuffer[1];
	double learnrate = 0.4;
	char *istrue = "true";
	char *templatePath = NULL;

	while ((argc > 1) && (argv[1][0] == '-')) {	// Parses parameters from stdin
		switch (argv[1][1]) {
		case 'i':
			inputfile = &argv[1][3];
			++argv;
			--argc;
			break;
		case 'w':
			sscanf(&argv[1][3], "%u", windowBuffer);
			windowSize = windowBuffer[0];
			++argv;
			--argc;
			break;
		case 'c':
			colorChannel = &argv[1][3];
			++argv;
			--argc;
			break;
		case 's':
			sscanf(&argv[1][3], "%u", uint_buffer);
			seed = &uint_buffer[0];
			++argv;
			--argc;
			break;
		case 'n':
			sscanf(&argv[1][3], "%u", &samplesCount);
			++argv;
			--argc;
			break;
		case'h':
			printf("Program name: %s\n", argv[0]);
			usage(argv);
			break;
		case 'l':
			sscanf(&argv[1][3], "%lf", &learnrate);
			++argv;
			--argc;
			break;
		case 'g':
			sscanf(&argv[1][3], "%s", xBuffer);
			if (strstr(xBuffer, istrue)) {
				include = 1;
			}
			else if (xBuffer && !strstr(xBuffer, istrue)) {
				templatePath = xBuffer;
				include = 1;
			}
			else {
				printf("Wrong Argruments: %s\n", argv[1]);
				usage(argv);
			}
			++argv;
			--argc;
			break;
		default:
			printf("Wrong Arguments: %s\n", argv[1]);
			usage(argv);
		}

		++argv;
		--argc;
	}

	init_mldata_t(windowSize, samplesCount, learnrate);
	xSamples = (double *)malloc(sizeof(double) * mlData->samplesCount); 	// Resize input values
	points = (point_t *)malloc(sizeof(point_t) * mlData->samplesCount);		// Resize points
	imagePixel_t *image;
	image = rdPPM(inputfile);							// Set Pointer on input values	

	printf("window Size: %d\n", mlData->windowSize);
	char fileName[50];								// Logfiles and their names 
	mkFileName(fileName, sizeof(fileName), TEST_VALUES);
	FILE* fp5 = fopen(fileName, "w");
	ppmColorChannel(fp5, image, colorChannel, mlData);
	FILE* fp6 = fopen(fileName, "r");
	colorSamples(fp6, mlData);

	if ((seed != NULL)) {
		srand(*seed);								// Seed for random number generating
		printf("srand is reproducable\n");
	}
	else {
		srand((unsigned int)time(NULL));
		printf("srand depends on time\n");					// Default seed is time(NULL)
	}
	printf("generated weights:\n");

	for (k = 0; k < mlData->windowSize; k++) {
		mlData->weights[k] = rndm(); 						// Init random weights
		printf("[%d] %lf\n", k, mlData->weights[k]);
	}

	mkFileName(fileName, sizeof(fileName), PURE_WEIGHTS);				// Logfile weights
	FILE *fp0 = fopen(fileName, "w");
	for (k = 0; k < mlData->windowSize; k++) {
		fprintf(fp0, "[%d]%lf\n", k, mlData->weights[k]);
	}
	fclose(fp0);

	
	localMean(mlData, points);							// math magic functions
	directPredecessor(mlData, points);
	differentialPredecessor(mlData, points);
	standardNLMS(mlData, points);

	if (include == 1) {
		mkSvgGraph(points, templatePath);					// Graph building

	}

	free(image);
	free(xSamples);
	free(points);
	free(mlData);
	printf("\nDONE!\n");
}


/*
======================================================================================================

standardNLMS

basic nullified least mean square implementation

======================================================================================================
*/
void standardNLMS(mldata_t *mlData, point_t points[]) {
	double *localWeights = (double *)malloc(sizeof(double) * mlData->windowSize + 1);
	memcpy(localWeights, mlData->weights, sizeof(double) * mlData->windowSize + 1);

	char fileName[512];
	const unsigned xErrorLength = mlData->samplesCount;

	double *xError = (double *)malloc(sizeof(double) * mlData->samplesCount + 1);					// Includes e(n)		
	for (int i = 0; i < mlData->samplesCount + 1; i++) {
		xError[i] = 0.0;
	}

	unsigned i, xCount = 0;

	mkFileName(fileName, sizeof(fileName), STANDARD_NLMS);
	FILE* fp01 = fopen(fileName, "w");
	fprintf(fp01, fileHeader(STANDARD_NLMS_HEADER));

	mkFileName(fileName, sizeof(fileName), USED_WEIGHTS_STANDARD_NLMS);
	FILE* fp9 = fopen(fileName, "w");

	double xMean = xSamples[0];
	double xSquared = 0.0;
	double xPredicted = 0.0;
	double xActual = 0.0;

	for (xCount = 1; xCount < mlData->samplesCount - 1; xCount++) {
		unsigned _arrayLength = (xCount > mlData->windowSize) ? mlData->windowSize + 1 : xCount;
		xMean = (xCount > 0) ? windowXMean(_arrayLength, xCount) : 0;
		xPredicted = 0.0;
		xActual = xSamples[xCount];

		for (i = 1; i < _arrayLength; i++) {
			xPredicted += localWeights[i - 1] * xSamples[xCount - i];
		}
		xError[xCount] = xActual - xPredicted;

		xSquared = 0.0;
		for (i = 1; i < _arrayLength; i++) {
			xSquared += xSamples[xCount - i] * xSamples[xCount - i];
		}
		if (xSquared == 0) {
			xSquared = 1.0;
		}

		for (i = 1; i < _arrayLength; i++) {
			localWeights[i - 1] = localWeights[i - 1] + mlData->learnrate * xError[xCount]
				* (xSamples[xCount - i] / xSquared);
			fprintf(fp9, "%lf;", localWeights[i - 1]);
		}
		fprintf(fp9, "\n");
		fprintf(fp01, "%d\t%f\t%f\t%f\n", xCount, xPredicted, xActual, xError[xCount]);

		points[xCount].xVal[7] = xCount;
		points[xCount].yVal[7] = xPredicted;
		points[xCount].xVal[8] = xCount;
		points[xCount].yVal[8] = xError[xCount];
	}
	fclose(fp9);

	double mean = sum_array(xError, xErrorLength) / xErrorLength;
	double deviation = 0.0;

	for (i = 1; i < xErrorLength; i++) {
		deviation += (xError[i] - mean) * (xError[i] - mean);
	}
	deviation /= xErrorLength;
	printf("mean square err: %lf, variance: %lf\t\tNLMS\n", mean, deviation);
	fprintf(fp01, "\nQuadratische Varianz(x_error): %f\nMittelwert:(x_error): %f\n\n", deviation, mean);

	free(localWeights);
	free(xError);

}

/*
======================================================================================================

localMean

Variant (1/3), substract local mean.

======================================================================================================
*/
void localMean(mldata_t *mlData, point_t points[]) {
	double *localWeights = (double *)malloc(sizeof(double) * mlData->windowSize + 1);
	memcpy(localWeights, mlData->weights, sizeof(double) * mlData->windowSize + 1);

	char fileName[512];
	const unsigned xErrorLength = mlData->samplesCount;
	double *xError = (double *)malloc(sizeof(double) * mlData->samplesCount + 1);					// Includes e(n)		
	for (int i = 0; i < mlData->samplesCount + 1; i++) {
		xError[i] = 0.0;
	}
	unsigned i, xCount = 0; 			            						// Runtime vars

	mkFileName(fileName, sizeof(fileName), LOCAL_MEAN);						// Create Logfile and its filename
	FILE* fp4 = fopen(fileName, "w");
	fprintf(fp4, fileHeader(LOCAL_MEAN_HEADER));

	mkFileName(fileName, sizeof(fileName), USED_WEIGHTS_LOCAL_MEAN);
	FILE *fp9 = fopen(fileName, "w");


	double xMean = xSamples[0];
	double xSquared = 0.0;
	double xPredicted = 0.0;
	double xActual = 0.0;

	for (xCount = 1; xCount < mlData->samplesCount - 1; xCount++) { 					// First value will not get predicted
		unsigned _arrayLength = (xCount > mlData->windowSize) ? mlData->windowSize + 1 : xCount;	// Ensures corect length at start
		xMean = (xCount > 0) ? windowXMean(_arrayLength, xCount) : 0;
		xPredicted = 0.0;
		xActual = xSamples[xCount];

		for (i = 1; i < _arrayLength; i++) { 								// Get predicted value
			xPredicted += (localWeights[i - 1] * (xSamples[xCount - i] - xMean));
		}
		xPredicted += xMean;
		xError[xCount] = xActual - xPredicted;								// Get error value
		xSquared = 0.0;
		for (i = 1; i < _arrayLength; i++) { 								// Get xSquared
			xSquared += (xSamples[xCount - i] - xMean) * (xSamples[xCount - i] - xMean);
		}
		if (xSquared == 0.0) { 									// Otherwise returns Pred: -1.#IND00 in some occassions
			xSquared = 1.0;
		}
		for (i = 1; i < _arrayLength; i++) { 								// Update weights
			localWeights[i - 1] = localWeights[i - 1] + mlData->learnrate * xError[xCount]  		// Substract localMean
				* ((xSamples[xCount - i] - xMean) / xSquared);
			fprintf(fp9, "%lf;", localWeights[i - 1]);
		}
		fprintf(fp9, "\n");
		fprintf(fp4, "%d\t%f\t%f\t%f\n", xCount, xPredicted, xActual, xError[xCount]);			// Write to logfile

		points[xCount].xVal[1] = xCount;								// Save points so graph can be build later on
		points[xCount].yVal[1] = xPredicted;
		points[xCount].xVal[4] = xCount;
		points[xCount].yVal[4] = xError[xCount];


	}
	fclose(fp9);

	double mean = sum_array(xError, xErrorLength) / xErrorLength;					// Mean 
	double deviation = 0.0;

	for (i = 1; i < xErrorLength; i++) {									// Mean square
		deviation += (xError[i] - mean) * (xError[i] - mean);
	}
	deviation /= xErrorLength;										// Deviation
	printf("mean square err: %lf, variance: %lf\t\tlocal Mean\n", mean, deviation);
	fprintf(fp4, "\nQuadratische Varianz(x_error): %f\nMittelwert:(x_error): %f\n\n", deviation, mean);	// Write to logfile
	fclose(fp4);

	free(localWeights);
	free(xError);
}

/*
======================================================================================================

directPredecessor

Variant (2/3),
substract direct predecessor

======================================================================================================
*/
void directPredecessor(mldata_t *mlData, point_t points[]) {
	double *localWeights = (double *)malloc(sizeof(double) * mlData->windowSize + 1);
	memcpy(localWeights, mlData->weights, sizeof(double) * mlData->windowSize + 1);

	char fileName[512];
	const unsigned xErrorLength = mlData->samplesCount;
	double *xError = (double *)malloc(sizeof(double) * mlData->samplesCount + 1);					// Includes e(n)		
	for (int i = 0; i < mlData->samplesCount + 1; i++) {
		xError[i] = 0.0;
	}
	unsigned xCount = 0, i;
	double xActual = 0.0;
	double xPredicted = 0.0;

	mkFileName(fileName, sizeof(fileName), DIRECT_PREDECESSOR);								// Logfile and name handling
	FILE *fp3 = fopen(fileName, "w");
	fprintf(fp3, fileHeader(DIRECT_PREDECESSOR_HEADER));

	mkFileName(fileName, sizeof(fileName), USED_WEIGHTS_DIR_PRED);
	FILE *fp9 = fopen(fileName, "w");

	for (xCount = 1; xCount < mlData->samplesCount - 1; xCount++) { 								// first value will not get predicted
		unsigned _arrayLength = (xCount > mlData->windowSize) ? mlData->windowSize + 1 : xCount;
		xPredicted = 0.0;
		xActual = xSamples[xCount];

		for (i = 1; i < _arrayLength; i++) {
			xPredicted += (localWeights[i - 1] * (xSamples[xCount - 1] - xSamples[xCount - i - 1]));
		}

		xPredicted += xSamples[xCount - 1];
		xError[xCount] = xActual - xPredicted;

		double xSquared = 0.0;
		for (i = 1; i < _arrayLength; i++) {
			xSquared += (xSamples[xCount - 1] - xSamples[xCount - i - 1])
				* (xSamples[xCount - 1] - xSamples[xCount - i - 1]); 					// substract direct predecessor
		}
		if (xSquared == 0.0) { 											// Otherwise returns Pred: -1.#IND00 in some occassions
			xSquared = 1.0;
		}
		for (i = 1; i < _arrayLength; i++) {										// Update weights
			localWeights[i - 1] = localWeights[i - 1] + mlData->learnrate * xError[xCount]
				* ((xSamples[xCount - 1] - xSamples[xCount - i - 1]) / xSquared);
			fprintf(fp9, "%lf;", localWeights[i - 1]);
		}
		fprintf(fp9, "\n");

		fprintf(fp3, "%d\t%f\t%f\t%f\n", xCount, xPredicted, xActual, xError[xCount]);					// Write to logfile
		points[xCount].xVal[2] = xCount; 										// Fill point_t array for graph building
		points[xCount].yVal[2] = xPredicted;
		points[xCount].xVal[5] = xCount;
		points[xCount].yVal[5] = xError[xCount];
	}
	fclose(fp9);

	double mean = sum_array(xError, xErrorLength) / xErrorLength;							// Mean
	double deviation = 0.0;


	for (i = 1; i < xErrorLength; i++) {
		deviation += (xError[i] - mean) * (xError[i] - mean);										// Mean square
	}
	deviation /= xErrorLength;												// Deviation
	printf("mean square err: %lf, variance: %lf\t\t\tdirect Predecessor\n", mean, deviation);
	fprintf(fp3, "\nQuadratische Varianz(x_error): %f\nMittelwert:(x_error): %f\n\n", deviation, mean);
	fclose(fp3);

	free(localWeights);
	free(xError);
}

/*
======================================================================================================

differentialPredecessor

variant (3/3),
differential predecessor.

======================================================================================================
*/
void differentialPredecessor(mldata_t *mlData, point_t points[]) {
	double *localWeights = (double *)malloc(sizeof(double) * mlData->windowSize + 1);

	memcpy(localWeights, mlData->weights, sizeof(double) * mlData->windowSize + 1);
	const unsigned xErrorLength = mlData->samplesCount;
	char fileName[512];

	double *xError = (double *)malloc(sizeof(double) * mlData->samplesCount + 1);					// Includes e(n)		
	
	for (int i = 0; i < mlData->samplesCount + 1; i++) {
		xError[i] = 0.0;
	}

	unsigned xCount = 0,i = 0;
	double xPredicted = 0.0;
	double xActual = 0.0;

	mkFileName(fileName, sizeof(fileName), DIFFERENTIAL_PREDECESSOR);							// File handling
	FILE *fp6 = fopen(fileName, "w");
	fprintf(fp6, fileHeader(DIFFERENTIAL_PREDECESSOR_HEADER));

	mkFileName(fileName, sizeof(fileName), USED_WEIGHTS_DIFF_PRED);
	FILE *fp9 = fopen(fileName, "w");

	for (xCount = 1; xCount < mlData->samplesCount - 1; xCount++) { 							// First value will not get predicted

		unsigned _arrayLength = (xCount > mlData->windowSize) ? mlData->windowSize + 1 : xCount;
		xPredicted = 0.0;
		xActual = xSamples[xCount];

		for (i = 1; i < _arrayLength; i++) {
			xPredicted += (localWeights[i - 1] * (xSamples[xCount - i] - xSamples[xCount - i - 1]));
		}
		xPredicted += xSamples[xCount - 1];
		xError[xCount] = xActual - xPredicted;

		double xSquared = 0.0;
		for (i = 1; i < _arrayLength; i++) {
			xSquared += (xSamples[xCount - i] - xSamples[xCount - i - 1])
				* (xSamples[xCount - i] - xSamples[xCount - i - 1]); 					// Substract direct predecessor
		}
		if (xSquared == 0.0) { 											// Otherwise returns Pred: -1.#IND00 in some occassions
			xSquared = 1.0;
		}

		for (i = 1; i < _arrayLength; i++) {
			localWeights[i - 1] = localWeights[i - 1] + mlData->learnrate * xError[xCount]
				* ((xSamples[xCount - i] - xSamples[xCount - i - 1]) / xSquared);
			fprintf(fp9, "%lf;", localWeights[i - 1]);

		}
		fprintf(fp9, "\n");
		fprintf(fp6, "%d\t%f\t%f\t%f\n", xCount, xPredicted, xActual, xError[xCount]); 					// Write to logfile

		points[xCount].xVal[3] = xCount;
		points[xCount].yVal[3] = xPredicted;
		points[xCount].xVal[6] = xCount;
		points[xCount].yVal[6] = xError[xCount];

	}
	fclose(fp9);

	double mean = sum_array(xError, xErrorLength) / xErrorLength;
	double deviation = 0.0;


	for (i = 1; i < xErrorLength; i++) {											// Mean square
		deviation += (xError[i] - mean) * (xError[i] - mean);
	}
	deviation /= xErrorLength;
	printf("mean square err: %lf, variance: %lf\t\t\tdifferential Predecessor\n", mean, deviation);
	fprintf(fp6, "\nQuadratische Varianz(x_error): %f\nMittelwert:(x_error): %f\n\n", deviation, mean);
	fclose(fp6);

	free(localWeights);
	free(xError);
}

/*
======================================================================================================

mkFileName

Writes the current date plus suffix with index suffixId
into the given buffer. If the total length is longer than max_len,
only max_len characters will be written.

======================================================================================================
*/
char *mkFileName(char* buffer, size_t max_len, int suffixId) {
	const char * format_str = "%Y-%m-%d_%H_%M_%S";				// Date formatting
	size_t date_len;
	const char * suffix = fileSuffix(suffixId);
	time_t now = time(NULL);

	strftime(buffer, max_len, format_str, localtime(&now));			// Get Date
	date_len = strlen(buffer);
	strncat(buffer, suffix, max_len - date_len);				// Concat filename
	return buffer;
}

/*
======================================================================================================

fileSuffix

Contains and returns every suffix for all existing filenames

======================================================================================================
*/
char * fileSuffix(int id) {
	char * suffix[] = { "_weights_pure.txt",
		"_weights_used_dir_pred_.txt",
		"_direct_predecessor.txt",
		"_ergebnisse.txt",
		"_localMean.txt",
		"_testvalues.txt",
		"_differential_predecessor.txt",
		"_weights_used_local_mean.txt",
		"_weights_used_diff_pred.txt",
		"_standard_least_mean_square.txt",
		"_weights_used_std_nlms.txt"
	};
	return suffix[id];
}

/*
======================================================================================================

fileHeader

Contains and returns header from logfiles

======================================================================================================
*/
char * fileHeader(int id) {
	char * header[] = { "\n=========================== Local Mean ===========================\nNo.\txPredicted\txAcutal\t\txError\n",
		"\n=========================== Direct Predecessor ===========================\nNo.\txPredicted\txAcutal\t\txError\n",
		"\n=========================== Differential Predecessor ===========================\nNo.\txPredicted\txAcutal\t\txError\n",
		"\n========================= Nullified Least Mean Square =========================\nNo.\txPredicted\txAcutal\t\txError\n"
	};
	return header[id];
}

/*
======================================================================================================

weightsLogger

Logs used weights to logfile - not used right now

======================================================================================================
*/
void weightsLogger(double *weights, int val) {
	char fileName[512];
	unsigned i;
	mkFileName(fileName, sizeof(fileName), val);
	FILE* fp = fopen(fileName, "wa");
	for (i = 0; i < mlData->windowSize - 1; i++) {
		fprintf(fp, "[%d]%lf\n", i, weights[i]);
	}
	fclose(fp);
}

/*
======================================================================================================

bufferLogger

formats output of mkSvgGraph -- Please open graphResults.html to see the output--
[0] = xActual,
[1] = xPredicted from localMean,
[2] = xPredicted from directPredecessor,
[3] = xPredicted from differentialpredecessor,
[4] = xError from localMean,
[5] = xError from directPredecessor,
[6] = xError from differentialPredecessor
[7] = xPredicted from NLMS,
[8] = xError from NLMS

======================================================================================================
*/
void bufferLogger(char *buffer, point_t points[]) {
	unsigned i;
	char _buffer[512] = "";
	for (i = 1; i < mlData->samplesCount - 1; i++) { 									// xActual
		sprintf(_buffer, "L %f %f\n", points[i].xVal[0], points[i].yVal[0]);
		strcat(buffer, _buffer);
	}
	strcat(buffer, "\" fill=\"none\" id=\"svg_1\" stroke=\"black\" stroke-width=\"0.4px\"/>\n<path d=\"M0 0\n");
	for (i = 1; i < mlData->samplesCount - 1; i++) {									// xPredicted from localMean
		sprintf(_buffer, "L %f %f\n", points[i].xVal[1], points[i].yVal[1]);
		strcat(buffer, _buffer);
	}
	strcat(buffer, "\" fill=\"none\" id=\"svg_2\" stroke=\"green\" stroke-width=\"0.4px\"/>\n<path d=\"M0 0\n");
	for (i = 1; i <= mlData->samplesCount - 2; i++) { 									//xPredicted from directPredecessor
		sprintf(_buffer, "L %f %f\n", points[i].xVal[2], points[i].yVal[2]);
		strcat(buffer, _buffer);
	}
	strcat(buffer, "\" fill=\"none\" id=\"svg_3\" stroke=\"blue\" stroke-width=\"0.4px\"/>\n<path d=\"M0 0\n");
	for (i = 1; i < mlData->samplesCount - 1; i++) { 									//xPredicted from diff Pred
		sprintf(_buffer, "L %f %f\n", points[i].xVal[3], points[i].yVal[3]);
		strcat(buffer, _buffer);
	}
	strcat(buffer, "\" fill=\"none\" id=\"svg_4\" stroke=\"red\" stroke-width=\"0.4px\"/>\n<path d=\"M0 0\n");
	for (i = 1; i < mlData->samplesCount - 1; i++) { 									//xPredicted from diff Pred
		sprintf(_buffer, "L %f %f\n", points[i].xVal[7], points[i].yVal[7]);
		strcat(buffer, _buffer);
	}
	strcat(buffer, "\" fill=\"none\" id=\"svg_5\" stroke=\"gray\" stroke-width=\"0.4px\"/>\n");


}

/*
======================================================================================================

sum_array

Sum of all elements in x within a defined length

======================================================================================================
*/
double sum_array(double x[], int xlength) {
	int i = 0;
	double sum = 0.0;

	if (xlength != 0) {
		for (i = 0; i < xlength; i++) {
			sum += x[i];
		}
	}
	return sum;
}

/*
======================================================================================================

r2

returns a random double value between 0 and 1

======================================================================================================
*/
double r2(void) {
	return((rand() % 10000) / 10000.0);
}

/*
======================================================================================================

rndm

fills a double variable with random value and returns it

======================================================================================================
*/
double rndm(void) {
	double rndmval = r2();
	return rndmval;
}

/*
======================================================================================================

mkSvgGraph

parses template.svg and writes results in said template

======================================================================================================
*/
void mkSvgGraph(point_t points[], char *templatePath) {
	FILE* input = NULL;
	FILE *target = fopen("graphResults.html", "w");
	if (templatePath) {
		printf("\ngraph template src at: %s\n", templatePath);
		input = fopen(templatePath, "r");
	}
	else {
		input = fopen("graphResults_template.html", "r");
	}

	char line[512];
	char firstGraph[15] = { "<path d=\"M0 0" };			// Position where points will be written after

	if (input == NULL) {
		printf("\nNo inputfile at mkSvgGraph()\n");
		exit(EXIT_FAILURE);
	}

	char buffer[131072] = "";					// Really really dirty

	memset(buffer, '\0', sizeof(buffer));
	while (!feof(input)) {						// parses file until "firstGraph" has been found 	
		fgets(line, 512, input);
		strncat(buffer, line, strlen(line));
		if (strstr(line, firstGraph) != NULL) {			// Compares line <-> "firstGraph"
			bufferLogger(buffer, points);			// write points
		}

	}
	fprintf(target, buffer);
}

/*
======================================================================================================

rdPPM

reads data from file of type PPM, stores colorchannels in a struct in the
size of given picture

======================================================================================================
*/
static imagePixel_t *rdPPM(char *fileName) {
	char buffer[16];
	imagePixel_t *image;
	int c, rgbColor;
	double	 	*tmp;

	FILE *fp = fopen(fileName, "rb");
	if (!fp) {
		exit(EXIT_FAILURE);
	}
	if (!fgets(buffer, sizeof(buffer), fp)) {
		perror(fileName);
		exit(EXIT_FAILURE);
	}
	if (buffer[0] != 'P' || buffer[1] != '6') {			// PPM files start with P6
		fprintf(stderr, "No PPM file format\n");
		exit(EXIT_FAILURE);
	}
	image = (imagePixel_t *)malloc(sizeof(imagePixel_t));
	if (!image) {
		fprintf(stderr, "malloc() failed");
	}
	c = getc(fp);
	while (c == '#') {						// PPM Comments start with #
		while (getc(fp) != '\n');
		c = getc(fp);
	}
	ungetc(c, fp);
	if (fscanf(fp, "%d %d", &image->x, &image->y) != 2) {
		fprintf(stderr, "Invalid image size in %s\n", fileName);
		exit(EXIT_FAILURE);
	}
	if (fscanf(fp, "%d", &rgbColor) != 1) {
		fprintf(stderr, "Invalid rgb component in %s\n", fileName);
	}
	if (rgbColor != RGB_COLOR) {
		fprintf(stderr, "Invalid image color range in %s\n", fileName);
		exit(EXIT_FAILURE);
	}
	while (fgetc(fp) != '\n');
	image->data = (colorChannel_t *)malloc(image->x * image->y * sizeof(imagePixel_t));
	if (!image) {
		fprintf(stderr, "malloc() on image->data failed");
		exit(EXIT_FAILURE);
	}
	if ((image->x * image->y) < mlData->samplesCount) {
		printf("Changing \"-n\" to %d, image max data size\n", (image->x * image->y));
		tmp = (double *)realloc(xSamples, sizeof(double) * (image->x * image->y));
		xSamples = tmp;
		mlData->samplesCount = (image->x * image->y);
	}
	if (fread(image->data, 3 * image->x, image->y, fp) != image->y) {
		fprintf(stderr, "Loading image failed");
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	return image;
}

/*
======================================================================================================

mkPpmFile

gets output from result of rdPPM and writes a new PPM file. Best Case is a
carbon copy of the source image. Build for debugging.

======================================================================================================
*/
void mkPpmFile(char *fileName, imagePixel_t *image) {
	FILE* fp = fopen(fileName, "wb");
	if (!fp) {
		fprintf(stderr, "Opening file failed.");
		exit(EXIT_FAILURE);
	}
	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", image->x, image->y);
	fprintf(fp, "%d\n", RGB_COLOR);
	fwrite(image->data, 3 * image->x, image->y, fp);
	fclose(fp);
}

/*
======================================================================================================

ppmColorChannel

gets one of the rgb color channels and writes them to a file

======================================================================================================
*/
int ppmColorChannel(FILE* fp, imagePixel_t *image, char *colorChannel, mldata_t *mlData) {
	unsigned i = 0;

	printf("colorChannel : %s\n", colorChannel);
	if (image) {											// RGB channel can be set through args from cli 
		if (strcmp(colorChannel, "green") == 0) {
			for (i = 0; i < mlData->samplesCount - 1; i++) {
				fprintf(fp, "%d\n", image->data[i].green);
			}
		}
		else if (strcmp(colorChannel, "red") == 0) {
			for (i = 0; i < mlData->samplesCount - 1; i++) {
				fprintf(fp, "%d\n", image->data[i].red);
			}

		}
		else if (strcmp(colorChannel, "blue") == 0) {
			for (i = 0; i < mlData->samplesCount - 1; i++) {
				fprintf(fp, "%d\n", image->data[i].blue);
			}
		}
		else {
			printf("Colorchannels are red, green and blue. Pick one of them!");
			exit(EXIT_FAILURE);
		}
	}
	fclose(fp);
	return mlData->samplesCount;									// returned for debugging, TODO: void PPmcolorChannel
}

/*
======================================================================================================

colorSamples

reads colorChannel values from file and stores them in xSamples as well as in points datatype for
creating the SVG graph

======================================================================================================
*/
void colorSamples(FILE* fp, mldata_t *mlData) {
	int i = 0;
	char *buffer = (char *)malloc(sizeof(char) * mlData->samplesCount + 1);

	while (!feof(fp)) {
		if (fgets(buffer, mlData->samplesCount, fp) != NULL) {
			sscanf(buffer, "%lf", &xSamples[i]);
			points[i].yVal[0] = xSamples[i];				// Fills points so actual input values can be seen as a graph
			points[i].xVal[0] = i;
			++i;
		}
	}
	fclose(fp);
}

/*
======================================================================================================

windowXMean

returns mean value of given input

======================================================================================================
*/
double windowXMean(int _arraylength, int xCount) {
	double sum = 0.0;
	double *ptr;

	for (ptr = &xSamples[xCount - _arraylength]; ptr != &xSamples[xCount]; ptr++) { 	// Set ptr to beginning of window and iterate through array
		sum += *ptr;
	}
	return sum / (double)_arraylength;
}

/*
======================================================================================================

usage

used in conjunction with the args parser. Returns help section of "-h"

======================================================================================================
*/
void usage(char **argv) {
	printf("Usage: %s [POSIX style options] -i file ...\n", &argv[0][0]);
	printf("POSIX options:\n");
	printf("\t-h\t\t\tDisplay this information.\n");
	printf("\t-i <filename>\t\tName of inputfile. Must be PPM image.\n");
	printf("\t-n <digit>\t\tAmount of input data used.\n");
	printf("\t-c <color>\t\tUse this color channel from inputfile.\n");
	printf("\t-w <digit>\t\tCount of used weights (windowSize).\n");
	printf("\t-l <digit>\t\tLearnrate, 0 < learnrate < 1.\n");
	printf("\t-g <path> or true\t\t\tGraph building.Path if you have changed the folder of the template. Otherwise use true.\n\t\t\t\tChoose for n < 1200.\n");
	printf("\t-s <digit>\t\tDigit for random seed generator.\n\t\t\t\tSame Digits produce same random values. Default is srand by time.\n");
	printf("\n\n");
	printf("%s compares prediction methods of least mean square filters.\nBy default it reads ppm file format and return logfiles as well\nas an svg graphs as an output of said least mean square methods.\n\nExample:\n\t%s -i myimage.ppm -w 3 -c green -s 5 -g true\n", &argv[0][0], &argv[0][0]);
	exit(8);
}

/*
======================================================================================================

init_mldata_t


Init meachine learning data

======================================================================================================
*/
mldata_t * init_mldata_t(unsigned windowSize, unsigned samplesCount, double learnrate) {
	mlData = (mldata_t *)malloc(sizeof(mldata_t));
	mlData->windowSize = windowSize;
	mlData->samplesCount = samplesCount;
	mlData->learnrate = learnrate;
	mlData->weights = (double *)malloc(sizeof(double) * windowSize + 1);
	return mlData;
}
