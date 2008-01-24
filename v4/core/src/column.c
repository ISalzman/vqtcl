/*
 * column.c - Implementation of sequences, columns, and items.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intern.h"
#include "wrap_gen.h"

ItemTypes DebugCmd_I (Item_p a) {
	Shared_p sh = GetShared();
	a->i = sh->debug ^= a[0].i;
	return IT_int;
}

void *AdjustSeqRefs (void *refs, int count) {
	/*if (count > 0)*/
	if (refs != NULL) {
		Seq_p ptr = refs;
		ptr->refs += count;
		if (ptr->refs <= 0) {
			if (ptr->type != NULL) {
				int i, f;
				
				if (ptr->type->cleaner != NULL)
					ptr->type->cleaner(ptr);

				for (i = 0, f = ptr->type->cleanups; f > 0; ++i, f >>= 3)
					if (f & 1)
						DecRefCount(ptr->data[i].p);
					else if (f & 2)
						free(ptr->data[i].p);
			}
			free(refs);
			refs = NULL;
		}
	}
	return refs;
}

Seq_p LoseRef (Seq_p seq) {
	ADD_PTR_TO_BUF(*GetShared()->keeps, seq);
	return seq;
}

Column SeqAsCol (Seq_p seq) {
	Column result;
	result.seq = seq;
	result.pos = -1;
	return result;
}

Seq_p NewSequenceNoRef (int count, SeqType_p type, int bytes) {
	Seq_p seq;
	
	seq = calloc(1, sizeof(struct Sequence) + bytes);
	seq->count = count;
	seq->type = type;
	seq->getter = type->getfun;

	/* default for data[0].p is to point to the aux bytes after the sequence */
	/* default for data[1].i is to contain number of extra bytes allocated */
	seq->data[0].p = seq + 1;
	seq->data[1].i = bytes;
	
	/* make sure 64-bit values have a "natural" 8-byte alignment */
	Assert(((Int_t) seq->data[0].p) % 8 == 0);

	return seq;
}

Seq_p NewSequence (int count, SeqType_p type, int bytes) {
	return KeepRef(NewSequenceNoRef(count, type, bytes));
}

void ReleaseSequences (Buffer_p keep) {
	int cnt;
	char *ptr = NULL;
	
	while (NextBuffer(keep, &ptr, &cnt)) {
		Seq_p *refs = (Seq_p*) ptr;
		int count = cnt / sizeof(Seq_p);

		while (--count >= 0)
			DecRefCount(refs[count]);
	}
}

/* TODO: not used yet, should review refcount scenarios, might be wrong now */
Seq_p ResizeSeq (Seq_p seq, int pos, int diff, int elemsize) {
	int newcnt, olimit, nlimit = 0, bytes;
	Seq_p result = seq;
	
	Assert(seq->refs == 1);
	
	newcnt = seq->count + diff;
	bytes = seq->data[1].i;
	olimit = bytes / elemsize;
	
	if (diff > 0) { /* grow when limit is reached */
		if (newcnt > olimit) {
			nlimit = (olimit / 2) * 3 + 4;
			if (nlimit < newcnt)
				nlimit = newcnt + 4;
		}
	} else { /* shrink when less than half full */
		if (newcnt < olimit / 2) {
			nlimit = newcnt + 2;
			bytes = seq->count * elemsize;
		}
	}

	if (nlimit > 0) {
		result = IncRefCount(NewSequence(seq->count, seq->type,
		                                                nlimit * elemsize));
		result->getter = seq->getter;
		result->data[2] = seq->data[2];
		result->data[3] = seq->data[3];
		memcpy(result+1, seq+1, pos * elemsize);

		seq->type = NULL; /* release and prevent from calling cleaner */
		/*LoseRef(seq);*/
	}

	if (diff > 0) {
		memmove((char*) (result+1) + (pos + diff) * elemsize,
						(char*) (seq+1) + pos * elemsize,
						(seq->count - pos) * elemsize);
		memset((char*) (result+1) + pos * elemsize, 0, diff * elemsize);
	} else
		memmove((char*) (result+1) + pos * elemsize,
						(char*) (seq+1) + (pos - diff) * elemsize,
						(seq->count - (pos - diff)) * elemsize);
	
	result->count += diff;
	return result;
}

