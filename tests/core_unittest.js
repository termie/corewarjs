var util = require('util');
var unittest = require('unittest');
var c = require('corewar');

// a couple warriors as described at http://vyznev.net/corewar/guide.html

var Imp = function () {
  return new c.Warrior(
    'imp',
    0,
    [
      [c.MOV|c.I, [c.DIRECT, 0], [c.DIRECT, 1]]
    ]
  );
};


var Dwarf = function () { 
  return new c.Warrior(
    'dwarf',
    0,
    [
      [c.ADD, [c.IMMEDIATE, 4], [c.DIRECT, 3]],
      [c.MOV, [c.DIRECT, 2], [c.INDIRECT|c.B, 2]],
      [c.JMP, [c.DIRECT, -2], [c.IMMEDIATE, 0]],
      [c.DAT, [c.IMMEDIATE, 0], [c.IMMEDIATE, 0]],
    ]
  );
};

var CoreBasicTestCase = function () {
  unittest.TestCase.apply(this, arguments);
};
CoreBasicTestCase.prototype = new unittest.TestCase();
CoreBasicTestCase.prototype.setUp = function () {

};

CoreBasicTestCase.prototype.testInitialize = function () {
  var testsize = 300;
  var core = new c.Core({coresize: testsize});
  core.initialize();
  this.assertEqual(core.core.length, testsize);
  for (var i in core.core) {
    this.assertDeepEqual(core.core[i],
                     ([c.DAT, [c.IMMEDIATE, 0], [c.IMMEDIATE, 0]]));
  }
};

CoreBasicTestCase.prototype.testLoadWarriors = function () {
  var coresize = 400;
  var mindistance = 100;
  var core = new c.Core({coresize: coresize, mindistance: mindistance});
  var imp = Imp();
  var dwarf = Dwarf();

  core.initialize();
  core.loadWarriors([imp, dwarf]);

  this.assertEqual(core.warriors, 2);
  var imp_pos = null;
  var dwarf_pos = null;

  for (var k in core.core) {
    if (core.core[k] == imp.instructions[0]) imp_pos = k;
    if (core.core[k] == dwarf.instructions[0]) dwarf_pos = k;
  }
  
  this.assertOk((Math.abs(imp_pos - dwarf_pos) >= mindistance));
  this.assertEqual(imp.curProcess.position, imp_pos);
  this.assertEqual(dwarf.curProcess.position, dwarf_pos);

  core.runOnce(true);
  core.runOnce(true);
  core.runOnce(true);
  core.runOnce(true);
  core.runOnce(true);
  core.runOnce(true);
  core.runOnce(true);
  core.runOnce(true);
};

exports.CoreBasicTestCase = CoreBasicTestCase;
