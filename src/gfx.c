#include <SDL2/SDL.h>

struct gfxState {
	bool inited;
	bool should_close;
	SDL_Window *window;
	SDL_Renderer *renderer;
} state = { 
	.inited = false,
	.should_close = false,
};

Value* gfx_init(Ir *ir, ValueArray args) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return make_number_value(ir, 0);
	}
	else {
		state.inited = true;
		return make_number_value(ir, 1);
	}
}

Value* gfx_create_window(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}

	if (args.size != 4) {
		ir_error(ir, "gfx.create_window takes 4 arguements: title, width, height, vsync");
	}
	Value *title = args.data[0];
	Value *width = args.data[1];
	Value *height = args.data[2];
	Value *vsync = args.data[3];

	if (!isstring(title) || !isnumber(width) || !isnumber(height) || !isnumber(vsync)) {
		ir_error(ir, "gfx.create_window takes 4 arguements: title, width, height, vsync");
	}

	//TODO: VSYNC flag
	state.window = SDL_CreateWindow(title->string.str.str, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width->number.value, height->number.value, 0);
	if (!state.window) {
		return make_number_value(ir, 0);
	}

	state.renderer = SDL_CreateRenderer(state.window, -1, SDL_RENDERER_ACCELERATED);
	if (!state.renderer) {
		return make_number_value(ir, 0);
	}

	return make_number_value(ir, 1);
}

Value* gfx_update(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size != 0) {
		ir_error(ir, "gfx.update takes no arguments");
	}

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_WINDOWEVENT: {
			if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
				state.should_close = true;
			}
		} break;
		}
	}

	return null_value;
}

Value* gfx_present(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size != 0) {
		ir_error(ir, "gfx.present takes no arguments");
	}

	SDL_RenderPresent(state.renderer);

	return null_value;
}

Value* gfx_should_close(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size != 0) {
		ir_error(ir, "gfx.should_close takes no arguments");
	}

	return make_number_value(ir, state.should_close ? 1 : 0);
}

Value* gfx_clear(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size == 0) {
		SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 0);
		SDL_RenderClear(state.renderer);
	}
	else if (args.size == 4) {
		if (!isnumber(args.data[0]) || !isnumber(args.data[1]) || !isnumber(args.data[1]) || !isnumber(args.data[1])) {
			ir_error(ir, "gfx.should_close takes either zero arguments or 4 numbers");
		}
		uint8_t r = (uint8_t) args.data[0]->number.value;
		uint8_t g = (uint8_t) args.data[1]->number.value;
		uint8_t b = (uint8_t) args.data[2]->number.value;
		uint8_t a = (uint8_t) args.data[3]->number.value;
		SDL_SetRenderDrawColor(state.renderer, r, g, b, a);
		SDL_RenderClear(state.renderer);
	}
	else {
		ir_error(ir, "gfx.should_close takes either zero arguments or 4 numbers");
	}

	return null_value;
}

Value* gfx_fill_rect(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size != 7) {
		ir_error(ir, "gfx.fill_rect takes 7 arguments: x, y, width, height, r, g, b");
	}

	Value *x      = args.data[0];
	Value *y      = args.data[1];
	Value *width  = args.data[2];
	Value *height = args.data[3];
	Value *r      = args.data[4];
	Value *g      = args.data[5];
	Value *b      = args.data[6];

	if (!isnumber(x) || !isnumber(y) ||
		!isnumber(width) || !isnumber(height) ||
		!isnumber(r) || !isnumber(g) || !isnumber(b)) {
		ir_error(ir, "gfx.fill_rect takes 7 arguments: x, y, width, height, r, g, b");
	}

	SDL_SetRenderDrawColor(state.renderer, r->number.value, g->number.value, b->number.value, 255);
	SDL_RenderFillRect(state.renderer, &(SDL_Rect) {x->number.value, y->number.value, width->number.value, height->number.value});

	return null_value;
}

void import_gfx(Ir *ir) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_TABLE;
	table_put_name(ir, v, make_string_slow("init"), make_native_function(ir, gfx_init));
	table_put_name(ir, v, make_string_slow("create_window"), make_native_function(ir, gfx_create_window));
	table_put_name(ir, v, make_string_slow("update"), make_native_function(ir, gfx_update));
	table_put_name(ir, v, make_string_slow("should_close"), make_native_function(ir, gfx_should_close));
	table_put_name(ir, v, make_string_slow("clear"), make_native_function(ir, gfx_clear));
	table_put_name(ir, v, make_string_slow("present"), make_native_function(ir, gfx_present));
	table_put_name(ir, v, make_string_slow("fill_rect"), make_native_function(ir, gfx_fill_rect));

	scope_add(ir, ir->global_scope, make_string_slow("gfx"), v);
}