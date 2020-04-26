# trainer-blebridge
esp32 based BLE bridge for a non-bluetooth/ant+ enabled exercise bike.

Currently supporting a BodyWorx ABX450AT 
with each of the cables from the base unit broken out to an ESP32 board, 
and then continuing to the existing console.

Current implementation supports sending the RPM and Watts to e.g. Zwift.

Next step in the implementation to be able to replace the console entirely, and drive 
the bike by changing the resistance from within the Zwift app.

