Thicket Web Services
--------------------

Uses node.js to provide:
- Web API's for client updates and MTGJSON updates
- Static file serving for MTGJSON files
- Redirection link for client download landing page

Install Node (Ubuntu 16.04)
---------------------------

curl -sL https://deb.nodesource.com/setup_6.x | sudo -E bash -
apt-get install nodejs
sudo npm install npm --global

Usage
-----

Copy config_sample.js to config.js and edit as necessary

Install dependencies:
 npm install

Run server (on Ubuntu 14.04)
 nodejs thicket.js
