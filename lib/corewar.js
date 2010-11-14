// Command modifiers
var A = 1;  // A & B are overloaded for instruction access and operators
var B = 2;
var AB = 4;
var BA = 8;
var F = 16;
var X = 32;
var I = 64;
var ALL_MODIFIERS = A | B | AB | BA | F | X | I;
var MOD_REVERSE = {
  A: A,
  B: B,
  AB: AB,
  BA: BA,
  F: F,
  X: X,
  I: I,
};

// Commands (these share space with the modifiers)
var DAT = 128;
var MOV = 256;
var ADD = 512;
var SUB = 1024;
var MUL = 2048;
var DIV = 4096;
var MOD = 8192;
var JMP = 16384;
var JMZ = 32768;
var JMN = 65536;
var DJN = 131072;
var SPL = 262144;
var CMP = 524288;
var SEQ = 1048576;
var SNE = 2097152;
var SLT = 4194304;
var LDP = 8388608;
var STP = 16777216;
var NOP = 33554432;
var CMD_REVERSE = {
  DAT: DAT,
  MOV: MOV,
  ADD: ADD,
  SUB: SUB,
  MUL: MUL,
  DIV: DIV,
  MOD: MOD,
  JMP: JMP,
  JMZ: JMZ,
  JMN: JMN,
  DJN: DJN,
  SPL: SPL,
  CMP: CMP,
  SEQ: SEQ,
  SNE: SNE,
  SLT: SLT,
  LDP: LDP,
  STP: STP,
  NOP: NOP,
};

// Operators
var IMMEDIATE = 4;
var DIRECT = 8;
var INDIRECT = 16;
var PREDECREMENT = 32;
var POSTINCREMENT = 64;

var OP_LOOKUP = {
  '#': IMMEDIATE,
  '$': DIRECT,
  '*': INDIRECT|A,
  '@': INDIRECT|B,
  '{': PREDECREMENT|A,
  '<': PREDECREMENT|B,
  '}': POSTINCREMENT|A,
  '>': POSTINCREMENT|B
};

// Array access
var OP = 0;
var CMD = 0;
var VALUE = 1;

// Operators for abstraction
var opAdd = function (a, b) { return a + b; };
var opSub = function (a, b) { return a - b; };
var opMul = function (a, b) { return a * b; };
var opDiv = function (a, b) { return Math.round(a / b); };
var opMod = function (a, b) { return a % b; };

// Shortcut to increment and mod
var modinc = function (num, inc, max) { return ((num + inc) % max); };

// Helpers to determine whether we _shouldn't_ jump/skip
var notJmz = function (a, b) { return b !== 0; };
var notJmn = function (a, b) { return b === 0; };
var notSeq = function (a, b) { return a != b; };
var notSne = function (a, b) { return a == b; };
var notSlt = function (a, b) { return a >= b; };


/** Methods copied directly from Node's EventEmitter class
 *  with minor modifications */
