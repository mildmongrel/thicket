Release Notes
=============

Release 0.21-beta:
------------------

Added "toast" indications for auto-selections and draft stage changes.

Added beep-on-new pack setting.  (Note: may not work on Linux due to a Qt limitation.)

Changed look and feel of player status in the ticker to be consistent with selection view.

Client cube-centric improvements:
  - Updated cube decklist parsing to allow card names without quantities.
  - Made client card lookups case-insensitive.

Added client settings dialog.  Includes configuration for:
  - Beep on new pack.
  - Basic land multiverse IDs - set to custom values or select from various presets.
  - Image cache maximum size.

Other client fixes:
  - Addressed button focus in room creation dialog.
  - Connect button in connect dialog not enabled while username is empty.
  - Fixed bug where draft indicators had remnants of previous room when entering a new room.

Internal:
  - Network protocol enhancements to allow for longer messages

Release 0.20-beta:
------------------

Protocol updated to 3.0.  (Clients and servers must be on the same
major protocol revision to communicate.)

Cube support!  Import a cube list in either .dec or .mwDeck format while creating a room.

To assist cube support, cards with no set code are searched in the client in reverse chronological set order to find a match.

Improvements to node.js web services script.

Fixed handling of player disconnected/departed state in ticker displays.

Other changes:
  - Building now requires g++4.9 for proper std::regex support.
  - Fixed some compiler warnings related to moving to g++4.9.


Release 0.19-beta:
------------------

New Features:
  - Added card preselection to avoid random selection on timer expiry.  Available via context menu on card or mouseover icon button when hovering over a card.

UI Improvements:
  - On startup, the initial view is set to the server tab; no draft tab is visible.
  - Removed the room tab; its functionality has been absorbed into the draft tab.
  - The draft tab is added when the room is joined.
  - Draft tab is now named by room name instead of 'Draft'.
  - A compactible sidebar has been added to the draft tab to allow player chat.
  - Added a floating indicator to the draft tab for packs queued and time remaining.
  - Land adjustments have been relocated and made retractable to conserve space.
  - A non-modal, non-dismissable 'Ready' dialog is presented while players are readying for draft.
  - The ticker shows player ready status while readying for draft.
  - The ticker shows deck hashes at the conclusion of the draft.

Other client fixes/improvements:
  - Splitter state is saved between sessions.
  - Improved saving and loading of window size/position.
  - Made connect and room creation dialogs children of the client window to fix their modality.

Server now sends all hashes to a client on rejoining if the draft is complete.

Modified room creation (client and server) to allow appropriate settings for draft types:
  - Booster draft: 2-8 players, 0-7 bots
  - Sealed draft: 1-8 players (bot options removed)

Miscellaneous general code cleanup.

Release 0.18-beta:
------------------

Client checks for application updates via web service.  Removed/obsoleted
now-unused method of client checking via the Thicket server.

Client checks for MTG JSON "AllSets" data updates via web service and
downloads from web server.  Under the covers this is channelized so
that 'stable' channel updates (the default) will use the thicketdraft.net
update method and web server.  The 'mtgjson' channel is able to update
direct from the untested mtgjson.com source.

Reorganized client menus.

Added simple node.js backend code to support web services and MTG JSON
file serving.

Fixed server bug with rotating log naming.

Added proper return error codes to server.

Release 0.17-beta:
------------------

Protocol updated to 2.0.  (Clients and servers must be on the same
major protocol revision to communicate.)

Sealed deck support!

Windows NSIS installer!

Added "Auto" zone for cards that are automatically assigned by server
(e.g. sealed deck and booster draft auto-picks when time expires).

Added client menu option to analyze deck on deckstats.net.

Client ticker modified to show round numbers between rounds.  Ticker no
longer shows auto-selected card updates.

Client image caching limited to 50MB.  This can be modified using the
"imagecache/maxsize" setting.

Improved client/server protocol mismatch handling.

Other client changes:
- Added memory of servers to connect dialog
- Added hide-when-empty behavior to Auto and Draft tabs
- Added tooltips to Auto and Draft tabs
- Added room type column to server view

Other changes:
- Sweeping "under the hood" changes to protocol and draft
  configuration, including modification and addition of many unit
  tests

Moved to "beta" status.  Thanks to the alpha testers!!

Release 0.16-alpha:
-------------------

Server sends user login/logout updates to clients, client shows current
users in server window.

Client MTGJSON AllSets update dialog now uses a combobox to display
builtin URL's in addition to others used to successfully download a valid
file.

Consolidated hostname and port in client connect dialog.

Updated client quit behavior to confirm exit when there are unsaved
changes.

Added a "Quit" menu item.

Fix #1: Clear server chat messages in client on disconnect

Other changes:
- Added unit test to create every card in every set


Release 0.15-alpha:
-------------------

Added an area to the client's server view to show server announcements.

Server alerts now bring up a modeless dialog on client.

Client saves and restores window size and position, as well as settings
for each pane (zoom, categoriztion, sort).

Other client changes:
- Rearranged menus
- Improved download progress feedback for MTG JSON file
- Updated about and compatibility dialog
- Limit chat histories to 1000 messages

Release 0.14-alpha:
-------------------

Introduced client dialog for downloading AllSets.json card data file.

Modified client default setting for server name to 'thicketdraft.net'.

Changed client save dialog to not use native widget.
  - On Windows the native dialog would interfere with the program event
    loop, resulting in keepalives not being sent, resulting in server
    disconnections.

Release 0.13-alpha:
-------------------

Fix for being unable to draft split cards.

Added server connection monitoring with client keepalive messages.
  - Previously, if a client vanished, the server would not know about it
    and keep its client connection open.

Other changes:
- Removed extraneous pack generation in server
- Improved unit test coverage for cardpools and mtgjson
- Code reorganization
- Client state machine refactoring

Release 0.12-alpha:
-------------------

First public release.
