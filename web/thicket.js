//
// thicket.js
//
// Thicket Web Services node.js script.
//
// Run with '--local' to issue download URLs using localhost.
//

// BASE SETUP
// =============================================================================

// call the packages we need
var express    = require('express');        // call express
var app        = express();                 // define our app using express
var semver     = require('semver');

// DEFAULTS
// ----------------------------------------------------
const DEFAULT_SERVER = 'thicketdraft.net';

// CONSTANTS
// ----------------------------------------------------
const SERVER = (process.argv[2] === "--local") ? 'localhost' : DEFAULT_SERVER;
const PORT = process.env.PORT || 53332;        // set our port

// ROUTES FOR OUR API
// =============================================================================
var router = express.Router();              // get an instance of the express Router

// Check for updates to client application
//   Input:  client version
//   Output: version (null if update not applicable)
//           site URL (not a direct DL link) (if update applicable)
// ----------------------------------------------------
const LATEST_CLIENT_VERSION          = "v0.17.0-beta";
const LATEST_CLIENT_VERSION_SITE_URL = "http://github.com/mildmongrel/thicket/releases/tag/v0.17.0-beta"

// on routes that end in /update/client/:client_version
router.route('/update/client/:client_version')

    // (accessed at GET http://<host>:<port>/api/update/client/:client_version)
    .get(function(req, res)
    {
        const client_version = req.params.client_version;
        console.log('[' + req.ip + '] client update check from version ' + client_version);
        if(!semver.valid(client_version))
        {
            console.log('  ERROR (invalid client version)');
            res.json(null);
        }
        else if(semver.gte(client_version, LATEST_CLIENT_VERSION))
        {
            console.log('  OK (no update necessary)');
            res.json({ version: null });
        }
        else
        {   
            console.log('  OK (update available)');
            res.json({ version:  LATEST_CLIENT_VERSION,
                       site_url: LATEST_CLIENT_VERSION_SITE_URL });
        }
    });

// Check for updates to mtgjson data
//   Input:  client version
//   Output: latest compatible allsets version
//           direct DL link to allsets data
// ----------------------------------------------------
const LATEST_MTGJSON_ALLSETS_VERSION        = "3.3.16";
const LATEST_MTGJSON_ALLSETS_VERSION_DL_URL = "http://" + SERVER + ":" + PORT + "/mtgjson/AllSets-3.3.16.json"
// on routes that end in /update/mtgjson/:client_version
router.route('/update/mtgjson/:client_version')

    // (accessed at GET http://<host>:<port>/api/update/mtgjson/:client_version)
    .get(function(req, res)
    {
        const client_version = req.params.client_version;
        const client_allsets_version = req.query['allsetsversion'];
        console.log('[' + req.ip + '] mtgjson update check from version ' +
                client_version + ' with allsets version ' + client_allsets_version );
        if(!semver.valid(client_version))
        {
            console.log('  ERROR (invalid client version)');
            res.json(null);
        }
        else if(semver.valid(client_allsets_version) && semver.eq(client_allsets_version, LATEST_MTGJSON_ALLSETS_VERSION))
        {
            console.log('  OK (no update necessary)');
            res.json({ version: null });
        }
        else
        {
            console.log('  OK (update available)');
            res.json({ version:      LATEST_MTGJSON_ALLSETS_VERSION,
                       download_url: LATEST_MTGJSON_ALLSETS_VERSION_DL_URL });
        }
    });

// REGISTER OUR ROUTES -------------------------------
// all of our routes will be prefixed with /api
app.use('/api', router);

// Redirect download link
// ----------------------------------------------------
app.use('/redirect/downloads', function (req, res, next) {
    res.redirect("http://github.com/mildmongrel/thicket/releases");
});

// SETUP STATIC FILE SERVING -------------------------
app.use('/mtgjson', function (req, res, next) {
    console.log('[' + req.ip + '] static mtgjson request: ' + req.url);
    next();
});
app.use('/mtgjson', express.static('www/mtgjson'));

// START THE SERVER
// =============================================================================
app.listen(PORT);
console.log('Thicket web services started on port ' + PORT);