var EventEmitter = function () {
  this._events = {};
};
EventEmitter.prototype.emit = function (type) {
  // If there is no 'error' event listener then throw.
  if (type === 'error') {
    if (!this._events || !this._events.error ||
        (Array.isArray(this._events.error) && !this._events.error.length))
    {
      if (arguments[1] instanceof Error) {
        throw arguments[1];
      } else {
        throw new Error("Uncaught, unspecified 'error' event.");
      }
      return false;
    }
  }

  if (!this._events) return false;
  if (!this._events[type]) return false;

  if (typeof this._events[type] == 'function') {
    if (arguments.length < 2) {
      // fast case
      this._events[type].call( this
                             , arguments[1]
                             //, arguments[2]
                             );
    } else {
      // slower
      var args = Array.prototype.slice.call(arguments, 1);
      this._events[type].apply(this, args);
    }
    return true;

  } else if (Array.isArray(this._events[type])) {
    var args = Array.prototype.slice.call(arguments, 1);


    var listeners = this._events[type].slice(0);
    for (var i = 0, l = listeners.length; i < l; i++) {
      listeners[i].apply(this, args);
    }
    return true;

  } else {
    return false;
  }
};
EventEmitter.prototype.addListener = function (type, listener) {
  if ('function' !== typeof listener) {
    throw new Error('addListener only takes instances of Function');
  }

  if (!this._events) this._events = {};

  // To avoid recursion in the case that type == "newListeners"! Before
  // adding it to the listeners, first emit "newListeners".
  this.emit("newListener", type, listener);

  if (!this._events[type]) {
    // Optimize the case of one listener. Don't need the extra array object.
    this._events[type] = listener;
  } else if (Array.isArray(this._events[type])) {
    // If we've already got an array, just append.
    this._events[type].push(listener);
  } else {
    // Adding the second element, need to change to array.
    this._events[type] = [this._events[type], listener];
  }

  return this;
};
EventEmitter.prototype.on = EventEmitter.prototype.addListener;
EventEmitter.prototype.removeListener = function (type, listener) {
  if ('function' !== typeof listener) {
    throw new Error('removeListener only takes instances of Function');
  }

  // does not use listeners(), so no side effect of creating _events[type]
  if (!this._events || !this._events[type]) return this;

  var list = this._events[type];

  if (Array.isArray(list)) {
    var i = list.indexOf(listener);
    if (i < 0) return this;
    list.splice(i, 1);
    if (list.length == 0)
      delete this._events[type];
  } else if (this._events[type] === listener) {
    delete this._events[type];
  }

  return this;
};
EventEmitter.prototype.removeAllListeners = function (type) {
  // does not use listeners(), so no side effect of creating _events[type]
  if (type && this._events && this._events[type]) this._events[type] = null;
  return this;
};
EventEmitter.prototype.listeners = function (type) {
  if (!this._events) this._events = {};
  if (!this._events[type]) this._events[type] = [];
  if (!Array.isArray(this._events[type])) {
    this._events[type] = [this._events[type]];
  }
  return this._events[type];
};


/**
 * Parse from the output of our Assemble method, the output is very consistent.
 */
var parseSimple = function (lines) {
  var instructions = [];
  for (var i in lines) {
    var line = lines[i];
    var cmd = line.substr(0, 3);
    var mod = line.substr(4, 1);
    var op_a = line.substr(7, 1);
    var value_a = line.substr(8, 6);
    var op_b = line.substr(16, 1);
    var value_b = line.substr(17, 6);

    //var dbg = [];
    //dbg.push([cmd, mod]);
    //dbg.push([op_a, value_a])
    //dbg.push([op_b, value_b])
    //console.log(dbg);

    var inst = [];
    inst.push(exports[cmd] + exports[mod]);
    inst.push([OP_LOOKUP[op_a], parseInt(value_a)])
    inst.push([OP_LOOKUP[op_b], parseInt(value_b)])
    instructions.push(inst);
  }
  return instructions;
}


var dump = function (inst) {
  var out = [];
  for (var c in CMD_REVERSE) {
    if (inst[CMD] & CMD_REVERSE[c]) {
      out.push(c);
    }
  }
  var mod = false;
  for (var d in MOD_REVERSE) {
    if (inst[CMD] & MOD_REVERSE[d]) {
      out.push(d);
      mod = true;
    }
  }
  if (!mod) out.push('_');
  
  for (var e in OP_LOOKUP) {
    if (inst[A][OP] & OP_LOOKUP[e]) {
      out.push(e);
      out.push(inst[A][VALUE]);
    }
  }
  for (var f in OP_LOOKUP) {
    if (inst[B][OP] & OP_LOOKUP[f]) {
      out.push(f);
      out.push(inst[B][VALUE]);
    }
  }

  return out[0] + '.' + out[1] + ' ' + out[2] + out[3] + ' ' + out[4] + out[5];
}


var pad = function pad(number, length) {
  var str = '' + number;
  while (str.length < length) {
    str = '0' + str;
  }
  return str;
};

/**
 *
 * Warrior interaction
 *
 * A Warrior contains possibly multiple processes and keeps track of them
 * internally.
 *
 * The flow for executing a process is:
 *
 * var process = aWarrior.start() <-- returns the curProcess
 * [ ... execute command at process.position ... ]
 * aWarrior.end() <-- sets curProcess to nextProcess
 */

