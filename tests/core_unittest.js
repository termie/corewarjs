
var unittest = require('unittest');
var c = require('corewar');


var WarriorTestImp1 = new c.Warrior(
    'testImp1', 0, [[c.MOV|c.I, [c.IMMEDIATE, 0], [c.IMMEDIATE, 1]]]);

var WarriorTestImp2 = new c.Warrior(
    'testImp2', 0, [[c.MOV|c.I, [c.IMMEDIATE, 0], [c.IMMEDIATE, 1]]]);


var CoreBasicTestCase = function () { };
CoreBasicTestCase.prototype = new unittest.TestCase();

CoreBasicTestCase.prototype.testInitialize = function () {
  var testsize = 300;
  var core = new c.Core({coresize: testsize});
  core.initialize();
  assert.equal(core.core.length, testsize);
  for (var i in core.core) {
    assert.deepEqual(core.core[i],
                     ([c.DAT, [c.IMMEDIATE, 0], [c.IMMEDIATE, 0]]));
  }
};

CoreBasicTestCase.prototype.testLoadWarriors = function () {
  var core = new c.Core({});
  core.loadWarriors([WarriorTestImp1, WarriorTestImp2]);
};

exports.CoreBasicTestCase = CoreBasicTestCase;
