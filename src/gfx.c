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

Value* gfx_create_texture(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size != 1) {
		ir_error(ir, "gfx.create_texture takes one arguments: path");
	}
	if (!isstring(args.data[0])) {
		ir_error(ir, "gfx.create_texture takes one arguments: path");
	}

	SDL_Surface *image_surface = SDL_LoadBMP(args.data[0]->string.str.str);
	if (!image_surface) {
		printf("Failed to load surface\n");
		return null_value;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(state.renderer, image_surface);
	if (!texture) {
		SDL_FreeSurface(image_surface);
		return null_value;
	}
	SDL_FreeSurface(image_surface);

	Value *t = alloc_value(ir);
	t->kind = VALUE_TABLE;

	table_put_name(ir, t, make_string_slow("width"), make_number_value(ir, image_surface->w));
	table_put_name(ir, t, make_string_slow("height"), make_number_value(ir, image_surface->h));

	Value *data = alloc_value(ir);
	data->kind = VALUE_USERDATA;
	data->userdata.data = texture;
	table_put_name(ir, t, make_string_slow("data"), data);

	return t;
}

Value* gfx_draw_texture(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	//TODO: Handle width,height args
	if (args.size != 3) {
		ir_error(ir, "gfx.draw_texture takes three arguments: texture, x, y");
	}
	Value *tex = args.data[0];
	Value *x = args.data[1];
	Value *y = args.data[2];

	if (!istable(tex) || !isnumber(x) || !isnumber(y)) {
		ir_error(ir, "gfx.draw_texture takes three arguments: texture, x, y");
	}

	Value *data = table_get_name(ir, tex, string("data"));
	int w, h;
	SDL_QueryTexture(data->userdata.data, 0, 0, &w, &h);
	SDL_RenderCopy(state.renderer, data->userdata.data, 0, &(SDL_Rect){.x = x->number.value, .y = y->number.value, .w = w, .h = h});

	return null_value;
}

Value* gfx_get_key_state(Ir *ir, ValueArray args) {
	if (!state.inited) {
		ir_error(ir, "gfx.init has to be called before any other gfx function!");
	}
	if (args.size != 1) {
		ir_error(ir, "gfx.get_key_state takes one arguments: key");
	}
	if (!isnumber(args.data[0])) {
		ir_error(ir, "gfx.get_key_state takes one arguments: key");
	}

	SDL_Scancode sc = SDL_GetScancodeFromKey(args.data[0]->number.value);
	const uint8_t *key_states = SDL_GetKeyboardState(0);

	if (key_states[sc]) {
		return make_number_value(ir, 1);
	}
	else {
		return make_number_value(ir, 0);
	}
}

void gfx_add_key_names(Ir *ir, Value *t);
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
	table_put_name(ir, v, make_string_slow("create_texture"), make_native_function(ir, gfx_create_texture));
	table_put_name(ir, v, make_string_slow("draw_texture"), make_native_function(ir, gfx_draw_texture));
	table_put_name(ir, v, make_string_slow("get_key_state"), make_native_function(ir, gfx_get_key_state));

	gfx_add_key_names(ir, v);

	scope_add(ir, ir->global_scope, make_string_slow("gfx"), v);
}


