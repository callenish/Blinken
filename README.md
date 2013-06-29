# The Arduino 120 Volt Blinkenlights Project #
## A Repeat Cycle Timer ##

I had a problem. Hot water took forever to get to the taps in my house. So I got a circulating pump to make sure I always had hot water ready for me.

That led to problem two. When the pipes were put into the concrete floor of the basement they were not insulated. Over time, the hot water constantly flowing through the pipes was heating up the cold water which started arriving at the tops luke-warm.

My solution is this project. Since it can be used to control any 120V circuit and all my testing is with a lamp and fairly short cycle times, I've taken to calling it my [Blinkenlights](http://www.jargon.net/jargonfile/b/blinkenlights.html "Blinkenlights") project (not to be confused with [any other](http://blinkenlights.net/) Blinkenlights project). The hardware consists of:

- an Arduino
- a PowerSwitch Tail 2
- an optional DS1307 RTC kit from AdaFruit (if you want schedules)
- a spring-loaded toggle switch to change modes (you could use a push button)
- an optional battery power source if not using the USB port
- and associated male header pins, wires, and case to hold it all together

Program has three modes that can be changed using the toggle switch (or use a push button if you prefer):

1. **Repeat Mode**. Indicator LED is *off*. Always cycle on for the indicated constant time, then off for the indicated time.
1. **Scheduled Mode**. Indicator is *blinking*. Full support TBD. For now it is identical to the repeat mode save for the indicator. Will eventually use a schedule table to determine when the repeat cycles should be run. The tables will have day of week the schedule is for, time to start the schedule, time to stop the schedule, how long the on period is for, and how long the off period is for. If you wanted to use this to make it look like you are home for long periods of time by turning on and off lights, you might want to add some kind of random skew field as well.
1. **Running Mode**. Indicator is *on*. The 120V circuit is on all the time.

The PowerSwitch Tail 2 is a great device for keeping yourself away from the 120V lines as everything is already in a case and you have standard male and female three prong (in North America) plugs to put your project inline with a 120V device. It has an LED to indicate when the relay is active and it has its own pull-up resistor so you don't need one.

This code uses an internal pull-up resistor for the switch. If you'd prefer to use your own, it is easy to find the required place to change in *setup()*. I also use a regular ground pin for the switch but an input pin set to LOW for ground on the PST2. Again, change *setup()* if you want something different.

There are a few constants you can change and they are clustered at the top of the sketch. The most important ones are onPeriod and offPeriod until I get the scheduled mode working.

That's about all. The code is free and in the public domain for anyone to use for any purpose, so go ahead and do whatever you want with it.
