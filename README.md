This is a single duration turn timer for tabletop games. It's like an hourglass without the slow reset.

Attribution:
* Code: 99% Gemini.
* Everything else: 99% me.

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
  
## Components
* Panel mount mushroom head buttons: [22mm](https://www.amazon.com/dp/B0CKV6B81P), [16mm (1)](https://www.sparkfun.com/16mm-metal-push-button-switch-mushroom-head-red.html), [16mm (2)](https://www.amazon.com/dp/B0FM3VK8YW)
* [Arduino Nano clone](https://www.amazon.com/dp/B0F6Y7GS4Q) / [Arduino Nano Every](
* [3.5mm mono jacks](https://www.amazon.com/dp/B0CF9DQYQ6)
* [Buzzer/mini speaker](https://www.sparkfun.com/mini-speaker-pc-mount-12mm-2-048khz.html)
* [Battery holder](https://www.sparkfun.com/battery-holder-3xaa-with-cover-and-switch-bare-wire.html) - switch removed.
* [LED holders](https://www.sparkfun.com/led-holder-5mm-chrome-finish.html)
* M2 x 4mm [heat set threaded inserts](https://www.amazon.com/dp/B0FD7DQS8Y)
* M2 x 5mm screws
* [Solderable breadboard](https://www.amazon.com/dp/B0B27XB69M)
