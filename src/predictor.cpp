//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <cstdlib>
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
int8_t * T0_pred;

int8_t * T1_pred;
uint8_t * T1_u;
uint8_t * T1_valid;

int8_t * T2_pred;
uint8_t * T2_u;
uint8_t * T2_valid;

int8_t * T3_pred;
uint8_t * T3_u;
uint8_t * T3_valid;

int8_t * T4_pred;
uint8_t * T4_u;
uint8_t * T4_valid;


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
  T0_pred = (int8_t*)malloc(T0_entries * sizeof(int8_t));
  for(i=0; i<T0_entries; i++){
    T0_pred[i] = WN; //T0 is a 2-bit counter
  }  
  
  //T1
  int T1_entries = 1 << L1;  
  T1_pred = (int8_t*)malloc(T1_entries * sizeof(int8_t));
  for(i=0; i<T1_entries; i++){
    T1_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T1_u = (uint8_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T1_valid = (uint8_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_valid[i] = 0; //initialize to invalid
  }  

  //T2
  int T2_entries = 1 << L2;  
  T2_pred = (int8_t*)malloc(T2_entries * sizeof(int8_t));
  for(i=0; i<T2_entries; i++){
    T2_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T2_u = (uint8_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T2_valid = (uint8_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_valid[i] = 0; //initialize to invalid
  }  
 
  //T3 
  int T3_entries = 1 << L3;  
  T3_pred = (int8_t*)malloc(T3_entries * sizeof(int8_t));
  for(i=0; i<T3_entries; i++){
    T3_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T3_u = (uint8_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T3_valid = (uint8_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_valid[i] = 0; //initialize to invalid
  }  

  //T4
  int T4_entries = 1 << L4;  
  T4_pred = (int8_t*)malloc(T4_entries * sizeof(int8_t));
  for(i=0; i<T4_entries; i++){
    T4_pred[i] = 0; //initialize to the 3-bit equivalent of "weakly not taken" which will switch most easily to taken 
  }  
  T4_u = (uint8_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T4_valid = (uint8_t*)malloc(T4_entries * sizeof(uint8_t));
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

void tage_walk(uint32_t pc, uint8_t& T0_idx, uint8_t& T1_idx,uint8_t& T2_idx,uint8_t& T3_idx,uint8_t& T4_idx, uint8_t& pred, uint8_t& provider, uint8_t& altpred)
{
  //first calculated the indexes for each of the tables
  //note that for all tables except T0, the hash function is XOR
  uint32_t T0_entries = 1 << L0;
  T0_idx = pc & (T0_entries - 1); 
  
  uint32_t T1_entries = 1 << L1;
  T1_idx = (pc & (T1_entries - 1)) ^ (ghistory & (T1_entries - 1)); 

  uint32_t T2_entries = 1 << L2;
  T2_idx = (pc & (T2_entries - 1)) ^ (ghistory & (T2_entries - 1)); 

  uint32_t T3_entries = 1 << L3;
  T3_idx = (pc & (T3_entries - 1)) ^ (ghistory & (T3_entries - 1)); 

  uint32_t T4_entries = 1 << L4;
  T4_idx = (pc & (T4_entries - 1)) ^ (ghistory & (T4_entries - 1)); 


  uint8_t default_pred; 
  switch (T0_pred[T0_idx])
  {
  case WN:
    default_pred = NOTTAKEN;
  case SN:
    default_pred = NOTTAKEN;
  case WT:
    default_pred = TAKEN;
  case ST:
    default_pred = TAKEN;
  default:
    printf("Warning: Undefined state of entry in table T0 !\n");
    default_pred = NOTTAKEN;
    break;
  }

  //predictions from T1-T4
  uint8_t t1_pred = T1_valid[T1_idx] ? ( (T1_pred[T1_idx]>0) ? TAKEN : NOTTAKEN ) : INVALID; 
  uint8_t t2_pred = T2_valid[T2_idx] ? ( (T2_pred[T2_idx]>0) ? TAKEN : NOTTAKEN ) : INVALID; 
  uint8_t t3_pred = T3_valid[T3_idx] ? ( (T3_pred[T3_idx]>0) ? TAKEN : NOTTAKEN ) : INVALID; 
  uint8_t t4_pred = T4_valid[T4_idx] ? ( (T4_pred[T4_idx]>0) ? TAKEN : NOTTAKEN ) : INVALID;  

  provider = 0xBC; 
  
  //choose the prediction with the longest branch history 
  if(t4_pred != INVALID)
  {
    pred = t4_pred; 
    provider = 4;    
  }
  else
  if(t3_pred != INVALID)
  {
    pred = t3_pred;
    provider = 3;    
  }
  else
  if(t2_pred != INVALID)
  {
    pred = t2_pred;
    provider = 2;    
  }
  else
  if(t1_pred != INVALID)
  {
    pred = t1_pred;
    provider = 1;    
  }
  else
  {
    pred = default_pred; 
    provider = 0;    
  }

  //altpred computation
  altpred = 0;
  if(provider == 4)
  {
    if(t3_pred != INVALID) 
      altpred = 3;
    else
    if(t2_pred != INVALID)
      altpred = 2;
    else
    if(t1_pred != INVALID)
      altpred = 1;
  }
  else
  if(provider == 3)
  {
    if(t2_pred != INVALID)
      altpred = 2;
    else
    if(t1_pred != INVALID)
      altpred = 1;
  }
  else
  if(provider == 2)
  {
    if(t1_pred != INVALID)
      altpred = 1;
  }
  else
    altpred = 0;
}

uint8_t tage_predict(uint32_t pc)
{
  uint8_t T0_idx;
  uint8_t T1_idx;
  uint8_t T2_idx;
  uint8_t T3_idx;
  uint8_t T4_idx;

  //default prediction from T0
  uint8_t default_pred; 
  //final prediction
  uint8_t pred;
  //who is the provider
  uint8_t provider; 
  //altpred
  uint8_t altpred;

  tage_walk(pc, T0_idx, T1_idx, T2_idx, T3_idx, T4_idx, pred, provider, altpred);

  return pred; 
  
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


void update_usefulness(int pred_correct ,uint8_t & u )
{
  //if the prediction was correct, the usefulness counter is incremented. Else, it is decremented
  if(pred_correct)
  {
    switch(u)
    {
      case SNU:
        u = WNU;
      case WNU:
        u = WU;
      case WU:
        u = SU;
      case SU:
        //NOP
      default:
        printf("Warning: undefined state of usefulness entry");
        break;
    }
  }
  else
  {
    switch(u)
    {
      case SNU:
        //NOP
      case WNU:
        u = SNU;
      case WU:
        u = WNU;
      case SU:
        u = WU; 
      default:
        printf("Warning: undefined state of usefulness entry");
        break;
    }
  }
}

void periodic_usefulness_reset()
{
 	tage_branch_count++;
    int even_cycle = 0; //reset MSBs
	int odd_cycle = 0; //reset LSBs
    uint8_t usefulness_mask = 0x03;
    if(tage_branch_count % TAGE_RESET_PERIOD == 0)
	{
		if(tage_branch_count % (2*TAGE_RESET_PERIOD)) 
			{
			even_cycle = 1;
			usefulness_mask = 0x01;
			}
		else
			{
			odd_cycle = 1;
			usefulness_mask = 0x02;
			}
		//iterate over all usefulness values and apply the mask
		int i;

  		int T1_entries = 1 << L1;  
  		for(i=0; i<T1_entries; i++){
  		  T1_u[i] = T1_u[i] & usefulness_mask;
  		}  
  		int T2_entries = 1 << L2;  
  		for(i=0; i<T2_entries; i++){
  		  T2_u[i] = T2_u[i] & usefulness_mask;
  		}  
  		int T3_entries = 1 << L3;  
  		for(i=0; i<T3_entries; i++){
  		  T3_u[i] = T3_u[i] & usefulness_mask;
  		}  
  		int T4_entries = 1 << L4;  
  		for(i=0; i<T4_entries; i++){
  		  T4_u[i] = T4_u[i] & usefulness_mask;
  		}  
	} 	
}

//function to update the prediction counter of an entry in T1,...,T4
void update_pred(uint8_t outcome, int8_t & entry )
{
	entry = (outcome == TAKEN) ? std::max<int8_t>(entry+1, 3) : std::min<int8_t>(entry-1, -4); 
}

void train_tage(uint32_t pc, uint8_t outcome)
{
  uint8_t T0_idx;
  uint8_t T1_idx;
  uint8_t T2_idx;
  uint8_t T3_idx;
  uint8_t T4_idx;

  //default prediction from T0
  uint8_t default_pred; 
  //final prediction
  uint8_t pred;
  //who is the provider
  uint8_t provider; 
  //altpred
  uint8_t altpred;

  tage_walk(pc, T0_idx, T1_idx, T2_idx, T3_idx, T4_idx, pred, provider, altpred);

  //update usefulness
  int pred_correct = (pred == outcome) ? 1 : 0;
  switch(provider)
  {
    case 0:
      //NOP
    case 1:
      update_usefulness(pred_correct, T1_u[T1_idx]);
    case 2:
      update_usefulness(pred_correct, T2_u[T2_idx]);
    case 3:
      update_usefulness(pred_correct, T3_u[T3_idx]);
    case 4:
      update_usefulness(pred_correct, T3_u[T3_idx]);
  default:
    printf("Warning: Undefined state of provider in TAGE !\n");
    break;
  }
 
  //update prediction counter on correct prediction
  if(pred_correct)
  {
  	switch(provider)
  	{
  	  case 0:
  	    {
  	      switch(T0_pred[T0_idx])
  	      {
		  //these updates are in the case when the prediction matched the outcome
  	  	  case WN:
  	  		T0_pred[T0_idx] = SN;
  	  	  case SN:
  	  		//NOP
  	  	  case WT:
  	  		T0_pred[T0_idx] = ST;
  	        case ST:
  	  		//NOP
  	        default:
  	          printf("Warning: undefined state of entry in table T0 during training");
  	      }
  	    }
  	  case 1:
  	    update_pred(outcome, T1_pred[T1_idx]); //these are signed 3 bit counters
  	  case 2:
  	    update_pred(outcome, T2_pred[T2_idx]); //these are signed 3 bit counters
  	  case 3:
  	    update_pred(outcome, T3_pred[T3_idx]); //these are signed 3 bit counters
  	  case 4:
  	    update_pred(outcome, T4_pred[T4_idx]); //these are signed 3 bit counters
  	default:
  	  printf("Warning: Undefined state of provider in TAGE !\n");
  	  break; 
  	}
  } 

  //updates if overall prediction is incorrect
  else 
  {
    //update the provider ctr 
  	switch(provider)
  	{
  	  case 0:
  	    {
  	      switch(T0_pred[T0_idx])
  	      {
		  //these updates are in the case when the prediction did not match the outcome
  	  	  case WN:
  	  		T0_pred[T0_idx] = WT;
  	  	  case SN:
  	  		T0_pred[T0_idx] = WN;
  	  	  case WT:
  	  		T0_pred[T0_idx] = WN;
  	        case ST:
  	  		T0_pred[T0_idx] = WT;
  	        default:
  	          printf("Warning: undefined state of entry in table T0 during training");
  	      }
  	    }
  	  case 1:
  	    update_pred(outcome, T1_pred[T1_idx]); //these are signed 3 bit counters
  	  case 2:
  	    update_pred(outcome, T2_pred[T2_idx]); //these are signed 3 bit counters
  	  case 3:
  	    update_pred(outcome, T3_pred[T3_idx]); //these are signed 3 bit counters
  	  case 4:
  	    update_pred(outcome, T4_pred[T4_idx]); //these are signed 3 bit counters
  	default:
  	  printf("Warning: Undefined state of provider in TAGE !\n");
  	  break; 
  	}
    //if the provider was NOT the component with the longest history (i.e. T4 in our case),
    if (provider != 4)
    {
    	//allocate a new entry with a longer history
		//FIXME with our ideal case of having as big tables T1,...,T4 as we need, 
		//our Tj and Tk become entries in T4 and T3
 
	    //pick Tj or Tk randomly, with Tj having twice the probability of Tk, j<k
		int allocation;
		int random_value = std::rand() % 3;
		if(random_value<2)
			allocation = 3;
		else
			allocation = 4;
				
	
		//initialize the newly allocated entry
		
		if(allocation == 3)
		{
			T3_valid[T3_idx] = 1;
			T3_u[T3_idx] = SNU;
			//prediction counter set to weak correct
			T3_pred[T3_idx] = (outcome == TAKEN) ? 1 : 0;  
		}
		else 
		if(allocation == 4)
		{
			T4_valid[T4_idx] = 1;
			T4_u[T4_idx] = SNU;
			//prediction counter set to weak correct
			T4_pred[T4_idx] = (outcome == TAKEN) ? 1 : 0;  
		}
		else
			printf("Warning: something went wrong in choosing randomly between Tj and Tk !\n"); 


    } //end of provider != 4 case


  } //end of prediction not correct case

  //periodic alternate reset of usefulness counters
  periodic_usefulness_reset();

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
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
