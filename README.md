## Moogdots

This is the code for running multisensory self-motion experiments used in the Zaidel lab at Bar Ilan University, Israel, (as of December 2021).
Our code is adapted from code that originated in the Angelaki lab at Baylor College of Medicine.
It was adapted to work with the Moog motion base model MB-E-6DOF/12/1000, Windows 7, to present visual stimuli via Oculus Rift (CV1), and with new custom experimental protocols.
The code comprises two parts: 1) "Moogdots" (which controls the vestibular and visual self-motion stimuli) and 2) "Matlab" (which controls the experiment conditions and trials, and saves participant responses).
These two components are installed on two separate computers that communicate via DAQ cards (Measurement Computing).
The computer with Moogdots communicates with the motion base via UDP. The Oculus is also connected to the Moogdots computer.
A Cedrus response box is connected to the Matlab computer.
The code runs only on Windows 7.
Due to the highly complex nature of this code and unique experimental hardware setup with moving robotic parts, this code is not "plug and play" and not endorsed for generic use. Anyone wanting to develop a system with similar experiments would need to undergo adequate training beforehand. 
For further technical details contact the Zaidel lab at https://zaidel.wixsite.com/zaidel-lab
