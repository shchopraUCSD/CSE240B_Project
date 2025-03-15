# Notes

## Approximations

- hardcode number of components to 5 - one base predictor and 4 tagged predictors
- hardcode the geometric series to 4,16,64,256
- have unlimited tables T1,...,T4 for now - note that we are XORing PC with ghistory[Li-1:0] so the tables aren't "infinite" 

## Observations
- for r=4 series, TAGE performs horribly - 40-50% misprediction rate
- for r=2 series, TAGE isn't that bad. Misprediction rate is roughly twice of GSHARE
- one idea: the TAGE paper mentioned performance on back-2-back traces. Maybe the traces just aren't long enough? ans. no, with the concatenated trace, gshare ends up performing better :-( 

## GSHARE vs TAGE
### GSHARE
- global history length 17. 
- storage = 2^17 entries X 2 bits/entry
#### TAGE
- ignore usefulness bits for size calculation. our TAGE with approximations doesn't use it yet
- 2 X 2^L0 + 3 X (2^L1 + 2^L2 + 2^L3 + 2^L4)
- for r=2 series, log(storage) is 17.6, so storage sizes of GSHARE and TAGE are comparable here  

