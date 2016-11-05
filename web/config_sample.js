var config = {};
config.client_update = {};
config.mtgjson_update = {};
config.redirects = {};


// port to run the web services server
config.port = 53332;

// latest client version and link to a location to download it
config.client_update.latest_version = "v0.19.0-beta";
config.client_update.site_url       = "http://github.com/mildmongrel/thicket/releases/tag/v0.19.0-beta"

// latest mtgjson 'allsets.json' version and a download link to it
// NOTES:
//   - the download link is relative to the client, i.e. 'localhost' == client machine
//   - the download link can be named anything - the client remembers its downloaded
//     version based on the advertised 'latest version' field
config.mtgjson_update.latest_version = "3.6.0";
config.mtgjson_update.download_url   = "http://localhost/mtgjson/AllSets-3.6.json"

// redirect link to find client releases
config.redirects.releases_url = "http://github.com/mildmongrel/thicket/releases";


module.exports = config;
