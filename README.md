# MakeUofT

For more details, see https://devpost.com/software/invisible-piano

## Inspiration

We're a team of music lovers: Two of us play the piano, and the other one plays the flute. For us piano players, we've always had to worry about finding an instrument to practice when we travel or move to a new place. That's when we thought if we might build a small portable piano-like instrument that would be both convenient to carry around and fun to play on. In addition, by making the instrument able to span multiple octaves, we can play a variety of simple melodies and test out those perfect pitches that many of us music lovers have (or claim to have).

## What it does

The instrument is played by placing an object in front of it at various distances. The pitch it produces varies with the distance the object is placed away from its detecting core. This variation spans a full octave, from a C note to the upper B note. The instrument has two modes: A keyboard mode (discrete mode) and a "trombone" mode (continuous mode). In the keyboard mode, the variation is discrete and two consecutive notes are separated by a semitone (C -> C#/Db -> D -> D#/Eb -> E and so on); in the trombone mode, the variation is continuous (or as continuous as it can be with a digital system), meaning that it can practically play any pitch within the octave range, making it able to produce that signature sliding sound that a trombone makes. A simple pushbutton is used to switch between the two modes.

Additionally, a touch pad accompanies the instrument to allow for an expanded range: Pressing the touch pad with varying amount of force can increase the octave of the primary pitch by one, or a maximum of two octaves. This means the instrument covers a full range of three octaves where any pitch is accessible with a combination of mode choice, playing the instrument with one hand, while pressing the touch pad with another. 

## How we built it

### Pitch Calculation

The pitch separation is achieved with a distance-measuring HC-SR04 ultrasonic sensor. Based on the distance the sensor returns and the choice of mode, the software makes a conversion from the measured distance (in mm) to a specific pitch frequency (in Hz). The mapping from distance to pitch is different for the two modes: In the keyboard mode, the full supported distance range is evenly split into twelve buckets, and each bucket maps directly to one of twelve distinct semitones within an octave, hence any distance that falls within the same bucket produces the same pitch; In the trombone mode, a direct linear conversion is made from the full range of supported distances to the full range of supported pitches (the range of an octave) with the formula
```
output_freq = (input_dist - min_dist) / (max_dist - min_dist) * (max_freq - min_freq) + min_freq
```
, hence any two different distances produce different pitches.

The octave control is achieved by using a force sensor. The force sensor is programmed to return 3 values based on the amount of pressure it detects: If no pressure is felt, it returns 0; It returns 1 or 2 with a small amount (light touch) vs a large amount of pressure (heavy touch).  

The base frequency returned by the pitch calculation is then adjusted based on the input from the force sensor module. Based on the property that any two notes separated by an octave have frequencies that are exactly two-to-one (the high note's frequency is double that of the low note's), we simply need to multiply the base frequency by a factor of two (2 to the exponent of the output of the force sensor module which is either 0, 1, or 2) to produce the same note at a higher octave.

### Sound Emission

The Arduino SDK does not provide the `tune()` and `noTone()` functions for our Arduino Due, which are vital for producing sound of constant pitches. To work around this limitation, we implemented the sound-emission functionality from the ground-up. We set and handled interrupts from the timer-counter peripherals of the microcontroller, and outputs dynamic waveforms to the DAV output (which is an analog pin). More interestingly, we are able to create a superposition of sounds of multiple pitches (i.e. harmony sounds) by running multiple timer-counts and updating the waveform in parallel. We decided to produce only single-pitch sounds in the end because the DAV output was not very precise and the quality of harmonic sounds we produced was not very satisfactory. Whatever we created in the end, we have learned a lot about the performance, capabilities, and limitations of microcontrollers and speakers.

## Challenges we ran into

1. We initially intended to use the LCD RGB Backlight display that comes with the Due Kit. However, it didn't work, and although mentors tried to help, we didn't have any success.
2. We spent a long time figuring out how to enable and handle the timer-counter interrupts correctly. In particular, how to manipulate the 3 registers (RA, RB, and RC) of a timer-counter is poorly documented and many example code we found online were wrong. It was not hours after our initial attempt until we realize that the 3 registers are not equivalent: although all three registers may trigger timer-counter interrupts, only RC can cause the timer to reset. These details are not explicitly described by the documentation or tutorials, and we had to read the source code of Arduino carefully to find them out.
3. We also tried a lot of sound-producing options which later proved not to work. For example, we tried connecting PWM signals to a buzzer, but it produced very low-quality sounds. Also, we tried to produce nicer sounds by generating sine waves instead of squared waves. In reality however, creating a complex waveform by frequently updating the analog output caused very high-pitch noises that were very unpleasant. We also tried generic harmonic sounds by outputs an superposition of multiple square waves, but again that caused unpleasant high-pitch noises. In the end, we fell back to the most preservative sonic option: square waves of single pitches.

## Accomplishments that we're proud of

We're most proud of being able to program the speaker and implement the tone() routine to produce notes of various pitches, especially considering the limitations surrounding clock frequency and counter overflow described above.

## What we learned

- We practices the two major ways for a microcontroller to use its peripherals: data polling and interrupts. To be specific, we used polling for ultrasonic distance measuring and force measurement and interrupts for pitch control.
- We are more aware of the hardware limitations of microcontrollers. For example, the clock speed of a microcontroller are much slower than that of a computer, so if we query everything using data polling the delay we achieved became almost unacceptable.
- We also learned the mechanisms how many measuring tools work, like force sensors are essentially force-sensitive resistors.

## What's next for Invisible Piano

1. We intend to improve the sound quality of the Invisible Piano, or provide multiple predefined sound effects for users to explore.
2. We intend to display the musical notes on an LED display so that users could know what notes they are playing.
3. We intend to calibrate the Invisible Piano further. Currently, it's not very accurate due to errors in the distance measurement.