var Warrior = function (name, offset, instructions) {
  EventEmitter.call(this);
  this.name = name;
  this.processes = 0;

  this.curProcess = null;
  this.nextProcess = null;

  this.offset = offset || 0;
  if (instructions && typeof(instructions[0]) === 'string') {
    instructions = parseSimple(instructions);
  }
  this.instructions = instructions || [];
  
  // Links for warrior list
  this.next = null;
  this.prev = null;
};
Warrior.prototype = new EventEmitter();
/**
 * Return the process that should execute now
 */
Warrior.prototype.start = function () {
  return this.curProcess;
};
Warrior.prototype.end = function () {
  this.curProcess = this.nextProcess;
  this.nextProcess = this.curProcess.next;
};
Warrior.prototype.kill = function () {
  var process = this.curProcess;
  this.curProcess = null;
  this.processes -= 1;
  if (this.processes == 0) {
    // If this is the last process set nextProcess to null, too;
    this.nextProcess = null;
  } else if (this.processes == 1) {
    // If we only have one process make sure it points to itself
    this.nextProcess.prev = this.nextProcess;
    this.nextProcess.next = this.nextProcess;
  } else {
    // Otherwise just switch the pointers
    process.prev.next = this.nextProcess;
    this.nextProcess.prev = process.prev;
  }
  this.emit('kill', process);
};
Warrior.prototype.split = function (newPosition) {
  this.processes += 1;
  var newProcess = {position: newPosition};
  newProcess.prev = this.curProcess;
  newProcess.next = this.curProcess.next;
  this.curProcess.next = newProcess;
  this.nextProcess = newProcess;
  this.emit('split', {process: this.curProcess, newProcess: newProcess});
};
Warrior.prototype.spawn = function (position) {
  if (this.processes > 0) {
    throw "Attempting to spawn a new process when a process is already running";
  }
  this.processes = 1;
  this.curProcess = {position: position};
  this.curProcess.next = this.curProcess;
  this.curProcess.prev = this.curProcess;
  this.nextProcess = this.curProcess;
  this.emit('spawn', this.curProcess);
}
Warrior.prototype.seek = function (position) {
  this.curProcess.position = position;
  this.emit('seek', this.curProcess);
};


