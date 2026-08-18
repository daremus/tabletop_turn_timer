This is a single duration turn timer for tabletop games. It's like an hourglass without the slow reset.

The code is 99% the work of Gemini.

<img src="timer_with_enclosure.jpg" width="300">

## Basic Usage

Everyone has the same time for each turn. The timer resets to that time after every turn.

1. Turn on
2. Select a duration for turns
3. when the first player is ready to play, press the button
5. if the player presses the button before their time is up, the next player's time begins
6. if the player does not press the button before their time is up, a buzzer sounds and the timer sleeps
7. if the timer is sleeping, press the button to start the next turn
8. double press the button to pause, tap again to start the next turn
9. LEDs indicate the remaining duration
  Initial logic:

  |pin|original color|min time remaining|max time remaining|
  |---|---|---|---|
  |2|blue|5/6*duration|1*duration|
  |3|green|4/6*duration|5/6*duration|
  |4|yellow|3/6*duration|4/6*duration|
  |5|orange|2/6*duration|3/6*duration|
  |6|red|1/6*duration|2/6*duration|
  |6|red (flashing)|0|1/6*duration|

## Details
* Wiring schematic: ![](timer_schematic.png)
* The TS jacks are for external buttons
  
## Notes
* nice button: https://www.sparkfun.com/16mm-metal-push-button-switch-mushroom-head-red.html
* 
