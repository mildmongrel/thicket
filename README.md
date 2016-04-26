# Thicket
Thicket is cross-platform software for card game drafting.

Thicket uses a client/server architecture to guarantee fairness; the server maintains the state of drafts and clients while clients receive only the information they are allowed to know.

Visit the [project wiki](http://github.com/mildmongrel/thicket/wiki) for more project information.

---

# Credits
Thicket stands on the shoulders of many folks' wonderful efforts.

## Software

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

## Other

- Card information provided by [MTG JSON](http://mtgjson.com/).
- Icon courtesy of [game-icons.net](http://game-icons.net/faq.html). License: [CC-BY-3.0](http://creativecommons.org/licenses/by/3.0/)
