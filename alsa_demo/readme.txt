0. Decide which lib_4.* we need to use.
   a. Type "echo $PATH" and check the first toolchain path comes from "arm_linux_4.2" or "arm_linux_4.8".
   b. For arm_linux_4.2, type "/bin/cp -ar lib_4.2.1/* lib".
   c. For arm_linux_4.8, type "/bin/cp -ar lib_4.8.4/* lib".
1. ALSA demo code aplay/arecord & amixer come from http://alsa.opensrc.org/Alsa-utils.
2. amixer: A command line mixer
3. aplay/arecord: A utility for audio playback/record.
4. Before running it on your device, please copy&extract conf files alsa_share.tar.gz into /usr/share/ on your device.
