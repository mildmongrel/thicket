Release Notes
=============

Release 0.14-alpha:
-------------------

Introduced client dialog for downloading AllSets.json card data file.

Modified client default setting for server name to 'thicketdraft.net'.

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
