# whatsapp4cardputer
![GitHub Repo stars](https://img.shields.io/github/stars/bjarnepw/whatsapp4cardputer?style=for-the-badge)
![Forks](https://img.shields.io/github/forks/bjarnepw/whatsapp4cardputer?style=for-the-badge)
![Issues](https://img.shields.io/github/issues/bjarnepw/whatsapp4cardputer?style=for-the-badge)
![Release Status](https://img.shields.io/badge/RELEASE-NO_RELEASE_YET-red?style=for-the-badge)
![Beta Status](https://img.shields.io/badge/BETA_STATUS-COMPILABLE-green?style=for-the-badge)

## IDEA
The Name of this project literally tells you everything Important. I wanna build a custom Whatsapp Client for the Cardputer. Since the Cardputer just is an ESP 32 we need an extra Server for that.

## Goals
- [x] Make it work in the local Network 
- [ ] Improve UI and make it run stable
    - [ ] Chat view & Chat list view Function as they should
    - [ ] Handle Emojis?...
- [ ] Integrate some sort of port forwarding or sth like that so we can access whatsapp from anywhere (ngrok maybe)
    - [x] Added ngrok Password page
    - [ ] Working authentication   
- [ ] More features?? / Integrations ??


## How to run (if it works)

### Release

1. Download Release and Run the server
2. Flash the Firmware via [M5Burner](https://docs.m5stack.com/en/uiflow/m5burner/intro)

### Beta Release
1. Clone gepository 
2. Compile with PlatformIO 
3. Run Server in your Network with NodeJS [Server.js](./server/server.js)

### How to Clone a Repository???

[Install Git](https://git-scm.com/)
```sh
git clone https://github.com/bjarnepw/whatsapp4cardputer
```

### Server

#### Start Server 

Open a terminal in the [Server Folder](./server/). 
Then run 
```sh
node server.js
```

#### Set up Ngrok

Follow [Ngrok installing instructions](https://ngrok.com) 

Run 
```sh
ngrok http 3000 --basic-auth="yourUserName:yourPassword"
```

## THANKS TO
- [Bruce](https://github.com/pr3y/Bruce)