var Core = function (kw) {
  EventEmitter.call(this);
  if (!kw) kw = {};
  this.coresize = kw.coresize || 8000;
  this.pspacesize = kw.pspacesize || 500;
  this.maxcycles = kw.maxcycles || 80000;
  this.maxprocesses = kw.maxprocesses || 8000;
  this.warriors = kw.warriors || 2;
  this.maxlength = kw.maxlength || 100;
  this.mindistance = kw.mindistance || 100;

  this.core = [];
  this.pspace = [];
  
  this.cycle = 0;

  this.curWarrior = null;
  this.nextWarrior = null;
};
Core.prototype = new EventEmitter();
Core.prototype.initialize = function () {
  for (var i = 0; i < this.coresize; i++) {
    this.core[i] = [DAT, [IMMEDIATE, 0], [IMMEDIATE, 0]];
  }
};
Core.prototype.loadWarriors = function (warriors) {
  // optimistically pick spots and check that they are each length + mindistance
  // apart
  var loadPoints = [];
  while (loadPoints.length < warriors.length) {
    var rnd = Math.floor(Math.random() * (this.coresize + 1));
    var viable = true;
    for (var i in loadPoints) {
      if (Math.abs(loadPoints[i] - rnd) < this.mindistance) {
        viable = false;
      }
    }
    if (viable) {
      loadPoints.push(rnd);
    }
  }

  var firstWarrior = warriors[0];
  var lastWarrior = null;
  this.warriors = warriors.length;
  for (var i in warriors) {
    var loadPoint = loadPoints[i];
    var warrior = warriors[i];
    var instructions = warrior.instructions;

    if (lastWarrior) {
      warrior.prev = lastWarrior;
      lastWarrior.next = warrior;
    }
    lastWarrior = warrior;

    for (var j in instructions) {
      this.core[(loadPoint + parseInt(j)) % this.coresize] = instructions[j];
    }

    warrior.spawn(loadPoint + warrior.offset);
  }
  lastWarrior.next = firstWarrior;
  firstWarrior.prev = lastWarrior;
  this.curWarrior = firstWarrior;
  this.nextWarrior = this.curWarrior.next;
};
Core.prototype.resolvePosition = function (address, current) {
  if (address[OP] == IMMEDIATE) {
    relative = 0;
  } else {
    relative = address[1];
  }
  return modinc(current, relative, this.coresize);
};
Core.prototype.instInc = function (position, field, inc) {
  this.core[position][field][VALUE] = modinc(
      this.core[position][field][VALUE], inc, this.coresize);
  this.instChanged(position)
};
Core.prototype.instChanged = function (position) {
  var inst = this.core[position];
  this.emit('coreChanged', {'position': position, 'instruction': inst});
};
Core.prototype.setDefaultCommandModifiers = function (inst) {
  var cmd = inst[CMD];
  // set up default modifiers if none are present
  if (!(cmd & ALL_MODIFIERS)) {
    if (cmd & (MOV|SEQ|SNE|CMP)) {
      if (inst[A][OP] & IMMEDIATE)      cmd |= AB;
      else if (inst[B][OP] & IMMEDIATE) cmd |= B;
      else                              cmd |= I;
    } else if (cmd & (ADD|SUB|MUL|DIV|MOD)) {
      if (inst[A][OP] & IMMEDIATE)      cmd |= AB;
      else if (inst[B][OP] & IMMEDIATE) cmd |= B;
      else                              cmd |= F;
    } else if (cmd & (SLT|LDP|STP)) {
      if (inst[A][OP] & IMMEDIATE)      cmd |= AB;
      else                              cmd |= B;
    }
  }
  inst[CMD] = cmd;
  return inst;
};
Core.prototype.handlePredecrement = function (inst, position) {
  var decrPos;
  if (inst[A] & PREDECREMENT) {
    decrPos = this.resolvePosition(inst[A], position);
    if (inst[A] & A) {
      this.instInc(decrPos, A, -1);
    } else if (inst[A] & B) {
      this.instInc(decrPos, B, -1);
    }
  }
  if (inst[B] & PREDECREMENT) {
    decrPos = this.resolvePosition(inst[B], position);
    if (inst[B] & A) {
      this.instInc(decrPos, A, -1);
    } else if (inst[B] & B) {
      this.instInc(decrPos, B, -1);
    }
  }
};
Core.prototype.handlePostincrement = function (inst, position) {
  var incPos;
  if (inst[A][OP] & POSTINCREMENT) {
    incPos = this.resolvePosition(inst[A], position);
    if (inst[A][OP] & A) {
      this.instInc(incPos, A, 1);
    } else if (inst[A][OP] & B) {
      this.instInc(incPos, B, 1);
    }
  }
  if (inst[B][OP] & POSTINCREMENT) {
    incPos = this.resolvePosition(inst[B], position);
    if (inst[B][OP] & A) {
      this.instInc(incPos, A, 1);
    } else if (inst[B][OP] & B) {
      this.instInc(incPos, B, 1);
    }
  }
};
Core.prototype.handleDAT = function (warrior, process, position, inst, cmd) {
  warrior.kill(process);
};
Core.prototype.handleSPL = function (warrior, process, position, inst, cmd) {
  var splitToPosition = this.resolvePosition(inst[A], position);
  var thisNewPosition = modinc(position, 1, this.coresize);

  warrior.seek(thisNewPosition);
  warrior.split(splitToPosition);
};
Core.prototype.handleMOV = function (warrior, process, position, inst, cmd) {
  var source = this.resolvePosition(inst[A], position);
  var target = this.resolvePosition(inst[B], position);

  if (cmd & (A|F|I)) this.core[target][A] = this.core[source][A];
  if (cmd & (B|F|I)) this.core[target][B] = this.core[source][B];
  if (cmd & (AB|X)) this.core[target][B] = this.core[source][A];
  if (cmd & (BA|X)) this.core[target][A] = this.core[source][B];
  if (cmd & I) this.core[target][CMD] = this.core[source][CMD];
  
  warrior.seek(modinc(position, 1, this.coresize));
};
Core.prototype.handleADD = function (warrior, process, position, inst, cmd) {
  var source = this.resolvePosition(inst[A], position);
  var target = this.resolvePosition(inst[B], position);
  var m = (cmd & SUB) ? -1 : 1;
  
  var targetA = this.core[target][A][VALUE] % this.coresize;
  var targetB = this.core[target][B][VALUE] % this.coresize;
  var sourceA = this.core[source][A][VALUE] % this.coresize; 
  var sourceB = this.core[source][B][VALUE] % this.coresize; 
  
  var op;
  if (cmd & ADD) op = opAdd;
  if (cmd & SUB) op = opSub;
  if (cmd & MUL) op = opMul;
  if (cmd & DIV) op = opDiv;
  if (cmd & MOD) op = opMod;

  if (cmd & (DIV|MOD) && cmd & (A|F|I|AB|X) && sourceA === 0) {
    // divide by zero
    warrior.kill();
  } else {
    if (cmd & (A|F|I)) this.core[target][A][VALUE] = op(targetA, sourceA); 
    if (cmd & (AB|X)) this.core[target][B][VALUE] = op(targetB, sourceA); 
  }

  if (cmd & (DIV|MOD) && cmd & (B|F|I|BA|X) && sourceB === 0) {
    // divide by zero
    warrior.kill();
  } else {
    if (cmd & (B|F|I)) this.core[target][B][VALUE] = op(targetB, sourceB);
    if (cmd & (BA|X)) this.core[target][A][VALUE] = op(targetA, sourceB);
  }
  
  this.instChanged(target);
  warrior.seek(modinc(position, 1, this.coresize));
};
Core.prototype.handleJMP = function (warrior, process, position, inst, cmd) {
  var newPosition = this.resolvePosition(inst[A], position);
  warrior.seek(newPosition);
};
Core.prototype.handleJMZ = function (warrior, process, position, inst, cmd) {
  var source = this.resolvePosition(inst[A], position);
  var target = this.resolvePosition(inst[B], position);
  
  var targetA = this.core[target][A][VALUE] % this.coresize;
  var targetB = this.core[target][B][VALUE] % this.coresize;
  var targetCMD = this.core[target][CMD];
  var sourceA = this.core[source][A][VALUE] % this.coresize; 
  var sourceB = this.core[source][B][VALUE] % this.coresize; 
  var sourceCMD = this.core[source][CMD];

  var testFunc;
  if (cmd & JMZ) testFunc = notJmz;
  if (cmd & (JMN|DJN)) testFunc = notJmn;
  if (cmd & (CMP|SEQ)) testFunc = notSeq;
  if (cmd & SNE) testFunc = notSne;
  if (cmd & SLT) testFunc = notSlt;
  
  // DJN decrements before testing
  if (cmd & DJN) {
    if (cmd & (A|F|I|BA|X)) this.instInc(target, A, -1);
    if (cmd & (B|F|I|AB|X)) this.instInc(target, B, -1);
  }

  // do the testing
  var jump = true;
  if (cmd & (A|F|I)) jump = testFunc(sourceA, targetA);
  if (cmd & (B|F|I)) jump = testFunc(sourceB, targetB);
  if (cmd & (AB|X)) jump = testFunc(sourceA, targetB);
  if (cmd & (BA|X)) jump = testFunc(sourceB, targetA);
  if (cmd & (CMP|SEQ) && cmd & I && targetCMD != sourceCMD) jump = false;
  if (cmd & (SNE) && cmd & I && targetCMD == sourceCMD) jump = false;

  // jump
  if (jump) {
    if (cmd & (CMP|SEQ|SNE|SLT)) {
      warrior.seek(modinc(position, 2, this.coresize));
    } else {
      // for jumps the resolved "source" is the destination
      warrior.seek(source);
    }
  } else {
    warrior.seek(modinc(position, 1, this.coresize));
  }
};
Core.prototype.handleNOP = function (warrior, process, position, inst, cmd) {
  process.position = modinc(position, 1, this.coresize);
};
Core.prototype.start = function () {
  return this.curWarrior;
};
Core.prototype.end = function () {
  this.curWarrior = this.nextWarrior;
  this.nextWarrior = this.curWarrior.next;
};
Core.prototype.runOnce = function (DEBUG) {
  if (this.warriors === 0) {
    // nothing to do
    return;
  }

  var warrior = this.start();
  var process = warrior.start();
  var position = process.position;
  var inst = this.core[position];
  inst = this.setDefaultCommandModifiers(inst);
  var cmd = inst[CMD];
  
  if (DEBUG) {
    console.log(pad(this.cycle, 5) +
                ' (' + warrior.name + ':' + pad(position, 5) + ')-> '
                + dump(inst)); 
  }

  this.handlePredecrement(inst, position);
  
  // Handle commands
  if (cmd & DAT) {
    this.handleDAT(warrior, process, position, inst, cmd);
  } else if (cmd & SPL) {
    if (warrior.processes == this.maxprocesses) {
      this.handleNOP(warrior, process, position, inst, cmd);
    } else {
      this.handleSPL(warrior, process, position, inst, cmd);
    }
  } else if (cmd & MOV) {
    this.handleMOV(warrior, process, position, inst, cmd);
  } else if (cmd & (ADD|SUB|MUL|DIV|MOD)) {
    this.handleADD(warrior, process, position, inst, cmd);
  } else if (cmd & JMP) {
    this.handleJMP(warrior, process, position, inst, cmd);
  } else if (cmd & (JMZ|JMN|DJN|SEQ|SNE|CMP|SLT)) {
    this.handleJMZ(warrior, process, position, inst, cmd);
  } else if (cmd & NOP) {
    this.handleNOP(warrior, process, position, inst, cmd);
  }

  this.handlePostincrement(inst, position);
  
  // The warrior's process has done all it can.
  warrior.end();
  
  // So has the warrior;
  this.end();

  if (warrior.processes === 0) {
    this.defeat();
  }

  if (this.warriors === 1) {
    this.emit('victory', warrior);
  }
  

  this.cycle += 1;
  if (this.cycle >= this.maxcycles) {
    this.emit('stalemate');
  }
};
Core.prototype.defeat = function () {
  var warrior = this.curWarrior;
  this.warriors -= 1;
  this.curWarrior = null;
  if (this.warriors == 0) {
    // If this is the last warrior set nextWarrior to null, too;
    this.nextWarrior = null;
  } else if (this.warriors == 1) {
    // If we only have on warrior make sure it points to itself
    this.nextWarrior.next = this.nextWarrior;
    this.nextWarrior.prev = this.nextWarrior;
  } else {
    // Otherwise just switch the pointers
    warrior.prev.next = this.nextWarrior;
    this.nextWarrior.prev = warrior.prev;
    
  }
  this.emit('defeat', warrior);
}


