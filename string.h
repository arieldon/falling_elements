#ifndef STRING_H
#define STRING_H

typedef struct string string;
struct string
{
	s32 Length;
	char *Data;
};

#define StringLiteral(String) (string){ .Data = String, .Length = sizeof(String) - 1 }

#endif