ItemTypes ResizeColCmd_iII (Item_p a) {
	int r, *data;
	Seq_p seq, result;
	
	seq = a[0].c.seq;
	result = NewIntVec(seq->count, &data);
	for (r = 0; r < seq->count; ++r)
		data[r] = GetSeqItem(r, seq, IT_int).i;
		
	a->c.seq = ResizeSeq(result, a[1].i, a[2].i, sizeof(int));
	return IT_column;
}

Seq_p NewBitVec (int count) {
	return NewSequence(count, PickIntGetter(1), (count + 7) / 8);
}

Seq_p NewIntVec (int count, int **dataptr) {
	const int w = sizeof(int);
	Seq_p seq = NewSequence(count, PickIntGetter(8 * w), w * count);
	if (dataptr != NULL)
		*dataptr = seq->data[0].p;
	return seq;
}

Seq_p NewPtrVec (int count, Int_t **dataptr) {
	const int w = sizeof(Int_t);
	Seq_p result = NewSequence(count, PickIntGetter(8 * w), w * count);
	if (dataptr != NULL)
		*dataptr = result->data[0].p;
	return result;
}

ItemTypes GetItem (int row, Item_p item) {
	Seq_p seq = item->c.seq;
	
	if (row < 0 || row >= seq->count) {
		item->e = EC_rioor;
		return IT_error;
	}
		
	return seq->getter(row, item);
}

Item GetColItem (int row, Column column, ItemTypes type) {
	Item item;
	ItemTypes got;
	
	item.c = column;
	got = GetItem(row, &item);
	Assert(got == type);
	return item;
}

Item GetSeqItem (int row, Seq_p seq, ItemTypes type) {
	Item item;
	ItemTypes got;
	
	item.c.seq = seq;
	item.c.pos = -1;
	got = GetItem(row, &item);
	Assert(got == type);
	return item;
}

Item GetViewItem (View_p view, int row, int col, ItemTypes type) {
	Item item;
	ItemTypes got;
	
	item.c = ViewCol(view, col);
	got = GetItem(row, &item);
	Assert(got == type);
	return item;
}

#if 0 /* unused */
const char *ItemTypeAsString (ItemTypes type) {
	static const char *typeTable[] = {
		"",	 /* IT_unknown */
		"I", /* IT_int */
		"L", /* IT_wide */
		"D", /* IT_double */
		"S", /* IT_string */
		"O", /* IT_object */
		"C", /* IT_column */
		"V", /* IT_view */
		"E", /* IT_error */
	};

	return typeTable[type];
}
#endif

ItemTypes CharAsItemType (char type) {
	switch (type) {
		case 0:		return IT_unknown;
		case 'I': return IT_int;
		case 'L': return IT_wide;
		case 'F': return IT_float;
		case 'D': return IT_double;
		case 'S': return IT_string;
		case 'B': return IT_bytes;
		case 'O': return IT_object;
		case 'C': return IT_column;
		case 'V': return IT_view;
	}
	return IT_error;
}

static ItemTypes StringGetter (int row, Item_p item) {
	char **ptrs = (char**) item->c.seq->data[0].p;
	item->s = ptrs[row];
	return IT_string;
}

static struct SeqType ST_String = { "string", StringGetter, 0102 };

static ItemTypes BytesGetter (int row, Item_p item) {
	char **ptrs = (char**) item->c.seq->data[0].p;
	item->u.ptr = (const Byte_t*) ptrs[row];
	item->u.len = ptrs[row+1] - ptrs[row];
	return IT_bytes;
}

static struct SeqType ST_Bytes = { "string", BytesGetter, 0102 };

Seq_p NewStrVec (int istext) {
	Seq_p seq;
	Buffer_p bufs;

	bufs = malloc(2 * sizeof(struct Buffer));
	InitBuffer(bufs);
	InitBuffer(bufs+1);

	seq = NewSequence(0, istext ? &ST_String : &ST_Bytes, 0);
	/* data[0] starts as two buffers, then becomes vector of string pointers */
	/* data[1] is not used */
	/* data[2] is an optional pointer to hash map sequence, see StringLookup */
	seq->data[0].p = bufs;
	
	return seq;
}

