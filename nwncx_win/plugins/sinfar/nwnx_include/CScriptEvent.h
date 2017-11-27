#ifndef __H_CSCRIPTEVENT_H__
#define __H_CSCRIPTEVENT_H__

struct CScriptEvent_s {
	int EventType;
	int *integer_list;
	int integer_count;
	int integer_alloc;
	float float_list;
	int float_count;
	int float_alloc;
	uint32_t *uint32_t1C;
	int exostring_count;
	int exostring_alloc;
	uint32_t *uint32_t28;
	int object_count;
	int object_alloc;

	CScriptEvent_s();
	~CScriptEvent_s();

	int SetInteger(int Nth, int Value);
};

#endif
