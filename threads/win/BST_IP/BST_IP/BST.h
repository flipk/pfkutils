
#pragma once

using namespace System;

namespace BST_BASE {

	typedef cli::array<Byte> BST_BUF;
	typedef System::ArraySegment<Byte> BST_BUFSEG;

	enum BST_OP {
		BST_OP_NONE = 0,
		BST_OP_ENCODE = 1,
		BST_OP_CALC_SIZE = 2,
		BST_OP_DECODE = 3
	};

#ifdef  BUILDING_BST_DLL

	public ref class BST_STREAM abstract {
	protected:
		BST_OP    op;
	public:
		BST_STREAM(void);
		virtual ~BST_STREAM(void);
		BST_OP get_op(void) { return op; }
		virtual bool get_ptr_noseg(int wanted) = 0;
		virtual BST_BUFSEG ^ get_ptr(int wanted) = 0;
	};

	public ref class BST_STREAM_BUFFER : public BST_STREAM {
		BST_BUFSEG ^ buffer; // offset and count
		int position; // absolute pos in buffer->array
		int remaining; // counts down from buffer->count to 0
		int size; // counts up from 0 to buffer->count
	public:
		BST_STREAM_BUFFER( void );
		BST_STREAM_BUFFER( BST_BUFSEG ^ _buffer );
		BST_STREAM_BUFFER( int starting_size );
		void start(BST_OP _op, BST_BUFSEG ^ _buffer);
		void start(BST_OP _op) { start(_op, nullptr); }
		BST_BUFSEG ^ get_finished_buffer(void) {
			return gcnew BST_BUFSEG(buffer->Array, buffer->Offset, size);
		}
		int get_finished_size(void) { return size; }
		virtual bool get_ptr_noseg(int wanted) override;
		virtual BST_BUFSEG ^ get_ptr(int wanted) override;
	};

	public ref class BST
	{
	protected:
		BST ^ head;
		BST ^ tail;
		BST ^ next;
		int num_children;
		array<BST^> ^ get_fields(void);
	public:
		BST(void);
		void reg(BST ^ parent);
		virtual ~BST(void) { /*placeholder*/ }
		virtual bool bst_op( BST_STREAM ^ str );
		int bst_calc_size(void);
		BST_BUFSEG ^ bst_encode( BST_BUFSEG ^ buf );
		BST_BUFSEG ^ bst_encode( void );
		bool bst_decode( BST_BUFSEG ^ buf );
	};

	public ref class BST_UINT64_t : public BST {
	public:
		UInt64 v;
		virtual bool bst_op( BST_STREAM ^ str ) override;
	};

	public ref class BST_UINT32_t : public BST {
	public:
		UInt32 v;
		virtual bool bst_op( BST_STREAM ^ str ) override;
	};

	public ref class BST_UINT16_t : public BST {
	public:
		UInt16 v;
		virtual bool bst_op( BST_STREAM ^ str ) override;
	};

	public ref class BST_UINT8_t : public BST {
	public:
		Byte v;
		virtual bool bst_op( BST_STREAM ^ str ) override;
	};

	public ref class BST_BINARY : public BST {
	public:
		BST_BINARY(void);
		BST_BINARY(int _dim);
		int dim;
		cli::array<Byte> ^ binary;
		void resize(int c);
		virtual bool bst_op(BST_STREAM ^ str) override;
	};

	public ref class BST_STRING : public BST {
	public:
		System::String  ^ str;
		virtual bool bst_op(BST_STREAM ^ stream) override;
	};

	// derive from this and add fields which reg with "this"
	// and add an enum type to implement a union. example:
	//
	//	public ref class MyType3 : public BST_UNION {
	//	public:
	//		enum class msgtype { EIGHT, NINE };
	//		MyType3(void) {
	//			eight.reg(this);
	//			nine.reg(this);
	//		}
	//		BST_STRING  eight;
	//		BST_UINT16_t nine;
	//	};

	public ref class BST_UNION : public BST {
		array<BST^> ^ fields;
	public:
		BST_UNION(void);
		BST_UINT8_t   which;
		virtual bool bst_op(BST_STREAM ^ stream) override;
	};

#endif /*BUILDING_BST_DLL*/

	template <class T>
	public ref class BST_POINTER : public BST {
	public:
		T ^ pointer;
		virtual bool bst_op(BST_STREAM ^ str) override {
			BST_UINT8_t   flag;
			switch (str->get_op()) {
			case BST_OP_ENCODE:
			case BST_OP_CALC_SIZE:
				flag.v = (pointer != nullptr) ? 1 : 0;
				if (!flag.bst_op(str))
					return false;
				if (pointer)
					if (!pointer->bst_op(str))
						return false;
				return true;
			case BST_OP_DECODE:
				if (!flag.bst_op(str))
					return false;
				if (flag.v == 0)
					pointer = nullptr;
				else {
					if (!pointer)
						pointer = gcnew T;
					if (!pointer->bst_op(str))
						return false;
				}
				return true;
			}
			return false;
		}
	};

	template <class T>
	public ref class BST_ARRAY : public BST {
	public:
		array<T^> ^ arr;
		void alloc(int c) {
			int numitems = (arr != nullptr) ? arr->Length : 0;
			if (c == numitems)
				return;
			array<T^> ^ narr = nullptr;
			if (c > 0)
				narr = gcnew array<T^>(c);
			if (c > numitems) {
				if (numitems > 0)
					Array::Copy(arr,0,narr,0,numitems);
				else
					for (int i=0; i < numitems; i++)
						narr[i] = gcnew T;
				for (int i=numitems; i < c; i++)
					narr[i] = gcnew T;
			} else {
				if (c > 0)
					Array::Copy(arr,0,narr,0,c);
			}
			arr = narr;
		}
		virtual bool bst_op(BST_STREAM ^ str) override {
			BST_UINT32_t  count;
			switch (str->get_op()) {
			case BST_OP_ENCODE:
			case BST_OP_CALC_SIZE:
				count.v = arr->Length;
				if (!count.bst_op(str))
					return false;
				for (int i=0; i < arr->Length; i++)
					if (!arr[i]->bst_op(str))
						return false;
				return true;
			case BST_OP_DECODE:
				if (!count.bst_op(str))
					return false;
				alloc(count.v);
				for (int i=0; i < arr->Length; i++)
					if (!arr[i]->bst_op(str))
						return false;
				return true;
			}
			return false;
		}
	};

} // namespace BST_IP