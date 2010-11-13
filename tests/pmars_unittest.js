var unittest = require('unittest');
var pmars = require('pmars');

var PmarsTestCase = function () { 
  unittest.TestCase.apply(this, arguments);
};
PmarsTestCase.prototype = new unittest.TestCase();

PmarsTestCase.prototype.testAssemble = function () {
  var validate = __dirname + '/../third_party/pmars-0.9.2/warriors/validate.red';
  var assembler = new pmars.Assembler();
  var parsed = assembler.assemble(0, validate);
};

exports.PmarsTestCase = PmarsTestCase;
