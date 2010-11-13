/* pMARS binding for node.js
 * I assume this is probably also GPL'd as pMARS is.
 * Copyright (C) 2010 Andy Smith
 */

#include <stdio.h>
#include <node.h>
#include <v8.h>

extern "C" {
	#include "config.h"
	#include "global.h"
 	#include "asm.h"
}

using namespace v8;
using namespace node;

typedef mem_struct mem_st;

/* whether INITIALINST is displayed or not */
#define HIDE 0
#define SHOW 1

class Assembler {
  public:
    static void
    Initialize(Handle<Object> target)
    {
      HandleScope scope;
      Local<FunctionTemplate> t = FunctionTemplate::New(New);

      t->InstanceTemplate()->SetInternalFieldCount(1);

      NODE_SET_PROTOTYPE_METHOD(t, "assemble", Assemble);
      target->Set(String::NewSymbol("Assembler"), t->GetFunction());
    }

  protected:
    static Handle<Value>
    New (const Arguments& args)
    {
      HandleScope scope;
      return args.This();
    }

    static Handle<Value>
    Assemble(const Arguments& args)
    {
      HandleScope scope;
      Assembler *assembler = ObjectWrap::Unwrap<Assembler>(args.This());
      
      if (args.Length() < 2 || !args[1]->IsString() || !args[0]->IsInt32()) {
        return ThrowException(Exception::Error(
              String::New("First arg is warrior num, second is filename")));
      }

      Local<Int32> warriorNumInt(args[0]->ToInt32());
      int32_t warriorNum = warriorNumInt->Value();
      
      String::Utf8Value filename(args[1]->ToString());

      coreSize = DEFAULTCORESIZE;
      instrLim = DEFAULTINSTRLIM;

      int ass = assemble(*filename, warriorNum);
      Local<Value> rv = assembler->Disassemble(warriorNum);
      return rv;
    }

  private:
    static Local<Value>
    Disassemble(const int32_t warriorNum)
    {
      HandleScope scope;
      ADDR_T i;
      char buf[MAXALLCHAR];

      mem_struct *cells = warrior[warriorNum].instBank;
      ADDR_T n = warrior[warriorNum].instLen;
      ADDR_T offset = warrior[warriorNum].offset;
      
      Local<Array> instructions = Array::New((int) n);
      Local<Object> rv = Object::New();

      rv->Set(String::New("offset"), Integer::New(offset));
      rv->Set(String::New("instructions"), instructions);


      for (i = 0; i < n; ++i) {
        instructions->Set(i,
            String::New(cellview((mem_struct *) cells + i, buf, 1)));
      }
      return rv;
    }
};

extern "C" void
init (Handle<Object> target)
{
  HandleScope scope;
  Assembler::Initialize(target);
}
