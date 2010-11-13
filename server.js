require.paths.unshift('./third_party');
require.paths.unshift('./lib');

var fs = require('fs');
var http = require('http');
var url = require('url');
var util = require('util');

//var io = require('Socket.IO-node');

var pmars = require('pmars');
var corewar = require('corewar');


// Load our warriors
var warriors = {};
var warriorNames = [];
var dir = './warriors';
var err, files = fs.readdirSync(dir);
var assembler = new pmars.Assembler();

for (var k in files) {
  var parsed = assembler.assemble(0, dir + '/' + files[k]);
  if (parsed) {
    parsed.instructions = corewar.parseSimple(parsed.instructions);
    if (!parsed.name || parsed.name == 'Unknown') {
      parsed.name = files[k];
    }
    warriors[parsed.name] = parsed;
    warriorNames.push(parsed.name);
  }
}


/**
 * Web Service
 */
var listWarriors = function () { return warriorNames; };
var showWarrior = function (query) { return warriors[query.name]; };

var ROUTES = {
  '/list': listWarriors,
  '/show': showWarrior
}

var notFound = function (response) {
  response.writeHead(404, {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Headers': 'X-Requested-With',
  });
  response.end();
};

var server = http.createServer(function (request, response) {
  request.on('end', function () {
    var parsed = url.parse(request.url, true);
    var handler = ROUTES[parsed.pathname];
    if (!handler) {
      return notFound(response);
    }

    var rv = handler(parsed.query);
    if (!rv) {
      return notFound(response);
    }

    var out = JSON.stringify(rv);
    response.writeHead(200, {
      'Content-Length': out.length,
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Headers': 'X-Requested-With',
    });
    response.write(out);
    response.end();
  });
});
server.listen(8001);

util.print('Listening on port 8001');


