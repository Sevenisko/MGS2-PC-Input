# MGS2: Substance - Improved PC input

The goal of this mod is to make MGS2: Substance much more playable on PC (Keyboard-Mouse setup) **without any 3rd-Party software or drivers**.



This mod is based on [Revised input driver for Metal Gear Solid 2](https://github.com/UltimateNova1203/mgs2_input) by [UltimateNova1203](https://github.com/UltimateNova1203).

## How to install?

Simply, download the release package, pick the ASI file and put it into *\<gameDirectory>/bin/scripts*, and also put SDL2.dll in *\<gameDirectory>/bin*

**Note**: You must have an ASI loader installed (the best way is to use V's fix, which automatically installs it)

## How to build?

Also simple, you must have Visual Studio 2022 in order to build it.

- Clone the repository

- Open the solution file in VS 2022

- Build the solution

## Any issues?

If you encounter any issues, please let me know via Issues tab.

### Known issues

- Mouse control is not way too accurate
  
  - That's because I'm remapping the mouse motion into a stick movement (the game has that as a single byte) - maybe in the future I'll try to reverse the input system and try to replace it completely
