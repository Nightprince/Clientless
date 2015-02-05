Clientless
=============

Clientless is a World of Warcraft client emulator for 4.3.4 15595 Cataclysm based on TrinityCore.

[![Build Status](https://drone.io/github.com/Dehravor/Clientless/status.png)](https://drone.io/github.com/Dehravor/Clientless/latest)

## Usage

After starting the client for the first time you'll need to provide the following details.

+ The realmlist of your server (e.g hu.logon.tauri.hu)
+ Your account name and password
+ The name of the realm you are trying to connect (e.g [HU] Tauri WoW Server)
+ The name of your character

These values can be saved (settings.ini) so you'll be able to login automatically when you start the program again.

## How to customize

Custom packet handlers can be added easily.

+ First, you need to declare your function's prototype in *WorldSession.h*

```
    HandleCharacterEnum(WorldPacket &recvPacket));
```

+ Then you have to associate the desired opcode with your handler function in *WorldSession.cpp*

```
    { SMSG_CHAR_ENUM, &WorldSession::HandleCharacterEnum };
```

+ Finally, you need to define your handler somewhere. You may create your own .cpp files or use *MiscHandler.cpp* in World/Handlers directory.

```
    void WorldSession::HandleCharacterEnum(WorldPacket &recvPacket)
    {
        ...
    }
```

Packet sending should be straightforward if you are used to TrinityCore's structure.

Requirements
-------

 + CMake ≥ 2.6
 + OpenSSL ≥ 0.9.7
 + C++11 Compiler
 + Windows/Linux