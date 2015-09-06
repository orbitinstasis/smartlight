## Smart Baby Light to Promote Comfort at Bed Time
> This is a project designed and developed by Ben Kazemi in the summer of 2014 initially on an Arduino. 

> At it's highest form of abstraction, it is a slow transforming, dimmable set of two narrow wave lengths in visible light, aptly named by its two user defined settings: 'hot', and 'cold' that transform depending on the childs sleep state, determined by the amount of noise emitted. 

> The sleepier the child (not cried or complained), the slower, and dimmer the transformations; the transformation increases in lux and frequency of transformations per unit time if the child has discomfort or fear of the dark, and is purposely designed to quickly grasp the childs attention. 

> The 'lava' setting is designed to indefintely vary transformations in terms of frequency and lux.

> Future updates will include an android application to monitor the audio presence for ease-of-mind, and to remotely modify system settings, or to control the state of the smart light, all through bluetooth. 

> Future idea is to replaced the simple solid colour LED sources (3 seperate colours per setting [hot or cold]), with two or more laser sources that have their respective light rays split into many points and reflected on a ceiling or wall, with each set of points varying in lux, and potentially moving very slowly, to give the perception of moving stars. 

The smart light is eco friendly, and is currently lit by a 2 by 5 array of LEDs, of which only one is ever lit at any time. Audio input is through an op-amp circuit connected to by a sensitive electret microphone. 

Code complexity and lux could be drastically reduced, and increased respectively, should a single RGB (or similar) bright LED be implemented. 


## Dependencies
- Arduino IDE 


## Build
Compile and upload from within the arduino IDE to an arduino with appropriately connected components. 


## TODO List:
- [ ] Convert the code to a C project file for AVR 
- [ ] Complete the implementation of the op amp for audio input 
- [ ] Add sleep states for power conservation for different system states 
- [ ] Implement Android application to control the system remotely 
- [ ] Send audio via bluetooth to android 


## License 

This code is available under the GNU V2 license. 
