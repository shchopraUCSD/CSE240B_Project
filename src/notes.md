# Notes

## Approximations

- hardcode number of components to 5 - one base predictor and 4 tagged predictors
- hardcode the geometric series to 2,4,8,16
- have tables T1,...,T4 for all possible indexes for now - note that we are XORing PC with ghistory[Li-1:0] so the tables aren't "infinite". This means that there are no evictions 

## Observations
- for r=4 series, TAGE performs horribly - 40-50% misprediction rate
- for r=2 series, TAGE isn't that bad. Misprediction rate is roughly twice of GSHARE in shorter traces
- printed out which tables end up being the provider more often, and which end up being allocated on mispredictions
- adding 2 tag bits reduced misprediction rate
- removing the valid bit had no impact on the misprediction rate after tag bits are added
- a 2-bit counter performs on average the same as a 3-bit counter as a predictor

## Ideas

- the TAGE paper mentioned performance on back-2-back traces. Maybe the traces just aren't long enough? ans. no, with the concatenated trace, gshare ends up performing better :-( 
- interpretation of "tag" : Shaurya understood "tag" as index, while Arjit understood tag in addition to index
- noticed that T4 is very underutilized even when we jack up the probability. This suggests aliasing. Adding unused higher order PC bits as a separate tag ought to help with this? ans. misprediction dropped from 13.5% to 13.15%
- used just the lower 2 bits of PC as tag and see if this helps? ans. misprediction further dropped to 9.421%
- is a 3-bit counter required for T1, T2, T3, T4? what happens if you use da 2-bit counter? and. apparently nothing


## GSHARE vs TAGE
### GSHARE
- global history length 17. 
- storage = 2^17 entries X 2 bits/entry
#### TAGE
- ignore usefulness bits for size calculation. our TAGE with approximations doesn't use it yet
- with valid bit, 2-bit counter for T0 and 3-bit signed counters for T1, T2, T3, T4:
	- storage = 2 X 2^L0 + (3+1) X (2^L1 + 2^L2 + 2^L3 + 2^L4)
	- log(storage) = log(2*1024 + 4*65812) = 18.02
- with 2-bit tag, 2-bit counter for T0 and 3-bit signed counters for T1, T2, T3, T4:
	- storage = 2 X 2^L0 + (2+2) X (2^L1 + 2^L2 + 2^L3 + 2^L4)
	- log(storage) = log(2*1024 + 5*65812) = 18.34
- with 2-bit tag, 2-bit counter for T0 and 2-bit signed counters for T1, T2, T3, T4:
	- storage = 2 X 2^L0 + (2+2) X (2^L1 + 2^L2 + 2^L3 + 2^L4)
	- log(storage) = log(2*1024 + 4*65812) = 18.02
- with 2-bit counter for T0 and 3-bit signed counters for T1, T2, T3, T4:
	- 2 X 2^L0 + 3 X (2^L1 + 2^L2 + 2^L3 + 2^L4)
	- for r=2 series, log(storage) is 17.6, so storage sizes of GSHARE and TAGE are comparable here  

