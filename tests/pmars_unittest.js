var fs = require('fs');
var util = require('util');

var unittest = require('unittest');
var pmars = require('pmars');
var c = require('corewar');


var PmarsTestCase = function () { 
  unittest.TestCase.apply(this, arguments);
};
PmarsTestCase.prototype = new unittest.TestCase();

PmarsTestCase.prototype.testAssemble = function () {
  var validate = __dirname + '/../third_party/pmars-0.9.2/warriors/validate.red';
  var assembler = new pmars.Assembler();
  var parsed = assembler.assemble(0, validate);
  var vWar = new c.Warrior('vleft', parsed.offset, parsed.instructions);
  var vWar2 = new c.Warrior('vright', parsed.offset, parsed.instructions);
  var core = new c.Core({coresize: 1000});
  core.initialize();
  core.loadWarriors([vWar, vWar2]);
  for (var i = 0; i < 10; i++) {
    core.runOnce(true);
  }
};

PmarsTestCase.prototype.testAssembleAll = function () {
  var dir = __dirname + '/../warriors';
  var err, files = fs.readdirSync(dir);
  var i = 0;
  var rss = process.memoryUsage()['rss'];
  var vsize = process.memoryUsage()['vsize'];
  var assembler = new pmars.Assembler();
  for (var k in files) {
    //util.debug('assembling ' + c + '  ' + files[k]);
    //util.debug('mem ' + ((process.memoryUsage()['rss'] - rss) / (1024*1024)));
    var parsed = assembler.assemble(0, dir + '/' + files[k]);
    if (parsed) {
      c.parseSimple(parsed.instructions);
    }
    k++;
  }
};


exports.PmarsTestCase = PmarsTestCase;
