//========================================================//
//  predictor.h                                           //
//  Header file for the Branch Predictor                  //
//                                                        //
//  Includes function prototypes and global predictor     //
//  variables and defines                                 //
//========================================================//

#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <stdint.h>
#include <stdlib.h>

//
// Student Information
//
extern const char *studentName;
extern const char *studentID;
extern const char *email;

//------------------------------------//
//      Global Predictor Defines      //
//------------------------------------//
#define NOTTAKEN 0
#define TAKEN 1

// The Different Predictor Types
#define STATIC 0
#define GSHARE 1
#define TAGE 2
#define CUSTOM 3
extern const char *bpName[];

// Definitions for 2-bit counters
#define SN 0 // predict NT, strong not taken
#define WN 1 // predict NT, weak not taken
#define WT 2 // predict T, weak taken
#define ST 3 // predict T, strong taken

#define INVALID 0xBC

// Definitions for 2-bit usefulness counters
#define SNU 0 // strong not useful 
#define WNU 1 // weak not useful
#define WU 2 // weak useful
#define SU 3 // strong useful

//set 256K as the number of branches after which the u values must be reset
#define TAGE_RESET_PERIOD 262114

//number of entries in each of the tables T1,T2,T3,T4
//note that in hardware these are effectively fully associative caches
//however, for our purposes, we have a "reverse mapping" in terms of the
//_valid tables. which means that the sum of all valids in each table
//cannot exceed this count
//FIXME for now, start with an infinite table size, so that a "valid"
//entry is always available
#define TAGGED_PREDICTOR_SIZE 1024 

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//
extern int ghistoryBits; // Number of bits used for Global History
extern int lhistoryBits; // Number of bits used for Local History
extern int pcIndexBits;  // Number of bits used for PC index
extern int bpType;       // Branch Prediction Type
extern int verbose;

//------------------------------------//
//    Predictor Function Prototypes   //
//------------------------------------//

// Initialize the predictor
//
void init_predictor();

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct);

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct);

// Please add your code below, and DO NOT MODIFY ANY OF THE CODE ABOVE
// 

//print out debug variables
void dbg_prints();

#endif
