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

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tage", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 18; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;
int tag_size = 5;
int tag_mask = (1 << tag_size) - 1;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//tage (5 component)
uint32_t L0 = 10; //2^10 entries in table T0 (not part of the geometric series, but defining L0 for convenience)
//for the geometric series, pick r as 4, to see the effect of long history lengths
//FIXME try with r as 2
uint32_t L1 = 2;  
uint32_t L2 = 4;  
uint32_t L3 = 8;  
uint32_t L4 = 16;  

uint32_t T0_entries = 1 << L0;
uint32_t T1_entries = 1 << L1;
uint32_t T2_entries = 1 << L2;
uint32_t T3_entries = 1 << L3;
uint32_t T4_entries = 1 << L4;

//the _pred are the 3-bit counter predictor tables
//the _u are 2-bit counter usefulness tables
int8_t * T0_pred;

int8_t * T1_pred;
uint8_t * T1_u;
uint8_t * T1_valid;
uint8_t * T1_tag;

int8_t * T2_pred;
uint8_t * T2_u;
uint8_t * T2_valid;
uint8_t * T2_tag;

int8_t * T3_pred;
uint8_t * T3_u;
uint8_t * T3_valid;
uint8_t * T3_tag;

int8_t * T4_pred;
uint8_t * T4_u;
uint8_t * T4_valid;
uint8_t * T4_tag;


//global counter for how many branches have been predicted so far
//will be used in resetting the u values
uint64_t tage_branch_count; 

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
//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

//debug variables
int dbg_t0_provider = 0;
int dbg_t1_provider = 0;
int dbg_t2_provider = 0;
int dbg_t3_provider = 0;
int dbg_t4_provider = 0;
int dbg_t1_allocated = 0;
int dbg_t2_allocated = 0;
int dbg_t3_allocated = 0;
int dbg_t4_allocated = 0;
int dbg_predict_taken = 0;
int dbg_predict_nottaken = 0;
int dbg_prediction_match = 0;

