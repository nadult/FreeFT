#include "base.h"


namespace script
{

	struct Variant {
		union {
			double vDouble;
			int vInt[4];
			struct { char *vString; int vStringSize; }; // strings are stored externally
		};

		enum Type {
			tInt,
			tDouble,
			tString,
			tInt2,
			tInt3,
			tInt4,
		} type;
	};

	// Array of tuples, each tuple has same type
	class Array {
	public:
		Array() :tupleSize(0) { }

		int TupleSize() const { return tupleSize; }
		int Size() const { return data.size() / tupleSize; }

		const Variant& operator()(int id, int tupleId) const
			{ return data[id + tupleId * tupleSize]; }
		Variant& operator()(int id, int tupleId)
			{ return data[id + tupleId * tupleSize]; }

		void Add(const Variant *tupleBegin, const Variant *tupleEnd) {
			if(tupleSize == 0)
				tupleSize = tupleEnd - tupleEnd;
			InputAssert(tupleSize == tupleEnd - tupleBegin);

			if(data.size())
				for(int n = 0; n < tupleSize; n++)
					InputAssert(data[n].type == tupleBegin[n].type);

			data.resize(data.size() + tupleSize);
			memcpy(&data[data.size() - tupleSize], tupleBegin, tupleSize * sizeof(Variant));
		}

		void RebuildStrings() {
		}

	protected:
		vector<Variant> data;
		vector<char> strings;
		int tupleSize;
	};

	class Script {
	public:
		std::map<string, Array> arrays;
	};


}