void AppendToStrVec (const void *string, int bytes, Seq_p seq) {
	Buffer_p bufs = seq->data[0].p;

	if (bytes < 0)
		bytes = strlen((const char*) string) + 1;
	
	/* TODO: consider tracking pointers i.s.o. making actual copies here */
	AddToBuffer(bufs, string, bytes);
	ADD_INT_TO_BUF(bufs[1], bytes);
	
	++seq->count;
}

Seq_p FinishStrVec (Seq_p seq) {
	int r = 0, cnt = 0, *iptr = NULL;
	char *fill, **ptrs, *cptr = NULL;
	Buffer_p bufs = seq->data[0].p;

	ptrs = malloc((seq->count+1) * sizeof(char*) + BufferFill(bufs));

	fill = (char*) (ptrs + seq->count + 1);
	while (NextBuffer(bufs+1, (void*) &iptr, &cnt)) {
		int i, n = cnt / sizeof(int);
		for (i = 0; i < n; ++i) {
			ptrs[r++] = fill;
			fill += iptr[i];
		}
	}
	ptrs[r] = fill; /* past-end mark needed to provide length of last entry */

	fill = (char*) (ptrs + seq->count + 1);
	while (NextBuffer(bufs, &cptr, &cnt)) {
		memcpy(fill, cptr, cnt);
		fill += cnt;
	}
	
	ReleaseBuffer(bufs, 0);
	ReleaseBuffer(bufs+1, 0);
	free(seq->data[0].p);
	
	seq->data[0].p = ptrs;
	return seq;
}

Column ForceStringColumn (Column column) {
	int r, rows;
	Seq_p result;
	
	/* this code needed to always make name column of meta views string cols */
	
	if (column.seq == NULL || column.seq->getter == StringGetter)
		return column;
		
	rows = column.seq->count;
	result = NewStrVec(1);
	
	for (r = 0; r < rows; ++r)
		AppendToStrVec(GetColItem(r, column, IT_string).s, -1, result);
	
	return SeqAsCol(FinishStrVec(result));
}

static ItemTypes IotaGetter (int row, Item_p item) {
	item->i = row;
	return IT_int;
}

static struct SeqType ST_Iota = { "iota", IotaGetter };

Column NewIotaColumn (int count) {
	return SeqAsCol(NewSequence(count, &ST_Iota, 0));
}

static void SequenceCleaner (Seq_p seq) {
	int i;
	View_p *items = seq->data[0].p;

	for (i = 0; i < seq->count; ++i)
		DecRefCount(items[i]);
}

static ItemTypes SequenceGetter (int row, Item_p item) {
	void **items = (void**) item->c.seq->data[0].p;
	ItemTypes type = item->c.seq->data[1].i;

	switch (type) {

		case IT_view:
			item->v = items[row];
			return IT_view;

		case IT_column:
			item->c.seq = items[row];
			item->c.pos = -1;
			return IT_column;

		default:
			return IT_unknown;
	}
}

static struct SeqType ST_Sequence = {
	"sequence", SequenceGetter, 0, SequenceCleaner
};

Seq_p NewSeqVec (ItemTypes type, const Seq_p *items, int count) {
	int i, bytes;
	Seq_p seq;

	bytes = count * sizeof(View_p);
	seq = NewSequence(count, &ST_Sequence, bytes);
	/* data[0] points to a list of pointers */
	/* data[1] is the type of the returned items */
	seq->data[1].i = type;

	if (items != NULL) {
		memcpy(seq->data[0].p, items, bytes);
		for (i = 0; i < count; ++i)
			IncRefCount(items[i]);
	}

	return seq;
}

static ItemTypes CountsGetter (int row, Item_p item) {
	const View_p *items = (const View_p*) item->c.seq->data[0].p;

	item->i = ViewSize(items[row]);
	return IT_int;
}

Column NewCountsColumn (Column column) {
	int r, rows;
	View_p *items;
	Seq_p seq;
	
	rows = column.seq->count;	 
	seq = NewSeqVec(IT_view, NULL, rows);
	seq->getter = CountsGetter;

	items = seq->data[0].p;
	for (r = 0; r < rows; ++r)
		items[r] = IncRefCount(GetColItem(r, column, IT_view).v);

	return SeqAsCol(seq);
}