void gfx_add_key_names(Ir *ir, Value *t) {
	table_put_name(ir, t, make_string_slow("KEY_UNKNOWN"), make_number_value(ir, SDLK_UNKNOWN));
	table_put_name(ir, t, make_string_slow("KEY_RETURN"), make_number_value(ir, SDLK_RETURN));
	table_put_name(ir, t, make_string_slow("KEY_ESCAPE"), make_number_value(ir, SDLK_ESCAPE));
	table_put_name(ir, t, make_string_slow("KEY_BACKSPACE"), make_number_value(ir, SDLK_BACKSPACE));
	table_put_name(ir, t, make_string_slow("KEY_TAB"), make_number_value(ir, SDLK_TAB));
	table_put_name(ir, t, make_string_slow("KEY_SPACE"), make_number_value(ir, SDLK_SPACE));
	table_put_name(ir, t, make_string_slow("KEY_EXCLAIM"), make_number_value(ir, SDLK_EXCLAIM));
	table_put_name(ir, t, make_string_slow("KEY_QUOTEDBL"), make_number_value(ir, SDLK_QUOTEDBL));
	table_put_name(ir, t, make_string_slow("KEY_HASH"), make_number_value(ir, SDLK_HASH));
	table_put_name(ir, t, make_string_slow("KEY_PERCENT"), make_number_value(ir, SDLK_PERCENT));
	table_put_name(ir, t, make_string_slow("KEY_DOLLAR"), make_number_value(ir, SDLK_DOLLAR));
	table_put_name(ir, t, make_string_slow("KEY_AMPERSAND"), make_number_value(ir, SDLK_AMPERSAND));
	table_put_name(ir, t, make_string_slow("KEY_QUOTE"), make_number_value(ir, SDLK_QUOTE));
	table_put_name(ir, t, make_string_slow("KEY_LEFTPAREN"), make_number_value(ir, SDLK_LEFTPAREN));
	table_put_name(ir, t, make_string_slow("KEY_RIGHTPAREN"), make_number_value(ir, SDLK_RIGHTPAREN));
	table_put_name(ir, t, make_string_slow("KEY_ASTERISK"), make_number_value(ir, SDLK_ASTERISK));
	table_put_name(ir, t, make_string_slow("KEY_PLUS"), make_number_value(ir, SDLK_PLUS));
	table_put_name(ir, t, make_string_slow("KEY_COMMA"), make_number_value(ir, SDLK_COMMA));
	table_put_name(ir, t, make_string_slow("KEY_MINUS"), make_number_value(ir, SDLK_MINUS));
	table_put_name(ir, t, make_string_slow("KEY_PERIOD"), make_number_value(ir, SDLK_PERIOD));
	table_put_name(ir, t, make_string_slow("KEY_SLASH"), make_number_value(ir, SDLK_SLASH));
	table_put_name(ir, t, make_string_slow("KEY_0"), make_number_value(ir, SDLK_0));
	table_put_name(ir, t, make_string_slow("KEY_1"), make_number_value(ir, SDLK_1));
	table_put_name(ir, t, make_string_slow("KEY_2"), make_number_value(ir, SDLK_2));
	table_put_name(ir, t, make_string_slow("KEY_3"), make_number_value(ir, SDLK_3));
	table_put_name(ir, t, make_string_slow("KEY_4"), make_number_value(ir, SDLK_4));
	table_put_name(ir, t, make_string_slow("KEY_5"), make_number_value(ir, SDLK_5));
	table_put_name(ir, t, make_string_slow("KEY_6"), make_number_value(ir, SDLK_6));
	table_put_name(ir, t, make_string_slow("KEY_7"), make_number_value(ir, SDLK_7));
	table_put_name(ir, t, make_string_slow("KEY_8"), make_number_value(ir, SDLK_8));
	table_put_name(ir, t, make_string_slow("KEY_9"), make_number_value(ir, SDLK_9));
	table_put_name(ir, t, make_string_slow("KEY_COLON"), make_number_value(ir, SDLK_COLON));
	table_put_name(ir, t, make_string_slow("KEY_SEMICOLON"), make_number_value(ir, SDLK_SEMICOLON));
	table_put_name(ir, t, make_string_slow("KEY_LESS"), make_number_value(ir, SDLK_LESS));
	table_put_name(ir, t, make_string_slow("KEY_EQUALS"), make_number_value(ir, SDLK_EQUALS));
	table_put_name(ir, t, make_string_slow("KEY_GREATER"), make_number_value(ir, SDLK_GREATER));
	table_put_name(ir, t, make_string_slow("KEY_QUESTION"), make_number_value(ir, SDLK_QUESTION));
	table_put_name(ir, t, make_string_slow("KEY_AT"), make_number_value(ir, SDLK_AT));
	table_put_name(ir, t, make_string_slow("KEY_LEFTBRACKET"), make_number_value(ir, SDLK_LEFTBRACKET));
	table_put_name(ir, t, make_string_slow("KEY_BACKSLASH"), make_number_value(ir, SDLK_BACKSLASH));
	table_put_name(ir, t, make_string_slow("KEY_RIGHTBRACKET"), make_number_value(ir, SDLK_RIGHTBRACKET));
	table_put_name(ir, t, make_string_slow("KEY_CARET"), make_number_value(ir, SDLK_CARET));
	table_put_name(ir, t, make_string_slow("KEY_UNDERSCORE"), make_number_value(ir, SDLK_UNDERSCORE));
	table_put_name(ir, t, make_string_slow("KEY_BACKQUOTE"), make_number_value(ir, SDLK_BACKQUOTE));
	table_put_name(ir, t, make_string_slow("KEY_A"), make_number_value(ir, SDLK_a));
	table_put_name(ir, t, make_string_slow("KEY_B"), make_number_value(ir, SDLK_b));
	table_put_name(ir, t, make_string_slow("KEY_C"), make_number_value(ir, SDLK_c));
	table_put_name(ir, t, make_string_slow("KEY_D"), make_number_value(ir, SDLK_d));
	table_put_name(ir, t, make_string_slow("KEY_E"), make_number_value(ir, SDLK_e));
	table_put_name(ir, t, make_string_slow("KEY_F"), make_number_value(ir, SDLK_f));
	table_put_name(ir, t, make_string_slow("KEY_G"), make_number_value(ir, SDLK_g));
	table_put_name(ir, t, make_string_slow("KEY_H"), make_number_value(ir, SDLK_h));
	table_put_name(ir, t, make_string_slow("KEY_I"), make_number_value(ir, SDLK_i));
	table_put_name(ir, t, make_string_slow("KEY_J"), make_number_value(ir, SDLK_j));
	table_put_name(ir, t, make_string_slow("KEY_K"), make_number_value(ir, SDLK_k));
	table_put_name(ir, t, make_string_slow("KEY_L"), make_number_value(ir, SDLK_l));
	table_put_name(ir, t, make_string_slow("KEY_M"), make_number_value(ir, SDLK_m));
	table_put_name(ir, t, make_string_slow("KEY_N"), make_number_value(ir, SDLK_n));
	table_put_name(ir, t, make_string_slow("KEY_O"), make_number_value(ir, SDLK_o));
	table_put_name(ir, t, make_string_slow("KEY_P"), make_number_value(ir, SDLK_p));
	table_put_name(ir, t, make_string_slow("KEY_Q"), make_number_value(ir, SDLK_q));
	table_put_name(ir, t, make_string_slow("KEY_R"), make_number_value(ir, SDLK_r));
	table_put_name(ir, t, make_string_slow("KEY_S"), make_number_value(ir, SDLK_s));
	table_put_name(ir, t, make_string_slow("KEY_T"), make_number_value(ir, SDLK_t));
	table_put_name(ir, t, make_string_slow("KEY_U"), make_number_value(ir, SDLK_u));
	table_put_name(ir, t, make_string_slow("KEY_V"), make_number_value(ir, SDLK_v));
	table_put_name(ir, t, make_string_slow("KEY_W"), make_number_value(ir, SDLK_w));
	table_put_name(ir, t, make_string_slow("KEY_X"), make_number_value(ir, SDLK_x));
	table_put_name(ir, t, make_string_slow("KEY_Y"), make_number_value(ir, SDLK_y));
	table_put_name(ir, t, make_string_slow("KEY_Z"), make_number_value(ir, SDLK_z));
	table_put_name(ir, t, make_string_slow("KEY_CAPSLOCK"), make_number_value(ir, SDLK_CAPSLOCK));
	table_put_name(ir, t, make_string_slow("KEY_F1"), make_number_value(ir, SDLK_F1));
	table_put_name(ir, t, make_string_slow("KEY_F2"), make_number_value(ir, SDLK_F2));
	table_put_name(ir, t, make_string_slow("KEY_F3"), make_number_value(ir, SDLK_F3));
	table_put_name(ir, t, make_string_slow("KEY_F4"), make_number_value(ir, SDLK_F4));
	table_put_name(ir, t, make_string_slow("KEY_F5"), make_number_value(ir, SDLK_F5));
	table_put_name(ir, t, make_string_slow("KEY_F6"), make_number_value(ir, SDLK_F6));
	table_put_name(ir, t, make_string_slow("KEY_F7"), make_number_value(ir, SDLK_F7));
	table_put_name(ir, t, make_string_slow("KEY_F8"), make_number_value(ir, SDLK_F8));
	table_put_name(ir, t, make_string_slow("KEY_F9"), make_number_value(ir, SDLK_F9));
	table_put_name(ir, t, make_string_slow("KEY_F10"), make_number_value(ir, SDLK_F10));
	table_put_name(ir, t, make_string_slow("KEY_F11"), make_number_value(ir, SDLK_F11));
	table_put_name(ir, t, make_string_slow("KEY_F12"), make_number_value(ir, SDLK_F12));
	table_put_name(ir, t, make_string_slow("KEY_PRINTSCREEN"), make_number_value(ir, SDLK_PRINTSCREEN));
	table_put_name(ir, t, make_string_slow("KEY_SCROLLLOCK"), make_number_value(ir, SDLK_SCROLLLOCK));
	table_put_name(ir, t, make_string_slow("KEY_PAUSE"), make_number_value(ir, SDLK_PAUSE));
	table_put_name(ir, t, make_string_slow("KEY_INSERT"), make_number_value(ir, SDLK_INSERT));
	table_put_name(ir, t, make_string_slow("KEY_HOME"), make_number_value(ir, SDLK_HOME));
	table_put_name(ir, t, make_string_slow("KEY_PAGEUP"), make_number_value(ir, SDLK_PAGEUP));
	table_put_name(ir, t, make_string_slow("KEY_DELETE"), make_number_value(ir, SDLK_DELETE));
	table_put_name(ir, t, make_string_slow("KEY_END"), make_number_value(ir, SDLK_END));
	table_put_name(ir, t, make_string_slow("KEY_PAGEDOWN"), make_number_value(ir, SDLK_PAGEDOWN));
	table_put_name(ir, t, make_string_slow("KEY_RIGHT"), make_number_value(ir, SDLK_RIGHT));
	table_put_name(ir, t, make_string_slow("KEY_LEFT"), make_number_value(ir, SDLK_LEFT));
	table_put_name(ir, t, make_string_slow("KEY_DOWN"), make_number_value(ir, SDLK_DOWN));
	table_put_name(ir, t, make_string_slow("KEY_UP"), make_number_value(ir, SDLK_UP));
	table_put_name(ir, t, make_string_slow("KEY_NUMLOCKCLEAR"), make_number_value(ir, SDLK_NUMLOCKCLEAR));
	table_put_name(ir, t, make_string_slow("KEY_KP_DIVIDE"), make_number_value(ir, SDLK_KP_DIVIDE));
	table_put_name(ir, t, make_string_slow("KEY_KP_MULTIPLY"), make_number_value(ir, SDLK_KP_MULTIPLY));
	table_put_name(ir, t, make_string_slow("KEY_KP_MINUS"), make_number_value(ir, SDLK_KP_MINUS));
	table_put_name(ir, t, make_string_slow("KEY_KP_PLUS"), make_number_value(ir, SDLK_KP_PLUS));
	table_put_name(ir, t, make_string_slow("KEY_KP_ENTER"), make_number_value(ir, SDLK_KP_ENTER));
	table_put_name(ir, t, make_string_slow("KEY_KP_1"), make_number_value(ir, SDLK_KP_1));
	table_put_name(ir, t, make_string_slow("KEY_KP_2"), make_number_value(ir, SDLK_KP_2));
	table_put_name(ir, t, make_string_slow("KEY_KP_3"), make_number_value(ir, SDLK_KP_3));
	table_put_name(ir, t, make_string_slow("KEY_KP_4"), make_number_value(ir, SDLK_KP_4));
	table_put_name(ir, t, make_string_slow("KEY_KP_5"), make_number_value(ir, SDLK_KP_5));
	table_put_name(ir, t, make_string_slow("KEY_KP_6"), make_number_value(ir, SDLK_KP_6));
	table_put_name(ir, t, make_string_slow("KEY_KP_7"), make_number_value(ir, SDLK_KP_7));
	table_put_name(ir, t, make_string_slow("KEY_KP_8"), make_number_value(ir, SDLK_KP_8));
	table_put_name(ir, t, make_string_slow("KEY_KP_9"), make_number_value(ir, SDLK_KP_9));
	table_put_name(ir, t, make_string_slow("KEY_KP_0"), make_number_value(ir, SDLK_KP_0));
	table_put_name(ir, t, make_string_slow("KEY_KP_PERIOD"), make_number_value(ir, SDLK_KP_PERIOD));
	table_put_name(ir, t, make_string_slow("KEY_APPLICATION"), make_number_value(ir, SDLK_APPLICATION));
	table_put_name(ir, t, make_string_slow("KEY_POWER"), make_number_value(ir, SDLK_POWER));
	table_put_name(ir, t, make_string_slow("KEY_KP_EQUALS"), make_number_value(ir, SDLK_KP_EQUALS));
	table_put_name(ir, t, make_string_slow("KEY_F13"), make_number_value(ir, SDLK_F13));
	table_put_name(ir, t, make_string_slow("KEY_F14"), make_number_value(ir, SDLK_F14));
	table_put_name(ir, t, make_string_slow("KEY_F15"), make_number_value(ir, SDLK_F15));
	table_put_name(ir, t, make_string_slow("KEY_F16"), make_number_value(ir, SDLK_F16));
	table_put_name(ir, t, make_string_slow("KEY_F17"), make_number_value(ir, SDLK_F17));
	table_put_name(ir, t, make_string_slow("KEY_F18"), make_number_value(ir, SDLK_F18));
	table_put_name(ir, t, make_string_slow("KEY_F19"), make_number_value(ir, SDLK_F19));
	table_put_name(ir, t, make_string_slow("KEY_F20"), make_number_value(ir, SDLK_F20));
	table_put_name(ir, t, make_string_slow("KEY_F21"), make_number_value(ir, SDLK_F21));
	table_put_name(ir, t, make_string_slow("KEY_F22"), make_number_value(ir, SDLK_F22));
	table_put_name(ir, t, make_string_slow("KEY_F23"), make_number_value(ir, SDLK_F23));
	table_put_name(ir, t, make_string_slow("KEY_F24"), make_number_value(ir, SDLK_F24));
	table_put_name(ir, t, make_string_slow("KEY_EXECUTE"), make_number_value(ir, SDLK_EXECUTE));
	table_put_name(ir, t, make_string_slow("KEY_HELP"), make_number_value(ir, SDLK_HELP));
	table_put_name(ir, t, make_string_slow("KEY_MENU"), make_number_value(ir, SDLK_MENU));
	table_put_name(ir, t, make_string_slow("KEY_SELECT"), make_number_value(ir, SDLK_SELECT));
	table_put_name(ir, t, make_string_slow("KEY_STOP"), make_number_value(ir, SDLK_STOP));
	table_put_name(ir, t, make_string_slow("KEY_AGAIN"), make_number_value(ir, SDLK_AGAIN));
	table_put_name(ir, t, make_string_slow("KEY_UNDO"), make_number_value(ir, SDLK_UNDO));
	table_put_name(ir, t, make_string_slow("KEY_CUT"), make_number_value(ir, SDLK_CUT));
	table_put_name(ir, t, make_string_slow("KEY_COPY"), make_number_value(ir, SDLK_COPY));
	table_put_name(ir, t, make_string_slow("KEY_PASTE"), make_number_value(ir, SDLK_PASTE));
	table_put_name(ir, t, make_string_slow("KEY_FIND"), make_number_value(ir, SDLK_FIND));
	table_put_name(ir, t, make_string_slow("KEY_MUTE"), make_number_value(ir, SDLK_MUTE));
	table_put_name(ir, t, make_string_slow("KEY_VOLUMEUP"), make_number_value(ir, SDLK_VOLUMEUP));
	table_put_name(ir, t, make_string_slow("KEY_VOLUMEDOWN"), make_number_value(ir, SDLK_VOLUMEDOWN));
	table_put_name(ir, t, make_string_slow("KEY_KP_COMMA"), make_number_value(ir, SDLK_KP_COMMA));
	table_put_name(ir, t, make_string_slow("KEY_KP_EQUALSAS400"), make_number_value(ir, SDLK_KP_EQUALSAS400));
	table_put_name(ir, t, make_string_slow("KEY_ALTERASE"), make_number_value(ir, SDLK_ALTERASE));
	table_put_name(ir, t, make_string_slow("KEY_SYSREQ"), make_number_value(ir, SDLK_SYSREQ));
	table_put_name(ir, t, make_string_slow("KEY_CANCEL"), make_number_value(ir, SDLK_CANCEL));
	table_put_name(ir, t, make_string_slow("KEY_CLEAR"), make_number_value(ir, SDLK_CLEAR));
	table_put_name(ir, t, make_string_slow("KEY_PRIOR"), make_number_value(ir, SDLK_PRIOR));
	table_put_name(ir, t, make_string_slow("KEY_RETURN2"), make_number_value(ir, SDLK_RETURN2));
	table_put_name(ir, t, make_string_slow("KEY_SEPARATOR"), make_number_value(ir, SDLK_SEPARATOR));
	table_put_name(ir, t, make_string_slow("KEY_OUT"), make_number_value(ir, SDLK_OUT));
	table_put_name(ir, t, make_string_slow("KEY_OPER"), make_number_value(ir, SDLK_OPER));
	table_put_name(ir, t, make_string_slow("KEY_CLEARAGAIN"), make_number_value(ir, SDLK_CLEARAGAIN));
	table_put_name(ir, t, make_string_slow("KEY_CRSEL"), make_number_value(ir, SDLK_CRSEL));
	table_put_name(ir, t, make_string_slow("KEY_EXSEL"), make_number_value(ir, SDLK_EXSEL));
	table_put_name(ir, t, make_string_slow("KEY_KP_00"), make_number_value(ir, SDLK_KP_00));
	table_put_name(ir, t, make_string_slow("KEY_KP_000"), make_number_value(ir, SDLK_KP_000));
	table_put_name(ir, t, make_string_slow("KEY_THOUSANDSSEPARATOR"), make_number_value(ir, SDLK_THOUSANDSSEPARATOR));
	table_put_name(ir, t, make_string_slow("KEY_DECIMALSEPARATOR"), make_number_value(ir, SDLK_DECIMALSEPARATOR));
	table_put_name(ir, t, make_string_slow("KEY_CURRENCYUNIT"), make_number_value(ir, SDLK_CURRENCYUNIT));
	table_put_name(ir, t, make_string_slow("KEY_CURRENCYSUBUNIT"), make_number_value(ir, SDLK_CURRENCYSUBUNIT));
	table_put_name(ir, t, make_string_slow("KEY_KP_LEFTPAREN"), make_number_value(ir, SDLK_KP_LEFTPAREN));
	table_put_name(ir, t, make_string_slow("KEY_KP_RIGHTPAREN"), make_number_value(ir, SDLK_KP_RIGHTPAREN));
	table_put_name(ir, t, make_string_slow("KEY_KP_LEFTBRACE"), make_number_value(ir, SDLK_KP_LEFTBRACE));
	table_put_name(ir, t, make_string_slow("KEY_KP_RIGHTBRACE"), make_number_value(ir, SDLK_KP_RIGHTBRACE));
	table_put_name(ir, t, make_string_slow("KEY_KP_TAB"), make_number_value(ir, SDLK_KP_TAB));
	table_put_name(ir, t, make_string_slow("KEY_KP_BACKSPACE"), make_number_value(ir, SDLK_KP_BACKSPACE));
	table_put_name(ir, t, make_string_slow("KEY_KP_A"), make_number_value(ir, SDLK_KP_A));
	table_put_name(ir, t, make_string_slow("KEY_KP_B"), make_number_value(ir, SDLK_KP_B));
	table_put_name(ir, t, make_string_slow("KEY_KP_C"), make_number_value(ir, SDLK_KP_C));
	table_put_name(ir, t, make_string_slow("KEY_KP_D"), make_number_value(ir, SDLK_KP_D));
	table_put_name(ir, t, make_string_slow("KEY_KP_E"), make_number_value(ir, SDLK_KP_E));
	table_put_name(ir, t, make_string_slow("KEY_KP_F"), make_number_value(ir, SDLK_KP_F));
	table_put_name(ir, t, make_string_slow("KEY_KP_XOR"), make_number_value(ir, SDLK_KP_XOR));
	table_put_name(ir, t, make_string_slow("KEY_KP_POWER"), make_number_value(ir, SDLK_KP_POWER));
	table_put_name(ir, t, make_string_slow("KEY_KP_PERCENT"), make_number_value(ir, SDLK_KP_PERCENT));
	table_put_name(ir, t, make_string_slow("KEY_KP_LESS"), make_number_value(ir, SDLK_KP_LESS));
	table_put_name(ir, t, make_string_slow("KEY_KP_GREATER"), make_number_value(ir, SDLK_KP_GREATER));
	table_put_name(ir, t, make_string_slow("KEY_KP_AMPERSAND"), make_number_value(ir, SDLK_KP_AMPERSAND));
	table_put_name(ir, t, make_string_slow("KEY_KP_DBLAMPERSAND"), make_number_value(ir, SDLK_KP_DBLAMPERSAND));
	table_put_name(ir, t, make_string_slow("KEY_KP_VERTICALBAR"), make_number_value(ir, SDLK_KP_VERTICALBAR));
	table_put_name(ir, t, make_string_slow("KEY_KP_DBLVERTICALBAR"), make_number_value(ir, SDLK_KP_DBLVERTICALBAR));
	table_put_name(ir, t, make_string_slow("KEY_KP_COLON"), make_number_value(ir, SDLK_KP_COLON));
	table_put_name(ir, t, make_string_slow("KEY_KP_HASH"), make_number_value(ir, SDLK_KP_HASH));
	table_put_name(ir, t, make_string_slow("KEY_KP_SPACE"), make_number_value(ir, SDLK_KP_SPACE));
	table_put_name(ir, t, make_string_slow("KEY_KP_AT"), make_number_value(ir, SDLK_KP_AT));
	table_put_name(ir, t, make_string_slow("KEY_KP_EXCLAM"), make_number_value(ir, SDLK_KP_EXCLAM));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMSTORE"), make_number_value(ir, SDLK_KP_MEMSTORE));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMRECALL"), make_number_value(ir, SDLK_KP_MEMRECALL));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMCLEAR"), make_number_value(ir, SDLK_KP_MEMCLEAR));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMADD"), make_number_value(ir, SDLK_KP_MEMADD));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMSUBTRACT"), make_number_value(ir, SDLK_KP_MEMSUBTRACT));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMMULTIPLY"), make_number_value(ir, SDLK_KP_MEMMULTIPLY));
	table_put_name(ir, t, make_string_slow("KEY_KP_MEMDIVIDE"), make_number_value(ir, SDLK_KP_MEMDIVIDE));
	table_put_name(ir, t, make_string_slow("KEY_KP_PLUSMINUS"), make_number_value(ir, SDLK_KP_PLUSMINUS));
	table_put_name(ir, t, make_string_slow("KEY_KP_CLEAR"), make_number_value(ir, SDLK_KP_CLEAR));
	table_put_name(ir, t, make_string_slow("KEY_KP_CLEARENTRY"), make_number_value(ir, SDLK_KP_CLEARENTRY));
	table_put_name(ir, t, make_string_slow("KEY_KP_BINARY"), make_number_value(ir, SDLK_KP_BINARY));
	table_put_name(ir, t, make_string_slow("KEY_KP_OCTAL"), make_number_value(ir, SDLK_KP_OCTAL));
	table_put_name(ir, t, make_string_slow("KEY_KP_DECIMAL"), make_number_value(ir, SDLK_KP_DECIMAL));
	table_put_name(ir, t, make_string_slow("KEY_KP_HEXADECIMAL"), make_number_value(ir, SDLK_KP_HEXADECIMAL));
	table_put_name(ir, t, make_string_slow("KEY_LCTRL"), make_number_value(ir, SDLK_LCTRL));
	table_put_name(ir, t, make_string_slow("KEY_LSHIFT"), make_number_value(ir, SDLK_LSHIFT));
	table_put_name(ir, t, make_string_slow("KEY_LALT"), make_number_value(ir, SDLK_LALT));
	table_put_name(ir, t, make_string_slow("KEY_LGUI"), make_number_value(ir, SDLK_LGUI));
	table_put_name(ir, t, make_string_slow("KEY_RCTRL"), make_number_value(ir, SDLK_RCTRL));
	table_put_name(ir, t, make_string_slow("KEY_RSHIFT"), make_number_value(ir, SDLK_RSHIFT));
	table_put_name(ir, t, make_string_slow("KEY_RALT"), make_number_value(ir, SDLK_RALT));
	table_put_name(ir, t, make_string_slow("KEY_RGUI"), make_number_value(ir, SDLK_RGUI));
	table_put_name(ir, t, make_string_slow("KEY_MODE"), make_number_value(ir, SDLK_MODE));
	table_put_name(ir, t, make_string_slow("KEY_AUDIONEXT"), make_number_value(ir, SDLK_AUDIONEXT));
	table_put_name(ir, t, make_string_slow("KEY_AUDIOPREV"), make_number_value(ir, SDLK_AUDIOPREV));
	table_put_name(ir, t, make_string_slow("KEY_AUDIOSTOP"), make_number_value(ir, SDLK_AUDIOSTOP));
	table_put_name(ir, t, make_string_slow("KEY_AUDIOPLAY"), make_number_value(ir, SDLK_AUDIOPLAY));
	table_put_name(ir, t, make_string_slow("KEY_AUDIOMUTE"), make_number_value(ir, SDLK_AUDIOMUTE));
	table_put_name(ir, t, make_string_slow("KEY_MEDIASELECT"), make_number_value(ir, SDLK_MEDIASELECT));
	table_put_name(ir, t, make_string_slow("KEY_WWW"), make_number_value(ir, SDLK_WWW));
	table_put_name(ir, t, make_string_slow("KEY_MAIL"), make_number_value(ir, SDLK_MAIL));
	table_put_name(ir, t, make_string_slow("KEY_CALCULATOR"), make_number_value(ir, SDLK_CALCULATOR));
	table_put_name(ir, t, make_string_slow("KEY_COMPUTER"), make_number_value(ir, SDLK_COMPUTER));
	table_put_name(ir, t, make_string_slow("KEY_AC_SEARCH"), make_number_value(ir, SDLK_AC_SEARCH));
	table_put_name(ir, t, make_string_slow("KEY_AC_HOME"), make_number_value(ir, SDLK_AC_HOME));
	table_put_name(ir, t, make_string_slow("KEY_AC_BACK"), make_number_value(ir, SDLK_AC_BACK));
	table_put_name(ir, t, make_string_slow("KEY_AC_FORWARD"), make_number_value(ir, SDLK_AC_FORWARD));
	table_put_name(ir, t, make_string_slow("KEY_AC_STOP"), make_number_value(ir, SDLK_AC_STOP));
	table_put_name(ir, t, make_string_slow("KEY_AC_REFRESH"), make_number_value(ir, SDLK_AC_REFRESH));
	table_put_name(ir, t, make_string_slow("KEY_AC_BOOKMARKS"), make_number_value(ir, SDLK_AC_BOOKMARKS));
	table_put_name(ir, t, make_string_slow("KEY_BRIGHTNESSDOWN"), make_number_value(ir, SDLK_BRIGHTNESSDOWN));
	table_put_name(ir, t, make_string_slow("KEY_BRIGHTNESSUP"), make_number_value(ir, SDLK_BRIGHTNESSUP));
	table_put_name(ir, t, make_string_slow("KEY_DISPLAYSWITCH"), make_number_value(ir, SDLK_DISPLAYSWITCH));
	table_put_name(ir, t, make_string_slow("KEY_KBDILLUMTOGGLE"), make_number_value(ir, SDLK_KBDILLUMTOGGLE));
	table_put_name(ir, t, make_string_slow("KEY_KBDILLUMDOWN"), make_number_value(ir, SDLK_KBDILLUMDOWN));
	table_put_name(ir, t, make_string_slow("KEY_KBDILLUMUP"), make_number_value(ir, SDLK_KBDILLUMUP));
	table_put_name(ir, t, make_string_slow("KEY_EJECT"), make_number_value(ir, SDLK_EJECT));
	table_put_name(ir, t, make_string_slow("KEY_SLEEP"), make_number_value(ir, SDLK_SLEEP));
	table_put_name(ir, t, make_string_slow("KEY_APP1"), make_number_value(ir, SDLK_APP1));
	table_put_name(ir, t, make_string_slow("KEY_APP2"), make_number_value(ir, SDLK_APP2));
	table_put_name(ir, t, make_string_slow("KEY_AUDIOREWIND"), make_number_value(ir, SDLK_AUDIOREWIND));
	table_put_name(ir, t, make_string_slow("KEY_AUDIOFASTFORWARD"), make_number_value(ir, SDLK_AUDIOFASTFORWARD));
}