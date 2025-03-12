//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "TODO";
const char *studentID = "TODO";
const char *email = "TODO";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tage", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//
//tage (5 component) 
uint32_t L0 = 10; //2^10 entries in table T0 (not part of the geometric series, but defining L0 for convenience)
//for the geometric series, pick r as 4, to see the effect of long history lengths
uint32_t L1 = 4;  
uint32_t L2 = 16;  
uint32_t L3 = 64;  
uint32_t L4 = 256;  

//the _pred are the 3-bit counter predictor tables
//the _u are 2-bit counter usefulness tables
uint32_t * T0_pred;

uint32_t * T1_pred;
uint32_t * T1_u;
uint32_t * T1_valid;

uint32_t * T2_pred;
uint32_t * T2_u;
uint32_t * T2_valid;

uint32_t * T3_pred;
uint32_t * T3_u;
uint32_t * T3_valid;

uint32_t * T4_pred;
uint32_t * T4_u;
uint32_t * T4_valid;

//global counter for how many branches have been predicted so far
//will be used in resetting the u values
uint64_t tage_branch_count; 

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}


void init_tage()
{
  ghistory = 0;
  
  //counter of how many branches have been predicted so far
  //will be used in resetting the u values
  tage_branch_count = 0;

  int i; 
  
  //T0
  int T0_entries = 1 << L0;  
  T0_pred = (uint32_t*)malloc(T0_entries * sizeof(uint8_t));
  for(i=0; i<T0_entries; i++){
    T0_pred[i] = WN; //T0 is a 2-bit counter
  }  
  
  //T1
  int T1_entries = 1 << L1;  
  T1_pred = (uint32_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T1_u = (uint32_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T1_valid = (uint32_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_valid[i] = 0; //initialize to invalid
  }  

  //T2
  int T2_entries = 1 << L2;  
  T2_pred = (uint32_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T2_u = (uint32_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T2_valid = (uint32_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_valid[i] = 0; //initialize to invalid
  }  
 
  //T3 
  int T3_entries = 1 << L3;  
  T3_pred = (uint32_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T3_u = (uint32_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T3_valid = (uint32_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_valid[i] = 0; //initialize to invalid
  }  

  //T4
  int T4_entries = 1 << L4;  
  T4_pred = (uint32_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T4_u = (uint32_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T4_valid = (uint32_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_valid[i] = 0; //initialize to invalid
  }  
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t tage_predict(uint32_t pc)
{
  //first calculated the indexes for each of the tables
  //note that for all tables except T0, the hash function is XOR
  uint32_t T0_entries = 1 << L0;
  uint32_t T0_idx = pc & (T0_entries - 1); 
  
  uint32_t T1_entries = 1 << L1;
  uint32_t T1_idx = (pc & (T1_entries - 1)) ^ (ghistory & (T1_entries - 1)); 

  uint32_t T2_entries = 1 << L2;
  uint32_t T2_idx = (pc & (T2_entries - 1)) ^ (ghistory & (T2_entries - 1)); 

  uint32_t T3_entries = 1 << L3;
  uint32_t T3_idx = (pc & (T3_entries - 1)) ^ (ghistory & (T3_entries - 1)); 

  uint32_t T4_entries = 1 << L4;
  uint32_t T4_idx = (pc & (T4_entries - 1)) ^ (ghistory & (T4_entries - 1)); 

  //FIXME implement selection logic across tables

  uint8_t default_pred = T0_pred[T0_idx];
  //return default prediction if nothing else was found
  switch (default_pred)
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
  
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void train_tage()
{
  //TODO
}

void cleanup_gshare()
{
  free(bht_gshare);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TAGE:
    break;
  case CUSTOM:
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TAGE:
    return NOTTAKEN;
  case CUSTOM:
    return NOTTAKEN;
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TAGE:
      return;
    case CUSTOM:
      return;
    default:
      break;
    }
  }
}