Column OmitColumn (Column omit, int size) {
	int i, *data, j = 0, outsize;
	Seq_p seq, map;
	
	if (omit.seq == NULL)
		return omit;
		
	outsize = size - omit.seq->count;
	
	/* FIXME: wasteful for large views, consider sorting input col instead */
	map = NewBitVec(size);
	for (i = 0; i < omit.seq->count; ++i)
		SetBit(&map, GetColItem(i, omit, IT_int).i);

	seq = NewIntVec(outsize, &data);
	
	for (i = 0; i < size; ++i)
		if (!TestBit(map, i)) {
			Assert(j < outsize);
			data[j++] = i;
		}
		
	return SeqAsCol(seq);
}

#if 0
static ItemTypes UnknownGetter (int row, Item_p item) {
	return IT_unknown;
}

static struct SeqType ST_Unknown = { "unknown", UnknownGetter };

static Seq_p NewUnknownVec (int count) {
	return NewSequence(count, &ST_Unknown, 0);
}
#endif

Column CoerceColumn (ItemTypes type, Object_p obj) {
	int i, n;
	void *data;
	Seq_p seq;
	Item item;
	Column out, column = ObjAsColumn(obj);

	if (column.seq == NULL)
		return column;
		
	/* determine the input column type by fetching one item */
	item.c = column;
	n = column.seq->count;

	if (GetItem(0, &item) == type)
		return column;
		
	switch (type) {
		case IT_int:	seq = NewIntVec(n, (void*) &data); break;
		case IT_wide:	seq = NewSequence(n, PickIntGetter(64), 8 * n); break;
		case IT_float:	seq = NewSequence(n, FixedGetter(4,1,1,0), 4 * n);
		                break;
		case IT_double: seq = NewSequence(n, FixedGetter(8,1,1,0), 8 * n);
		                break;
		case IT_string: seq = NewStrVec(1); break;
		case IT_bytes:	seq = NewStrVec(0); break;
		case IT_view:	seq = NewSeqVec(IT_view, NULL, n); break;
		default:		Assert(0);
	}

	out = SeqAsCol(seq);
	data = out.seq->data[0].p;

	for (i = 0; i < n; ++i) {
		item.o = GetColItem(i, column, IT_object).o;
		if (!ObjToItem(type, &item)) {
			out.seq = NULL;
			out.pos = i;
			break;
		}
	
		switch (type) {
			case IT_int:	((int*) data)[i] = item.i; break;
			case IT_wide:	((int64_t*) data)[i] = item.w; break;
			case IT_float:	((float*) data)[i] = item.f; break;
			case IT_double: ((double*) data)[i] = item.d; break;
			case IT_string: AppendToStrVec(item.s, -1, out.seq); break;
			case IT_bytes:	AppendToStrVec(item.u.ptr, item.u.len, out.seq);
			                break;
			case IT_view:	((Seq_p*) data)[i] = IncRefCount(item.v); break;
			default:		Assert(0);
		}
	}

	if (type == IT_string || type == IT_bytes)
		FinishStrVec(seq);

	return out;
}

Column CoerceCmd (Object_p obj, const char *str) {
	return CoerceColumn(CharAsItemType(str[0]), obj);
}

int CastObjToItem (char type, Item_p item) {
	switch (type) {

		case 'M':
			item->o = NeedMutable(item->o);
			return (void*) item->o != NULL;
			
		case 'N':
			item->i = ColumnByName(V_Meta(item[-1].v), item->o);
			return item->i >= 0;
		
		case 'n': {
			int *data, r, rows;
			View_p meta;
			Column column;
			Seq_p seq;

			column = ObjAsColumn(item->o);
			if (column.seq == NULL)
				return 0;
				
			rows = column.seq->count;
			seq = NewIntVec(rows, &data);
			meta = V_Meta(item[-1].v);
			
			for (r = 0; r < rows; ++r) {
				/* FIXME: this only works if input is a list, not a column! */
				data[r] = ColumnByName(meta, GetColItem(r, column,
				                                            IT_object).o);
				if (data[r] < 0)
					return 0;
			}
			
			item->c = SeqAsCol(seq);
			break;
		}
		
		default:
			if (type < 'a' || type > 'z')
				return ObjToItem(CharAsItemType(type), item);

			item->c = CoerceColumn(CharAsItemType(type +'A'-'a'), item->o);
			break;
	}

	return 1; /* cast succeeded */
}
