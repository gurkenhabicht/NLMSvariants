//
//  main.c
//  NLMS
//
//  Created by FBRDNLMS on 26.04.18.
//  Copyright © 2018 FBRDNLMS. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define M 1000
#define tracking 40 //Count of weights
//static Stack<double> x = new Stack<double>();
//static Random rnd = new Random();
//static double[] _x = new double[M];
//static double[,] w = new double[M, M];
#define learnrate 1.0

double x[] ={0};
double _x[M] = {0};
double w [M][M]={{0},{0}};


void fileName(char *name_suffix);
double r2(void);
double rnd(void);
double sum_array(double x[], int length);
void direkterVorgaenger(void);
void lokalerMittelWert(void);





int main(int argc, char **argv ) {

    //init test_array, fill in weights by random
    int i = 0;
    for (i = 0; i < M; i++) {
        _x[i] += ((255.0 / M) * i);
        for (int k = 1; k < M; k++)
        {
            w[k][i] = rnd();
        }
    }

    // save plain test_array before math magic happened
    char weightsBefore  [] = "_weights.txt";
    fileName(&weightsBefore);
    FILE *fp0 = fopen(weightsBefore,"wb+");

    for (i = 0; i < tracking; i++){
      for ( int k = 1; k < tracking; k++ ){
        fprintf(fp0, "[%f][%f] %.2f\n", k, i, w[k][i]);
      }
    }
    fclose(fp0);


    // math magic
    direkterVorgaenger();
   

    // save test_array after math magic happened
    char weightsAfter [] = "_weights_after.txt";
    fileName(&weightsAfter);
    FILE *fp1 = fopen(weightsAfter,"wb+");

    for (i = 0; i < tracking; i++) {
        for (int k = 1; k < tracking; k++) {
            fprintf(fp1, "[%f][%f] %.2f\n", k,i, w[k][i]);
	}
        
    }
    fclose(fp1);
    
    getchar();
    
}


/*
 
 ===================================
 
 lokalerMittelwert()
 
 
 Variant (1/3), 
 substract local mean

 ===================================
*/ 

void lokalerMittelWert() {

    double xError[M]; // includes e(n)
    memset(xError, 0, M);// initialize xError-array with Zero
    int xCount = 0; // runtime var	
    int i;
	

    for (xCount = 1; xCount < M; xCount++){ // x_cout can not be zero 
        
	//double xPartArray[xCount]; //includes all values at the size of runtime var

	double xMean = (xCount > 0) ? ( sum_array(_x, xCount) / xCount) : 0;
        
	double xPredicted = 0.0;
	double xActual = _x[xCount + 1];

	for ( i = 1; i < xCount; i++ ){ //get predicted value
	  xPredicted += (w[i][xCount] * (_x[xCount - i] - xMean)) ;
	}
	
	xPredicted += xMean;
	xError [xCount] = xActual - xPredicted;

	double xSquared = 0.0;

	for ( i = 1; i < xCount; i++ ){ //get x squared
          xSquared =+ pow(_x[xCount-i],2);
        }
	
	for ( i - 1; i < xCount; i++ ){ //update weights
	  w[i][xCount+1] = w[i][xCount] + learnrate * xError[xCount] * (_x[xCount - i] / xSquared);
	}
    }
    
    int xErrorLength = sizeof(xError) / sizeof(xError[0]);
    double mean = sum_array(xError, xErrorLength) / M;
    double deviation = 0.0;
   
    // Mean square
    for( i = 0; i < M-1; i++ ){
      deviation += pow( xError[i], 2 );
    }
    deviation /= xErrorLength;

   
    // write in file
    char results [] = "_results.txt";
    fileName(&results);
    FILE *fp2 = fopen(results, "wb+"); 
    fprintf(fp2, "quadr. Varianz(x_error): {%f}\nMittelwert:(x_error): {%f}\n\n", deviation, mean);
    fclose(fp2);

}


/*
 
 ===================================
 
 direkterVorgaenger() 
 
 
 Variant (2/3), 
 substract direct predecessor

 ===================================
*/ 

void direkterVorgaenger() {
   
    double xError [M];
    int xCount = 0, i;

    // File handling
    char direkterVorgaenger [] = "_direkterVorgaenger.txt";
    fileName(&direkterVorgaenger);
    FILE *fp3 = fopen(direkterVorgaenger, "wb+");	

    for ( xCount = 1; xCount < M; xCount++ ){
      double xPredicted = 0.0;
      double xActual = _x[xCount+1];
      
      for ( i = 1; i < xCount; i++ ){
        xPredicted += ( w[i][xCount] * ( _x[xCount - i] - _x[xCount - i - 1]));
      }

      xPredicted += _x[xCount-1]; 
      xError[xCount] = xActual - xPredicted;

      fprintf(fp3, "{%d}.\txPredicted{%f}\txActual{%f}\txError{%f}\n", xCount, xPredicted, xActual, xError[xCount]);


      //get x squared
      double xSquared = 0;
      for ( i = 1; i < xCount; i++ ){
	xSquared += pow( _x[xCount - i] - _x[xCount - i - 1], 2); // substract direct predecessor
      }	

      for ( i = 1; x < xCount; i++){
        w[i][xCount+1] = w[i][xCount] + learnrate * xError[xCount] * ( ( _x[xCount - i - 1] ) / xSquared );
      }
    }
    int xErrorLength = sizeof(xError) / sizeof(xError[0]);  
    double mean = sum_array(xError, xErrorLength) / xErrorLength;
    double deviation = 0.0;

    for ( i = 0; i < xErrorLength -1; i++ ){
      deviation += pow( xError[i] - mean, 2);
    }

    
    fprintf(fp3, "{%d}.\tLeast Mean Squared{%f}\tMean{%f}\n\n", xCount, deviation, mean);
    fclose(fp3);
}


/*
 
 ===================================
 
 fileName()
 
 
 generates filename with date for
 logging purposes

 ===================================
*/ 

void fileName(char *name_suffix){
    //filename
    char date[34];
    //char name[13] = "_results.txt";
    time_t now;
    now = time(NULL);
    strftime(date, 20, "%Y-%m-%d_%H_%M_%S", localtime(&now));
    strcpy(date, *name_suffix);
    //return &date[0];
    return;
}


/*
 
 ===================================
 
 sum_array
 
 
 sum of all elements in x
 within a defined length
 
 ===================================
 
 */

double sum_array(double x[], int length){
    //int length = 0;
    int i = 0;
    double sum = 0.0;
    //length = sizeof(x)/sizeof(x[0]);
    for (i=0; i< length; i++){
        sum += x[i];
    }
    return sum;
}


/*

 ===================================


 r2()

 returns a double value between
 0 and 1

 ===================================

*/
double r2()
{
    return((rand() % 10000) / 10000.0);
}

/*

 ===================================


 int rnd()

 fills a double variable with
 random value and returns it

 ===================================

*/
double rnd()
{
    double u;
    u = r2();
    return u;
}
