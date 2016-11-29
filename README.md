# Thicket
Thicket is cross-platform software for collectible card game drafting.

![Screenshot](https://i.imgbox.com/90lyZIf1.png)
_Screenshot of the Thicket client, version 0.19.0-beta (card images obscured intentionally)_

To get started, download the [latest release](http://github.com/mildmongrel/thicket/releases/latest) of the client.

Visit the [project wiki](http://github.com/mildmongrel/thicket/wiki) for more project information.

## Features

- Booster draft and sealed deck support
- Dual-pane interface for drafting and building a deck
- Zoom, sort, and categorize as you draft and build
- Send deck to deckstats.net for analysis, sample draws, etc.
- Save drafted decks in standard .dec format
- Fairness enforcement:
  - Client/server architecture; server maintains the state of drafts and players, clients receive only the information they are allowed to know
  - Calculation of [Cockatrice](http://cockatrice.github.io) hashes for each player's drafted deck

## Bugs and Enhancements

Please report bugs or suggest enhancements via the [issue tracker](http://github.com/mildmongrel/thicket/issues).

## Contribution

If you're considering patching or contributing to Thicket in some manner, please discuss your proposal in an issue first or with the maintainer.

- Repository policy: No unsolicited pull requests!

## Credits
Thicket stands on the shoulders of many folks' wonderful efforts.

### Software

- [Qt](http://www.qt.io)
  - Application framework used for both server and client
- [Google Protocol Buffers](https://developers.google.com/protocol-buffers/)
  - Messaging library
- [RapidJSON](http://rapidjson.org/)
  - Fast JSON parsing/generation for C++
- [spdlog](https://github.com/gabime/spdlog)
  - Super fast C++ logging library
- [catch](https://github.com/philsquared/Catch)
  - C++-native, header-only framework for unit-tests
- [cpp-lru-cache](https://github.com/lamerman/cpp-lru-cache)
  - Simple and reliable LRU (Least Recently Used) cache for C++

### Other

- Card information provided by [MTG JSON](http://mtgjson.com/).
- Several icons courtesy of [game-icons.net](http://game-icons.net/faq.html). License: [CC-BY-3.0](http://creativecommons.org/licenses/by/3.0/)