void dbg_prints()
{
	printf("\n======= TAGE DEBUG VARIABLES =======\n\n");
	printf("Number of times T1 was allocated                 :    %d\n",dbg_t1_allocated);	
	printf("Number of times T2 was allocated                 :    %d\n",dbg_t2_allocated);	
	printf("Number of times T3 was allocated                 :    %d\n",dbg_t3_allocated);	
	printf("Number of times T4 was allocated                 :    %d\n",dbg_t4_allocated);
	printf("Number of times T0 was provider                 :    %d\n",dbg_t0_provider);	
	printf("Number of times T1 was provider                 :    %d\n",dbg_t1_provider);	
	printf("Number of times T2 was provider                 :    %d\n",dbg_t2_provider);	
	printf("Number of times T3 was provider                 :    %d\n",dbg_t3_provider);	
	printf("Number of times T4 was provider                 :    %d\n",dbg_t4_provider);
	printf("Total Number of Predictions                     :    %d\n",dbg_t0_provider+dbg_t1_provider+dbg_t2_provider+dbg_t3_provider+dbg_t4_provider);
//FIXME: Why is the total count coming out twice the number of branches?
	printf("Number of times TAKEN was predicted             :    %d\n",dbg_predict_taken);	
	printf("Number of times NOT TAKEN was predicted         :    %d\n",dbg_predict_nottaken);	
	printf("Number of times prediction matched the outcome  :    %d\n",dbg_prediction_match);	
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor

//##################
// gshare functions
//##################

void init_gshare()
{
  // allocate memory for 2^ghistoryBits entries in BHT
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // XOR the lower ghistoryBits of PC, GHR
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t index =  (pc & (bht_entries - 1)) ^ (ghistory & (bht_entries - 1));

  // Return the prediction based on state
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

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // XOR the lower ghistoryBits of PC, GHR
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t index =  (pc & (bht_entries - 1)) ^ (ghistory & (bht_entries - 1));

  // Update state of entry in BHT based on outcome
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

void cleanup_gshare()
{
  free(bht_gshare);
}


//################
// tage functions
//################

void init_tage()
{
  ghistory = 0;
  
  //counter of how many branches have been predicted so far
  //will be used in resetting the u values
  tage_branch_count = 0;

  int i; 

  //Create tables T0, T1, T2, T3, T4
  //T0 has a single column for 2-bit unsigned predictor
  //Each of T1, T2, T3, T4 has columns for 3-bit signed predictor, 2-bit usefulness counter, valid
  //FIXME: What do we need valid for?

  // allocate memory for 2^L0 entries in T0
  T0_pred = (int8_t*)malloc(T0_entries * sizeof(int8_t));
  for(i=0; i<T0_entries; i++){
    T0_pred[i] = WN; //initialize to WN which will switch most easily to taken
  }  
  
  // allocate memory for 2^L1 entries in T1
  T1_pred = (int8_t*)malloc(T1_entries * sizeof(int8_t));
  for(i=0; i<T1_entries; i++){
    T1_pred[i] = -1; //initialize to WN which will switch most easily to taken 
  }  
  T1_u = (uint8_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T1_valid = (uint8_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_valid[i] = 0x0; //initialize to invalid
  }
  T1_tag = (uint8_t*)malloc(T1_entries * sizeof(uint8_t));
  for(i=0; i<T1_entries; i++){
    T1_tag[i] = 0xBC; //initialize to invalid
  }   

  // allocate memory for 2^L2 entries in T2
  int T2_entries = 1 << L2;  
  T2_pred = (int8_t*)malloc(T2_entries * sizeof(int8_t));
  for(i=0; i<T2_entries; i++){
    T2_pred[i] = -1; //initialize to WN which will switch most easily to taken 
  }  
  T2_u = (uint8_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T2_valid = (uint8_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_valid[i] = 0x0; //initialize to invalid
  }  
  T2_tag = (uint8_t*)malloc(T2_entries * sizeof(uint8_t));
  for(i=0; i<T2_entries; i++){
    T2_tag[i] = 0xBC; //initialize to invalid
  } 

  // allocate memory for 2^L3 entries in T3
  int T3_entries = 1 << L3;  
  T3_pred = (int8_t*)malloc(T3_entries * sizeof(int8_t));
  for(i=0; i<T3_entries; i++){
    T3_pred[i] = -1; //initialize to WN which will switch most easily to taken 
  }  
  T3_u = (uint8_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T3_valid = (uint8_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_valid[i] = 0x0; //initialize to invalid
  }  
  T3_tag = (uint8_t*)malloc(T3_entries * sizeof(uint8_t));
  for(i=0; i<T3_entries; i++){
    T3_tag[i] = 0xBC; //initialize to invalid
  }
 
  // allocate memory for 2^L4 entries in T4
  int T4_entries = 1 << L4;  
  T4_pred = (int8_t*)malloc(T4_entries * sizeof(int8_t));
  for(i=0; i<T4_entries; i++){
    T4_pred[i] = -1; //initialize to WN which will switch most easily to taken 
  }  
  T4_u = (uint8_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_u[i] = SNU; //initialize to strongly not useful as per TAGE paper
  }  
  T4_valid = (uint8_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_valid[i] = 0x0; //initialize to invalid
  } 
  T4_tag = (uint8_t*)malloc(T4_entries * sizeof(uint8_t));
  for(i=0; i<T4_entries; i++){
    T4_tag[i] = 0xBC; //initialize to invalid
  }  
}

void tage_walk(uint32_t pc, uint8_t& T0_idx, uint8_t& T1_idx,uint8_t& T2_idx,uint8_t& T3_idx,uint8_t& T4_idx, uint8_t& pred, uint8_t& provider, uint8_t& altpred)
{
  //first calculated the indexes for each of the tables
  //note that for all tables except T0, the hash function is XOR

  T0_idx = pc & (T0_entries - 1); 
  T1_idx = (pc & (T1_entries - 1)) ^ (ghistory & (T1_entries - 1)); 
  T2_idx = (pc & (T2_entries - 1)) ^ (ghistory & (T2_entries - 1)); 
  T3_idx = (pc & (T3_entries - 1)) ^ (ghistory & (T3_entries - 1)); 
  T4_idx = (pc & (T4_entries - 1)) ^ (ghistory & (T4_entries - 1)); 


  uint8_t default_pred; 
  switch (T0_pred[T0_idx])
  {
  case WN:
    default_pred = NOTTAKEN;
	break;
  case SN:
    default_pred = NOTTAKEN;
	break;
  case WT:
    default_pred = TAKEN;
	break;
  case ST:
    default_pred = TAKEN;
	break;
  default:
    printf("Warning: Undefined state of entry in table T0 !\n");
    default_pred = NOTTAKEN;
    break;
  }

  //predictions from T1-T4 using 2 bits after index as tag
/*
  uint8_t t1_pred = T1_valid[T1_idx] && (T1_tag[T1_idx] == ((pc >> (T1_entries - 1)) & tag_mask)) ? ( (T1_pred[T1_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID; 
  uint8_t t2_pred = T2_valid[T2_idx] && (T2_tag[T2_idx] == ((pc >> (T2_entries - 1)) & tag_mask)) ? ( (T2_pred[T2_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID; 
  uint8_t t3_pred = T3_valid[T3_idx] && (T3_tag[T3_idx] == ((pc >> (T3_entries - 1)) & tag_mask)) ? ( (T3_pred[T3_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID; 
  uint8_t t4_pred = T4_valid[T4_idx] && (T4_tag[T4_idx] == ((pc >> (T4_entries - 1)) & tag_mask)) ? ( (T4_pred[T4_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID; 
*/
 
  //predictions from T1-T4 using lower 2 bits as tag
  uint8_t t1_pred = (T1_u[T1_idx]==SNU||T1_u[T1_idx]==WNU) ? INVALID : ((T1_tag[T1_idx] == (pc & tag_mask)) ? ( (T1_pred[T1_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID); 
  uint8_t t2_pred = (T2_u[T2_idx]==SNU||T2_u[T2_idx]==WNU) ? INVALID : ((T2_tag[T2_idx] == (pc & tag_mask)) ? ( (T2_pred[T2_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID); 
  uint8_t t3_pred = (T3_u[T3_idx]==SNU||T3_u[T3_idx]==WNU) ? INVALID : ((T3_tag[T3_idx] == (pc & tag_mask)) ? ( (T3_pred[T3_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID); 
  uint8_t t4_pred = (T4_u[T4_idx]==SNU||T4_u[T4_idx]==WNU) ? INVALID : ((T4_tag[T4_idx] == (pc & tag_mask)) ? ( (T4_pred[T4_idx]>=0) ? TAKEN : NOTTAKEN ) : INVALID);  

  provider = 0xBC; 

  int provider_found = 0;
  
  //choose the prediction with the longest branch history 
  if(t4_pred != INVALID)
  {
    pred = t4_pred; 
    provider = 4; 
	dbg_t4_provider++;   
    provider_found = 1;
  }
  else if(!provider_found && t3_pred != INVALID)
  {
    pred = t3_pred;
    provider = 3;    
	dbg_t3_provider++;   
    provider_found = 1;
  }
  else if(!provider_found && t2_pred != INVALID)
  {
    pred = t2_pred;
    provider = 2;    
	dbg_t2_provider++;   
    provider_found = 1;
  }
  else if(!provider_found && t1_pred != INVALID)
  {
    pred = t1_pred;
    provider = 1;    
	dbg_t1_provider++;   
    provider_found = 1;
  }
  else
  {
    pred = default_pred; 
    provider = 0;    
	dbg_t0_provider++;   
    provider_found = 1;
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

  tage_walk(pc, T0_idx, T1_idx, T2_idx, T3_idx, T4_idx, pred, provider, altpred);

  if(pred == TAKEN)
    dbg_predict_taken++;

  else if(pred == NOTTAKEN)
    dbg_predict_nottaken++;

  return pred; 
  
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
		break;
      case WNU:
        u = WU;
		break;
      case WU:
        u = SU;
		break;
      case SU:
        //NOP
		break;
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
		break;
      case WNU:
        u = SNU;
		break;
      case WU:
        u = WNU;
		break;
      case SU:
        u = WU; 
		break;
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

  		for(i=0; i<T1_entries; i++){
  		  T1_u[i] = T1_u[i] & usefulness_mask;
  		}  
  		for(i=0; i<T2_entries; i++){
  		  T2_u[i] = T2_u[i] & usefulness_mask;
  		}  
  		for(i=0; i<T3_entries; i++){
  		  T3_u[i] = T3_u[i] & usefulness_mask;
  		}  
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

  //update usefulness
  int pred_correct = (pred == outcome) ? 1 : 0;
  switch(provider)
  {
    case 0:
      //NOP
	  break;
    case 1:
      update_usefulness(pred_correct, T1_u[T1_idx]);
	  break;
    case 2:
      update_usefulness(pred_correct, T2_u[T2_idx]);
	  break;
    case 3:
      update_usefulness(pred_correct, T3_u[T3_idx]);
	  break;
    case 4:
      update_usefulness(pred_correct, T3_u[T3_idx]);
	  break;
  	default:
  	  printf("Warning: Undefined state of provider in TAGE !\n");
  	  break;
  }
 
  //update prediction counter on correct prediction
  if(pred_correct)
  {
    dbg_prediction_match++;
  	switch(provider)
  	{
      // T0 has 2-bit predictors
  	  case 0:
  	    {
  	      switch(T0_pred[T0_idx])
  	      {
		  //these updates are in the case when the prediction matched the outcome
  	  	  case WN:
  	  		T0_pred[T0_idx] = SN;
	  		break;
  	  	  case SN:
  	  		//NOP
	  		break;
  	  	  case WT:
  	  		T0_pred[T0_idx] = ST;
	  		break;
  	      case ST:
  	  		//NOP
	  		break;
  	      default:
  	        printf("Warning: undefined state of entry in table T0 during training");
	  		break;
  	      }
		break;
  	    }
      // T1, T2, T3, T4 have signed 3 bit counters
  	  case 1:
  	    update_pred(outcome, T1_pred[T1_idx]); //these are signed 3 bit counters
	  	break;
  	  case 2:
  	    update_pred(outcome, T2_pred[T2_idx]); //these are signed 3 bit counters
	  	break;
  	  case 3:
  	    update_pred(outcome, T3_pred[T3_idx]); //these are signed 3 bit counters
	  	break;
  	  case 4:
  	    update_pred(outcome, T4_pred[T4_idx]); //these are signed 3 bit counters
	  	break;
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
      // T0 has 2-bit predictors
  	  case 0:
  	    {
  	      switch(T0_pred[T0_idx])
  	      {
		  //these updates are in the case when the prediction did not match the outcome
  	  	  case WN:
  	  		T0_pred[T0_idx] = WT;
			break;
  	  	  case SN:
  	  		T0_pred[T0_idx] = WN;
			break;
  	  	  case WT:
  	  		T0_pred[T0_idx] = WN;
			break;
  	      case ST:
  	  	    T0_pred[T0_idx] = WT;
		    break;
  	      default:
  	        printf("Warning: undefined state of entry in table T0 during training");
		    break;
  	      }
  		break;
  	    }
      // T1, T2, T3, T4 have signed 3 bit counters
  	  case 1:
  	    update_pred(outcome, T1_pred[T1_idx]); //these are signed 3 bit counters
  		break;
  	  case 2:
  	    update_pred(outcome, T2_pred[T2_idx]); //these are signed 3 bit counters
  		break;
  	  case 3:
  	    update_pred(outcome, T3_pred[T3_idx]); //these are signed 3 bit counters
  		break;
  	  case 4:
  	    update_pred(outcome, T4_pred[T4_idx]); //these are signed 3 bit counters
  		break;
  	  default:
  	    printf("Warning: Undefined state of provider in TAGE !\n");
  	    break; 
  	}

    //if the provider was NOT the component with the longest history (i.e. T4 in our case),
    if (provider != 4)
    {
    	//allocate a new entry with a longer history
	    //pick Tj or Tk randomly, with Tj having twice the probability of Tk, j<k
		int allocation;
		int random_value; 

		switch(provider)
		{
			case 3:
				{
					allocation = 4;
					dbg_t4_allocated++;
					break;
				}
			case 2:
				{
					random_value = std::rand() % 4;
					if(random_value<2)
						{
						allocation = 3;
						dbg_t3_allocated++;
						}
					else
						{
						allocation = 4;
						dbg_t4_allocated++;
						}
					break;
				}
			case 1:
				{
					random_value = std::rand() % 7;
					if(random_value<4)
						{
						allocation = 4;
						dbg_t4_allocated++;
						}
					else if(random_value<6)
						{
						allocation = 3;
						dbg_t3_allocated++;
						}
					else
						{
						allocation = 1;
						dbg_t1_allocated++;
						}
					break;
				}
			case 0:
				{
					random_value = std::rand() % 15;
					if(random_value<8)
						{
						allocation = 4;
						dbg_t4_allocated++;
						}
					else if (8<=random_value & random_value<12)
						{
						allocation = 2;
						dbg_t2_allocated++;
						}
					else if (12<=random_value & random_value<14)
						{
						allocation = 3;
						dbg_t3_allocated++;
						}
					else if (random_value==14)
						{
						allocation = 1;
						dbg_t1_allocated++;
						}
				}
		}
				
	
		//initialize the newly allocated entry
		//FIXME: Add tag bits here to the entry
		if(allocation == 1)
		{
			T1_valid[T1_idx] = 1;
			//T1_tag[T1_idx] = (pc >> (T1_entries - 1)) & tag_mask;
			T1_tag[T1_idx] = pc & tag_mask;
			T1_u[T1_idx] = SNU;
			//prediction counter set to weak correct
			T1_pred[T1_idx] = (outcome == TAKEN) ? 0 : -1;  
		}
		else if(allocation == 2)
		{
			T2_valid[T2_idx] = 1;
			//T2_tag[T2_idx] = (pc >> (T2_entries - 1)) & tag_mask;
			T2_tag[T2_idx] = pc & tag_mask;
			T2_u[T2_idx] = SNU;
			//prediction counter set to weak correct
			T2_pred[T2_idx] = (outcome == TAKEN) ? 0 : -1;  
		}
		else if(allocation == 3)
		{
			T3_valid[T3_idx] = 1;
			//T3_tag[T3_idx] = (pc >> (T3_entries - 1)) & tag_mask;
			T3_tag[T3_idx] = pc & tag_mask;
			T3_u[T3_idx] = SNU;
			//prediction counter set to weak correct
			T3_pred[T3_idx] = (outcome == TAKEN) ? 0 : -1;  
		}
		else if(allocation == 4)
		{
			T4_valid[T4_idx] = 1;
			//T4_tag[T4_idx] = (pc >> (T4_entries - 1)) & tag_mask;
			T4_tag[T4_idx] = pc & tag_mask;
			T4_u[T4_idx] = SNU;
			//prediction counter set to weak correct
			T4_pred[T4_idx] = (outcome == TAKEN) ? 0 : -1;  
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


//------------------------------------//
//        Predictor Execution         //
//------------------------------------//


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
	init_tage();
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
    return tage_predict(pc);
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
      return train_tage(pc, outcome);
    case CUSTOM:
      return;
    default:
      break;
    }
  }
}
