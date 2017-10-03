
#pragma once

using namespace System;
using namespace BST_BASE;
using namespace BST_MSG;

public ref class MyType2 : public BST {
public:
	MyType2(void) {
		four.reg(this);
		five.reg(this);
	}
	BST_BINARY    four;
	BST_STRING    five;
};

public ref class MyType3 : public BST_UNION {
public:
	enum class msgtype { EIGHT, NINE };
	MyType3(void) {
		eight.reg(this);
		nine.reg(this);
	}
	BST_STRING  eight;
	BST_UINT16_t nine;
};

public ref class MyType : public BST {
public:
	MyType(void) {
		one.reg(this);
		two.reg(this);
		three.reg(this);
		six.reg(this);
		seven.reg(this);
	}
	BST_UINT64_t  one;
	BST_UINT16_t  two;
	BST_POINTER<MyType2>  three;
	BST_ARRAY<BST_UINT16_t> six;
	MyType3  seven;
};

PkMsgExtDef( MyMessage, 0x1234, MyType );