exports.Core = Core;
exports.Warrior = Warrior;
exports.parseSimple = parseSimple;
exports.OP_LOOKUP = OP_LOOKUP;

// Command modifiers
exports.A = A;
exports.B = B;
exports.AB = AB;
exports.BA = BA;
exports.F = F;
exports.X = X;
exports.I = I;
exports.ALL_MODIFIERS = ALL_MODIFIERS;

// Commands (these share space with the modifiers)
exports.DAT = DAT;
exports.MOV = MOV;
exports.ADD = ADD;
exports.SUB = SUB;
exports.MUL = MUL;
exports.DIV = DIV;
exports.MOD = MOD;
exports.JMP = JMP;
exports.JMZ = JMZ;
exports.JMN = JMN;
exports.DJN = DJN;
exports.SPL = SPL;
exports.CMP = CMP;
exports.SEQ = SEQ;
exports.SNE = SNE;
exports.SLT = SLT;
exports.LDP = LDP;
exports.STP = STP;
exports.NOP = NOP;

// Operators
exports.IMMEDIATE = IMMEDIATE;
exports.DIRECT = DIRECT;
exports.INDIRECT = INDIRECT;
exports.PREDECREMENT = PREDECREMENT;
exports.POSTINCREMENT = POSTINCREMENT;

// Array access
exports.OP = OP;
exports.CMD = CMD;
exports.VALUE = VALUE;


