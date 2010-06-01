var assert = require('assert');
var sys = require('sys');

var c = require('./core');


var TestCase = function () { };
TestCase.prototype.setUp = function () { };
TestCase.prototype.tearDown = function () { };
TestCase.prototype.run = function () {
  this.passes = [];
  this.failures = [];
  this.errors = [];
  for (var k in this) {
    if (k.match(/^test/)) {
      this.setUp();
      try {
        this[k]();
        sys.print('.');
        this.passes.push(k);
      } catch (e) {
        if (e instanceof assert.AssertionError) {
          sys.print('F');
          this.failures.push([k, e]);
        } else {
          sys.print('E');
          this.errors.push([k, e]);
        }
      }
      this.tearDown();
    }
  }

  sys.puts('');
  for (var i in this.failures) {
    sys.puts('-----------------------------------');
    sys.puts('FAIL: ' + this.failures[i][0]);
    sys.puts('-----------------------------------');
    sys.puts(this.failures[i][1]);
  }

  for (var i in this.errors) {
    sys.puts('-----------------------------------');
    sys.puts('ERROR: ' + this.errors[i][0]);
    sys.puts('-----------------------------------');
    sys.puts(this.errors[i][1]);
  }

  sys.puts('Results: Passed ' + this.passes.length +
           ', Failed ' + this.failures.length +
           ', Errors ' + this.errors.length);
};




var WarriorTestImp1 = new c.Warrior(
    'testImp1', 0, [[c.MOV|c.I, [c.IMMEDIATE, 0], [c.IMMEDIATE, 1]]]);

var WarriorTestImp2 = new c.Warrior(
    'testImp2', 0, [[c.MOV|c.I, [c.IMMEDIATE, 0], [c.IMMEDIATE, 1]]]);






var CoreBasicTestCase = function () { };
CoreBasicTestCase.prototype = new TestCase();

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

var testcase = new CoreBasicTestCase();
testcase.run();
