typedef enum {
	VALUE_NULL = 0,
	VALUE_NUMBER,
	VALUE_STRING,
	VALUE_TABLE,
} ValueKind;

typedef struct {
	ValueKind kind;
	union {
		struct {
			double value;
		} number;
		struct {
			String str;
		} string;
		struct {
			void *temp;
		} table;
	};
} Value;