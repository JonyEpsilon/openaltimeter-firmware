openaltimeter firmware
=======================

Licence
=======

The openaltimeter firmware is licenced to you under the terms of the GPL v3. You can find full details of this licence in licence.txt.


Contributions
=============

The following people have contributed code to this project :)

Jony Hudson
Jan Steidl


Changelog
=========

beta6: Fix a bug that crashed the board when the log memory was full (thanks to Jan Steidl for the patch). Improve radio pulse detection code so that it should work with all brands of radio (thanks to lebenj for extensive testing).

beta5: Make LiPo cell detection algorithm simpler and more robust.

beta4: Added code to support the desktop download application. Change the serial rate to 115200 baud, improving download times by more than a factor of ten.

beta3: Enable low-voltage and lost-model alarms. Hysteresis for battery monitor to stop low-voltage alarm from repeatedly starting and stopping near threshold. Change beeper frequency which, despite the claims of the beeper datasheet, make the alarms _way_ louder. Make radio pulse measurement more robust. Minor bugfixes.

beta2: Very minor changes to make a usable build. Bugfixes.

beta1: Initial version. Working altimeter logging and launch height output. Code is mostly there for lost-model-and low-voltage- alarms, but not enabled.
