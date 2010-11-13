require.paths.unshift('./third_party/node-unittest/lib');
require.paths.unshift('./lib');
require.paths.unshift('./tests');

var unittest = require('unittest');

var core_unittest = require('core_unittest');
var pmars_unittest = require('pmars_unittest');

unittest.run([core_unittest, pmars_unittest]);
