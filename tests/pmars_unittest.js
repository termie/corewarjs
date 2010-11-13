var unittest = require('unittest');
var pmars = require('pmars');

var PmarsTestCase = function () { };
PmarsTestCase.prototype = new unittest.TestCase();

PmarsTestCase.prototype.testAssemble = function () {
  var validate = __dirname + '/third_party/pmars-0.9.2/warriors/validate.red';
  var assembler = pmars.Assembler();
  var parsed = assembler.Assemble(0, validate);
};

exports.PmarsTestCase = PmarsTestCase;
