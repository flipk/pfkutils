
#include "stdafx.h"
#include "BST.h"

using namespace BST_BASE;

BST_STREAM :: BST_STREAM(void)
{
	op = BST_OP_NONE;
}

//virtual
BST_STREAM :: ~BST_STREAM(void)
{
	/*placeholder*/
}

BST_STREAM_BUFFER :: BST_STREAM_BUFFER( void )
{
	buffer = nullptr;
	position = 0;
	remaining = 65536;
	size = 0;
}

BST_STREAM_BUFFER :: BST_STREAM_BUFFER( BST_BUFSEG ^ _buffer )
{
	buffer = _buffer;
	position = buffer->Offset;
	remaining = buffer->Count;
	size = 0;
}

BST_STREAM_BUFFER :: BST_STREAM_BUFFER( int starting_size )
{
	buffer = BST_BUFSEG(gcnew BST_BUF(size));
	position = 0;
	remaining = starting_size;
	size = 0;
}

void
BST_STREAM_BUFFER :: start(BST_OP _op, BST_BUFSEG ^ _buffer)
{
	op = _op;
	if (_buffer != nullptr)
	{
		buffer = _buffer;
		position = buffer->Offset;
		remaining = buffer->Count;
		size = 0;
	}
}

// virtual
bool
BST_STREAM_BUFFER :: get_ptr_noseg(int wanted)
{
	if (wanted > remaining)
		return false;
	position += wanted;
	size += wanted;
	remaining -= wanted;
	return true;
}

//virtual
BST_BUFSEG ^
BST_STREAM_BUFFER :: get_ptr(int wanted)
{
	if (wanted > remaining)
		return nullptr;
	BST_BUFSEG ^ ret = BST_BUFSEG(buffer->Array, position, wanted);
	position += wanted;
	size += wanted;
	remaining -= wanted;
	return ret;
}

BST :: BST(void)
{
	head = tail = next = nullptr;
	num_children = 0;
}

void
BST :: reg(BST ^ parent)
{
	if (parent->tail)
	{
		parent->tail->next = this;
		parent->tail = this;
	}
	else
	{
		parent->head = parent->tail = this;
	}
	parent->num_children ++;
}

array<BST^> ^ 
BST :: get_fields(void)
{
	array<BST^> ^ ret;
	ret = gcnew array<BST^>(num_children);
	int i=0;
	for (BST ^ b = head; b; b = b->next)
		ret[i++] = b;
	return ret;
}

//virtual
bool
BST :: bst_op( BST_STREAM ^ str )
{
	for (BST ^ b = head; b; b = b->next)
		if (!b->bst_op(str))
			return false;
	return true;
}

int
BST :: bst_calc_size(void)
{
	BST_STREAM_BUFFER  str; // no buffer
	str.start(BST_OP_CALC_SIZE);
	bst_op(%str);
	return str.get_finished_size();
}

BST_BUFSEG ^
BST :: bst_encode( BST_BUFSEG ^ buf )
{
	BST_STREAM_BUFFER str(buf);
	str.start(BST_OP_ENCODE);
	if (bst_op(%str) == false)
		return nullptr;
	return str.get_finished_buffer();
}

BST_BUFSEG ^
BST :: bst_encode( void )
{
	int len = bst_calc_size();
	BST_BUFSEG ^ ret = gcnew BST_BUFSEG(gcnew BST_BUF(len));
	return bst_encode(ret);
}

bool
BST :: bst_decode( BST_BUFSEG ^ buf )
{
	BST_STREAM_BUFFER  str(buf);
	str.start(BST_OP_DECODE);
	return bst_op(%str);
}

//virtual
bool
BST_UINT64_t :: bst_op( BST_STREAM ^ str )
{
	BST_BUFSEG ^ seg;
	switch (str->get_op()) {
		case BST_OP_ENCODE:
			seg = str->get_ptr(8);
			if (!seg)
				return false;
			seg->Array[seg->Offset+0] = (v >> 56) & 0xFF;
			seg->Array[seg->Offset+1] = (v >> 48) & 0xFF;
			seg->Array[seg->Offset+2] = (v >> 40) & 0xFF;
			seg->Array[seg->Offset+3] = (v >> 32) & 0xFF;
			seg->Array[seg->Offset+4] = (v >> 24) & 0xFF;
			seg->Array[seg->Offset+5] = (v >> 16) & 0xFF;
			seg->Array[seg->Offset+6] = (v >>  8) & 0xFF;
			seg->Array[seg->Offset+7] = (v >>  0) & 0xFF;
			return true;

		case BST_OP_DECODE:
			seg = str->get_ptr(8);
			if (!seg)
				return false;
			v = 
				((UInt64)seg->Array[seg->Offset+0] << 56) +
				((UInt64)seg->Array[seg->Offset+1] << 48) +
				((UInt64)seg->Array[seg->Offset+2] << 40) +
				((UInt64)seg->Array[seg->Offset+3] << 32) +
				((UInt64)seg->Array[seg->Offset+4] << 24) +
				((UInt64)seg->Array[seg->Offset+5] << 16) +
				((UInt64)seg->Array[seg->Offset+6] <<  8) +
				((UInt64)seg->Array[seg->Offset+7] <<  0);
			return true;

		case BST_OP_CALC_SIZE:
			return str->get_ptr_noseg(8);
	}
	return false;
}

