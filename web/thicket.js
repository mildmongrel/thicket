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
const assert   = require('assert');

// load configuration
var config     = require('./config');

// VALIDATE CONFIGURATION
// =============================================================================
assert(semver.valid(config.client_update.latest_version),
       '** invalid config: client version **');
assert(semver.valid(config.mtgjson_update.latest_version),
       '** invalid config: mtgjson version **');

// ROUTES FOR OUR API
// =============================================================================
var router = express.Router();              // get an instance of the express Router

// Check for updates to client application
//   Input:  client version
//   Output: version (null if update not applicable)
//           site URL (not a direct DL link) (if update applicable)
// ----------------------------------------------------

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
        else if(semver.gte(client_version, config.client_update.latest_version))
        {
            console.log('  OK (no update necessary)');
            res.json({ version: null });
        }
        else
        {   
            console.log('  OK (update available)');
            res.json({ version:  config.client_update.latest_version,
                       site_url: config.client_update.site_url });
        }
    });

// Check for updates to mtgjson data
//   Input:  client version
//   Output: latest compatible allsets version
//           direct DL link to allsets data
// ----------------------------------------------------

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
        else if(semver.valid(client_allsets_version) && semver.gte(client_allsets_version, config.mtgjson_update.latest_version))
        {
            console.log('  OK (no update necessary)');
            res.json({ version: null });
        }
        else
        {
            console.log('  OK (update available)');
            res.json({ version:      config.mtgjson_update.latest_version,
                       download_url: config.mtgjson_update.download_url });
        }
    });

// REGISTER OUR ROUTES -------------------------------
// all of our routes will be prefixed with /api
app.use('/api', router);

// Redirect download link
// ----------------------------------------------------
app.use('/redirect/downloads', function (req, res, next) {
    res.redirect(config.redirects.releases_url);
});

// SETUP STATIC FILE SERVING -------------------------
app.use('/mtgjson', function (req, res, next) {
    console.log('[' + req.ip + '] static mtgjson request: ' + req.url);
    next();
});
app.use('/mtgjson', express.static('www/mtgjson'));

// START THE SERVER
// =============================================================================
app.listen(config.port);
console.log('Thicket web services started on port ' + config.port);

