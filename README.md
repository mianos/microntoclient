# Micro NTP Client
This code implements a mS accurate local NTP clock.
Unlike the usual sntp libraries, this code uses the normal UDP protocol and uses a slow phase lock loop with a dynamic single bit delta offset system.