//virtual
bool
BST_UINT32_t :: bst_op( BST_STREAM ^ str )
{
	BST_BUFSEG ^ seg;
	switch (str->get_op()) {
		case BST_OP_ENCODE:
			seg = str->get_ptr(4);
			if (!seg)
				return false;
			seg->Array[seg->Offset+0] = (v >> 24) & 0xFF;
			seg->Array[seg->Offset+1] = (v >> 16) & 0xFF;
			seg->Array[seg->Offset+2] = (v >>  8) & 0xFF;
			seg->Array[seg->Offset+3] = (v >>  0) & 0xFF;
			return true;

		case BST_OP_DECODE:
			seg = str->get_ptr(4);
			if (!seg)
				return false;
			v = 
				((UInt32)seg->Array[seg->Offset+0] << 24) +
				((UInt32)seg->Array[seg->Offset+1] << 16) +
				((UInt32)seg->Array[seg->Offset+2] <<  8) +
				((UInt32)seg->Array[seg->Offset+3] <<  0);
			return true;

		case BST_OP_CALC_SIZE:
			return str->get_ptr_noseg(4);
	}
	return false;
}

//virtual
bool
BST_UINT16_t :: bst_op( BST_STREAM ^ str ) 
{
	BST_BUFSEG ^ seg;
	switch (str->get_op()) {
		case BST_OP_ENCODE:
			seg = str->get_ptr(2);
			if (!seg)
				return false;
			seg->Array[seg->Offset+0] = (v >>  8) & 0xFF;
			seg->Array[seg->Offset+1] = (v >>  0) & 0xFF;
			return true;

		case BST_OP_DECODE:
			seg = str->get_ptr(2);
			if (!seg)
				return false;
			v = 
				((UInt16)seg->Array[seg->Offset+0] <<  8) +
				((UInt16)seg->Array[seg->Offset+1] <<  0);
			return true;

		case BST_OP_CALC_SIZE:
			return str->get_ptr_noseg(2);
	}
	return false;
}

//virtual
bool
BST_UINT8_t :: bst_op( BST_STREAM ^ str )
{
	BST_BUFSEG ^ seg;
	switch (str->get_op()) {
		case BST_OP_ENCODE:
			seg = str->get_ptr(1);
			if (!seg)
				return false;
			seg->Array[seg->Offset+0] = (v >>  0) & 0xFF;
			return true;

		case BST_OP_DECODE:
			seg = str->get_ptr(1);
			if (!seg)
				return false;
			v = 
				((UInt16)seg->Array[seg->Offset+0] <<  0);
			return true;

		case BST_OP_CALC_SIZE:
			return str->get_ptr_noseg(1);
	}
	return false;
}

BST_BINARY :: BST_BINARY(void)
{
	dim = 0;
	binary = nullptr;
}

BST_BINARY :: BST_BINARY(int _dim)
{
	dim = _dim;
	binary = gcnew cli::array<Byte>(dim);
}

void
BST_BINARY :: resize(int c)
{
	if (dim == c)
		return;

	cli::array<Byte> ^ newbin;
	if (c > 0)
		newbin = gcnew cli::array<Byte>(c);

	if (c > dim)
	{
		if (dim > 0)
		{
			System::Array::Copy(binary,0,newbin,0,dim);
			System::Array::Clear(newbin,dim,c-dim);
		}
	}
	else
	{ // c < size
		if (c > 0 && dim > 0)
			System::Array::Copy(binary,0,newbin,0,c);
	}

	if (newbin)
		delete binary;

	dim = c;
	binary = newbin;
}

//virtual
bool 
BST_BINARY :: bst_op(BST_STREAM ^ str)
{
	BST_BUFSEG ^ seg;
	BST_UINT16_t  count;

	switch (str->get_op()) {
		case BST_OP_ENCODE:
			count.v = dim;
			if (!count.bst_op(str))
				return false;
			seg = str->get_ptr(dim);
			if (!seg)
				return false;
			System::Array::Copy(binary,0,seg->Array,seg->Offset,dim);
			return true;

		case BST_OP_DECODE:
			if (!count.bst_op(str))
				return false;
			resize(count.v);
			seg = str->get_ptr(dim);
			if (!seg)
				return false;
			System::Array::Copy(seg->Array,seg->Offset,binary,0,dim);
			return true;

		case BST_OP_CALC_SIZE:
			if (!count.bst_op(str))
				return false;
			return str->get_ptr_noseg(dim);
	}
	return false;
}

//virtual 
bool 
BST_STRING :: bst_op(BST_STREAM ^ stream)
{
	BST_UINT16_t len;
	BST_UINT8_t  byte;

	switch (stream->get_op()) {
		case BST_OP_ENCODE:
			len.v = str->Length;
			if (!len.bst_op(stream))
				return false;
			for (int i = 0; i < len.v; i++)
			{
				byte.v = (Byte)str[i];
				if (!byte.bst_op(stream))
					return false;
			}
			return true;

		case BST_OP_DECODE:
			if (!len.bst_op(stream))
				return false;
			str = L"";
			for (int i = 0; i < len.v; i++)
			{
				if (!byte.bst_op(stream))
					return false;
				str = System::String::Concat(str, (Char)byte.v);
			}
			return true;

		case BST_OP_CALC_SIZE:
			if (!len.bst_op(stream))
				return false;
			return stream->get_ptr_noseg(str->Length);
	}
	return false;
}

BST_UNION :: BST_UNION(void)
{
	which.v = 0xFF;
	fields = nullptr;
}

//virtual 
bool
BST_UNION :: bst_op(BST_STREAM ^ stream)
{
	if (!fields)
		fields = get_fields();
	if (!which.bst_op(stream))
		return false;
	if (which.v > fields->Length)
		return false;
	if (!fields[which.v]->bst_op(stream))
		return false;
	return true;
}
