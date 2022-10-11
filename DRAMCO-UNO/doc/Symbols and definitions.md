# Multihop symbols and definitions

| New							    | Old								| Description																| 
| ----------------------------------| --------------------------------- | ------------------------------------------------------------------------- |
| PREAMBLE_DURATION				    | PREAMBLE_DURATION					| Time in ms for how long the preamble is									| 
|									|									| 																			| 
| CAD_STABILIZE						| CAD_STABILIZE               		| Timedelay before executing CAD, after deep sleep when power down: to stabilize power supply | 
| CAD_DELAY		     				| CAD_DELAY_MIN             		| Nominal delay between CADs, defined by PREAMBLE_DURATION					| 
| CAD_DELAY_RANDOM					| CAD_DELAY_MAX						| Random window around CAD delay, defined by PREAMBLE_DURATION				| 
|									| 									| 																			| 
| AGGREGATION_TIMER_MIN				| PRESET_MIN_LATENCY         		| Aggregation timer = timer to wait for incoming data (own data or rx) to append to message in queu. To imrove latency, a stepup/stepdown mechanism is put in place (depending on how much extra data is coming in): this is the min value of that. | 
| AGGREGATION_TIMER_MAX 			| PRESET_MAX_LATENCY             	| Max value of the aggregation timer.										| 
| AGGREGATION_TIMER_RANDOM			| PRESET_MAX_LATENCY_RAND_WINDOW   	| Size of the random window around the current value of the aggregation timer. | 
| AGGREGATION_TIMER_UPSTEP			| PRESET_LATENCY_UP_STEP          	| Added to the current value of the aggregation timer, when more latency is tolerated, or more packets are coming in (efficient appending/aggregating). | 
| AGGREGATION_TIMER_DOWNSTEP		| PRESET_LATENCY_DOWN_STEP        	| Subtracted to the current value of the aggregation timer, when less latency is prefered, or almost no packets are coming in (added latency amounts to nothing: no use in waiting for packets if none are coming in). | 
|									| 									| 																			| 
| /									| FORWARD_BACKOFF_MIN				| Not in use 																| 
| /									| FORWARD_BACKOFF_MAX        		| Not in use 																| 
|									|									| 																			| 
| TX_DELAY_AFTER_CAD            	| TX_BACKOFF_MIN					| When pending tx (any packet), and a CAD is detected just before, postpone the message by this amount. This is the nominal value. Postpone at least 1 preamble. | 
| TX_DELAY_AFTER_CAD_RANDOM 		| TX_BACKOFF_MAX					| Random window around time delay after CAD									| 
|									|									| 	| 
| MAX_PRESET_BUFFER_SIZE 			| 									| 	| 
| MAX_FORWARD_BUFFER_SIZE 			| 									| 	| 