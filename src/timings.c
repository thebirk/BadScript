// Taken from https://github.com/odin-lang/Odin/blob/master/src/timings.cpp and modified.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct TimeStamp {
	long long start;
	long long finish;
	String label;
} TimeStamp;

typedef struct Timings {
	TimeStamp total;
	Array(TimeStamp) sections;
	long long freq;
	double total_time_seconds;
} Timings;

#ifdef _WIN32
long long time_stamp_time_now() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

long long time_stamp_freq() {
	static LARGE_INTEGER win32_perf_count = { 0 };
	if (!win32_perf_count.QuadPart) {
		QueryPerformanceFrequency(&win32_perf_count);
	}
	return win32_perf_count.QuadPart;
}

#else
#	error "Implement timings for system"
#endif

TimeStamp make_time_stamp(String label) {
	TimeStamp ts = { 0 };
	ts.start = time_stamp_time_now();
	ts.label = label;
	return ts;
}

void timings_init(Timings *t, String label) {
	array_init(t->sections, 16);
	t->total = make_time_stamp(label);
	t->freq = time_stamp_freq();
}

void timings_stop_current_section(Timings *t) {
	if (t->sections.size > 0) {
		t->sections.data[t->sections.size - 1].finish = time_stamp_time_now();
	}
}

void timings_start_section(Timings *t, String label) {
	timings_stop_current_section(t);
	array_add(t->sections, make_time_stamp(label));
}

double time_stamp_as_s(TimeStamp *ts, long long freq) {
	return (double)(ts->finish - ts->start) / (double)freq;
}

double time_stamp_as_ms(TimeStamp *ts, long long freq) {
	return 1000.0*time_stamp_as_s(ts, freq);
}

double long time_stamp_as_us(TimeStamp *ts, long long freq) {
	return 1000000.0*time_stamp_as_s(ts, freq);
}

typedef enum TimingUnit {
	TimingUnit_Second,
	TimingUnit_Millisecond,
	TimingUnit_Microsecond,
	TimingUnit_COUNT,
} TimingUnit;

char const *timing_unit_strings[TimingUnit_COUNT] = { "s", "ms", "us" };

double time_stamp(TimeStamp *ts, long long freq, TimingUnit unit) {
	switch (unit) {
	case TimingUnit_Millisecond: return time_stamp_as_ms(ts, freq);
	case TimingUnit_Microsecond: return time_stamp_as_us(ts, freq);
	default: /*fallthrough*/
	case TimingUnit_Second:      return time_stamp_as_s(ts, freq);
	}
}

void timings_print_all(Timings *t, TimingUnit unit) {
	char const SPACES[] = "                                                                ";
	size_t max_len;

	timings_stop_current_section(t);
	t->total.finish = time_stamp_time_now();

	max_len = t->total.label.len;
	max_len = 36;
	TimeStamp *ts;
	for_array_ref(t->sections, ts) {
		max_len = max(max_len, ts->label.len);
	}

	t->total_time_seconds = time_stamp_as_s(&t->total, t->freq);

	double total_time = time_stamp(&t->total, t->freq, unit);

	printf("%.*s%.*s - % 9.3f %s - %6.2f%%\n",
		(int)t->total.label.len, t->total.label.str,
		(int)(max_len - t->total.label.len), SPACES,
		total_time,
		timing_unit_strings[unit],
		(double)100.0);

	for_array_ref(t->sections, ts) {
		double section_time = time_stamp(ts, t->freq, unit);
		printf("%.*s%.*s - % 9.3f %s - %6.2f%%\n",
			(int)ts->label.len, ts->label.str,
			(int)(max_len - ts->label.len), SPACES,
			section_time,
			timing_unit_strings[unit],
			100.0*section_time / total_time);
	}
}