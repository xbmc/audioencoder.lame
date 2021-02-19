# LAME Audio Encoder add-on for Kodi

This is a [Kodi] (http://kodi.tv) LAME audio encoder add-on.

#### CI Testing
[![Build Status](https://travis-ci.com/xbmc/audioencoder.lame.svg?branch=master)](https://travis-ci.com/xbmc/audioencoder.lame)
[![Build Status](https://dev.azure.com/teamkodi/binary-addons/_apis/build/status/xbmc.audioencoder.lame?branchName=Leia)](https://dev.azure.com/teamkodi/binary-addons/_build/latest?definitionId=22&branchName=Leia)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

## Build instructions

When building the add-on you have to use the correct branch depending on which version of Kodi you're building against. 
For example, if you're building the `Jarvis` branch of Kodi you should checkout the `Jarvis` branch of this repository. 
Add-on releases are also tagged regularly.

### Linux

1. `git clone --branch Leia https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/audioencoder.lame/audioencoder.lame.git`
3. `cd audioencoder.lame && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=audioencoder.lame -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/project/cmake/addons`
5. `make`

The add-on files will be placed in `../../xbmc/addons` so if you build Kodi from source and run it directly 
the add-on will be available as a system add-on.

##### Useful links

* [Kodi's add-ons development support] (http://forum.kodi.tv/forumdisplay.php?fid=26)
