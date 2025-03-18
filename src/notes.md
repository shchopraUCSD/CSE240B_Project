# Notes

## Approximations

- hardcode number of components to 5 - one base predictor and 4 tagged predictors
- hardcode the geometric series to 2,4,8,16
- have tables T1,...,T4 for all possible indexes for now - note that we are XORing PC with ghistory[Li-1:0] so the tables aren't "infinite". This means that there are no evictions 

## Observations
- for r=4 series, TAGE performs horribly - 40-50% misprediction rate
- for r=2 series, TAGE isn't that bad. Misprediction rate is roughly twice of GSHARE in shorter traces
- printed out which tables end up being the provider more often, and which end up being allocated on mispredictions

## Ideas

- the TAGE paper mentioned performance on back-2-back traces. Maybe the traces just aren't long enough? ans. no, with the concatenated trace, gshare ends up performing better :-( 
- interpretation of "tag" : Shaurya understood "tag" as index, while Arjit understood tag in addition to index
- noticed that T4 is very underutilized even when we jack up the probability. This suggests aliasing. Adding unused higher order PC bits as a separate tag ought to help with this? TODO Arjit to try this experiment


## GSHARE vs TAGE
### GSHARE
- global history length 17. 
- storage = 2^17 entries X 2 bits/entry
#### TAGE
- ignore usefulness bits for size calculation. our TAGE with approximations doesn't use it yet
- 2 X 2^L0 + 3 X (2^L1 + 2^L2 + 2^L3 + 2^L4)
- for r=2 series, log(storage) is 17.6, so storage sizes of GSHARE and TAGE are comparable here  

