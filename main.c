#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ml.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_test.h>
#include <SDL2/SDL_opengles2.h>
#include <SDL2/SDL_mixer.h>
#include "plat.h"

#ifdef GCW0
const int window_w = 320;
const int window_h = 240;
#else
const int window_w = 320 * 2;
const int window_h = 240 * 2;
#endif

#define SIZE (5 * 4)

struct attrib {
	GLuint program;
	GLuint position;
	GLuint normal;
	GLuint matrix;
	GLuint uv;
	GLuint sampler;
	GLuint camera;
	GLuint timer;
	GLuint extra1;
	GLuint extra2;
	GLuint extra3;
	GLuint extra4;

	GLuint color;
	m4f mvp;
	v3f angle;
	v3f scale;
	v3f translate;
};

char *load_file(const char *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    rewind(file);
    char *data = calloc(length + 1, sizeof(char));
    fread(data, 1, length, file);
    fclose(file);
    return data;
}

GLuint make_shader(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		GLchar *info = calloc(length, sizeof(GLchar));
		glGetShaderInfoLog(shader, length, NULL, info);
		fprintf(stderr, "glCompileShader failed:\n%s\n", info);
		free(info);
	}
	return shader;
}

GLuint load_shader(const char *path, GLenum type) {
	char *data = load_file(path);
	GLuint result = make_shader(type, data);
	free(data);
	return result;
}

GLuint make_program(GLuint vert_shader, GLuint frag_shader) {
	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		GLchar *info = calloc(length, sizeof(GLchar));
		glGetProgramInfoLog(program, length, NULL, info);
		fprintf(stderr, "glLinkProgram failed: %s\n", info);
		free(info);
	}
	glDetachShader(program, vert_shader);
	glDetachShader(program, frag_shader);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	return program;
}

GLuint load_program(const char *vert_path, const char *frag_path) {
	GLuint vert = load_shader(vert_path, GL_VERTEX_SHADER);
	GLuint frag = load_shader(frag_path, GL_FRAGMENT_SHADER);
	return make_program(vert, frag);
}

void draw_cube(struct attrib *attr, GLuint buf);

static void quit(int rc, SDL_GLContext *cxt) {
	SDL_GL_DeleteContext(*cxt);
	exit(rc);
}

/* 3D data. Vertex range -0.5..0.5 in all axes.
* Z -0.5 is near, 0.5 is far. */
const float _vertices[] = {
	/* Front face. */
	/* Bottom left */
	-0.5,  0.5, -0.5,
	0.5, -0.5, -0.5,
	-0.5, -0.5, -0.5,
	/* Top right */
	-0.5,  0.5, -0.5,
	0.5,  0.5, -0.5,
	0.5, -0.5, -0.5,
	/* Left face */
	/* Bottom left */
	-0.5,  0.5,  0.5,
	-0.5, -0.5, -0.5,
	-0.5, -0.5,  0.5,
	/* Top right */
	-0.5,  0.5,  0.5,
	-0.5,  0.5, -0.5,
	-0.5, -0.5, -0.5,
	/* Top face */
	/* Bottom left */
	-0.5,  0.5,  0.5,
	0.5,  0.5, -0.5,
	-0.5,  0.5, -0.5,
	/* Top right */
	-0.5,  0.5,  0.5,
	0.5,  0.5,  0.5,
	0.5,  0.5, -0.5,
	/* Right face */
	/* Bottom left */
	0.5,  0.5, -0.5,
	0.5, -0.5,  0.5,
	0.5, -0.5, -0.5,
	/* Top right */
	0.5,  0.5, -0.5,
	0.5,  0.5,  0.5,
	0.5, -0.5,  0.5,
	/* Back face */
	/* Bottom left */
	0.5,  0.5,  0.5,
	-0.5, -0.5,  0.5,
	0.5, -0.5,  0.5,
	/* Top right */
	0.5,  0.5,  0.5,
	-0.5,  0.5,  0.5,
	-0.5, -0.5,  0.5,
	/* Bottom face */
	/* Bottom left */
	-0.5, -0.5, -0.5,
	0.5, -0.5,  0.5,
	-0.5, -0.5,  0.5,
	/* Top right */
	-0.5, -0.5, -0.5,
	0.5, -0.5, -0.5,
	0.5, -0.5,  0.5,
};

const float _colors[] = {
	// Front face -- Red
	// Bottom left
	1.0, 0.0, 0.0, // red
	0.6, 0.0, 0.4, // blue
	0.6, 0.4, 0.0, // green
	// Top right 
	1.0, 0.0, 0.0, // red
	0.6, 0.0, 0.0, // black
	0.6, 0.0, 0.4, // blue
	// Left face  -- Blue
	// Bottom left
	0.4, 0.0, 0.6, // red
	0.0, 0.0, 1.0, // blue
	0.0, 0.4, 0.6, // green
	// Top right 
	0.4, 0.0, 0.6, // red
	0.0, 0.0, 0.6, // black
	0.0, 0.0, 1.0, // blue
	// Top face -- Green
	// Bottom left
	0.4, 0.6, 0.0, // red
	0.0, 0.6, 0.4, // blue
	0.0, 1.0, 0.0, // green
	// Top right 
	0.4, 0.6, 0.0, // red
	0.0, 0.6, 0.0, // black
	0.0, 0.6, 0.4, // blue
	// Right face -- Yellow
	// Bottom left
	1.0, 0.6, 0.0, // red
	0.6, 0.6, 0.4, // blue
	0.6, 1.0, 0.0, // green
	// Top right 
	1.0, 0.6, 0.0, // red
	0.6, 0.6, 0.0, // black
	0.6, 0.6, 0.4, // blue
	// Back face -- Cyan
	// Bottom left
	0.4, 0.6, 0.6, // red
	0.0, 0.6, 1.0, // blue
	0.0, 1.0, 0.6, // green
	// Top right 
	0.4, 0.6, 0.6, // red
	0.0, 0.6, 0.6, // black
	0.0, 0.6, 1.0, // blue
	// Bottom face -- Magenta
	// Bottom left
	1.0, 0.0, 0.6, // red
	0.6, 0.0, 1.0, // blue
	0.6, 0.4, 0.6, // green
	// Top right 
	1.0, 0.0, 0.6, // red
	0.6, 0.0, 0.6, // black
	0.6, 0.0, 1.0, // blue
};

void update_mvp(const m4f persp, struct attrib *attrib) {
	const m4f scale = scalem4f(eyem4f(), attrib->scale);
	m4f rotate;
	rotate = mulm4f(rotm4f(attrib->angle.x, _v3f(1,0,0)), eyem4f());
	rotate = mulm4f(rotm4f(attrib->angle.y, _v3f(0,1,0)), rotate);
	rotate = mulm4f(rotm4f(attrib->angle.z, _v3f(0,0,1)), rotate);
	const m4f translate = translatem4f(attrib->translate);
	const m4f mv = addm4f(mulm4f(rotate, scale), translate);
	attrib->mvp = mulm4f(persp, mv);
}

static void step(struct attrib *attribs) {
	for (int i = 0; i < SIZE; i++) {
		struct attrib *attrib = attribs + i;
		attrib->scale = _v3f(1,1,1);
		const float t = 0.1;
		const float s = t * (float)i + t;
		attrib->angle = addv3f(attrib->angle, _v3f(0.03 * s, 0.02 * s, 0.01 * s));
		attrib->translate = _v3f(
			-3 + (float)(i % 5) * 1.5,
			-2 + (float)(i / 5) * 1.5, 
			-6  - (i % 2 ? 2 : 0)
		);
	}
}

static void render(unsigned int width, unsigned int height,
		   struct attrib* attribs) {
	const m4f persp = perspm4f(45, (float)width / (float)height, 0.01, 100);

	glClearColor(0.2, 0.2, 0.2, 1);
	glClear(GL_COLOR_BUFFER_BIT |
		GL_DEPTH_BUFFER_BIT |
		GL_STENCIL_BUFFER_BIT);
	for (int i = 0; i < SIZE; i++) {
		struct attrib *attrib = attribs + i;
		update_mvp(persp, attrib);
		glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE,
				   attrib->mvp.val);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

SDL_Window *window;
int done;

void loop(SDL_GLContext *cxt, struct attrib *attribs) {
	SDL_Event event;
	int i;
	int status;
	// Check for events 
	while (SDL_PollEvent(&event) && !done) {
		if (event.type == SDL_KEYDOWN &&
		    event.key.keysym.sym == SDLK_ESCAPE) {
		    done = true;
		}
	}
	if (!done) {
		status = SDL_GL_MakeCurrent(window, *cxt);
		if (status) {
			SDL_Log("SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
			// Continue for next window
		}
		step(attribs);
		render(window_w, window_h, attribs);
		SDL_GL_SwapWindow(window);
	}
	SDL_Delay(16);
}

void init_gl_settings() {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        //SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 1);;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        //SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			    SDL_GL_CONTEXT_PROFILE_ES);
}

void init_sdl() {
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Couldn't initialize audio: %s\n",
			SDL_GetError());
		exit(EXIT_FAILURE);
	}
        if (SDL_VideoInit(NULL) < 0) {
		fprintf(stderr, "Couldn't initialize video driver: %s\n",
			SDL_GetError());
		exit(EXIT_FAILURE);
        }
	if (Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1) {
		exit(EXIT_FAILURE);
        }
}

SDL_Window* create_window() {
	return SDL_CreateWindow(
		"plat",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		window_w, window_h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS
	);
}

void make_context(SDL_GLContext *cxt) {
	// Create OpenGL ES contexts 
	*cxt = SDL_GL_CreateContext(window);
	if (!*cxt) {
	    SDL_Log("SDL_GL_CreateContext(): %s\n", SDL_GetError());
	    quit(2, cxt);
	}
	// VSYNC
	SDL_GL_SetSwapInterval(1); // VSYNC
	int status = SDL_GL_MakeCurrent(window, *cxt);
	if (status) {
		SDL_Log("SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
		// Continue for next window
	}
}

void setup_gl_viewport(SDL_Window *window) {
	int w, h;
	SDL_GL_GetDrawableSize(window, &w, &h);
	glViewport(0, 0, w, h);
}

void setup_attrib(GLuint program, struct attrib *attrib) {
	// Setup shaders and bind locations
	attrib->angle = zerov3f();
	attrib->program = program;
	// Get attribute locations of non-fixed attributes like color
	// and texture coordinates.
	attrib->position = glGetAttribLocation(attrib->program, "av4position");
	attrib->color = glGetAttribLocation(attrib->program, "av3color");
	// Get uniform locations
	attrib->matrix = glGetUniformLocation(attrib->program, "mvp");
	glUseProgram(attrib->program);
	// Enable attributes for position, color and texture coordinates etc.
	glEnableVertexAttribArray(attrib->position);
	glEnableVertexAttribArray(attrib->color);
	//
	glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, 0,
			      _vertices);
	glVertexAttribPointer(attrib->color, 3, GL_FLOAT, GL_FALSE, 0, _colors);
}



int main(int argc, char *argv[]) {
	SDL_GLContext context;
	struct attrib attribs[SIZE];

	init_sdl();
	init_gl_settings();
	window = create_window();
	Mix_Music *music = Mix_LoadMUS("res/di2.xm");
	if (!music) return EXIT_FAILURE;
	Mix_PlayMusic(music, -1);
	// Mix_PauseMusic();
	make_context(&context);
	setup_gl_viewport(window);
	GLuint program = load_program("res/shader/cube.vert",
				      "res/shader/cube.frag");
	for (int i = 0; i < SIZE; i++) {
		setup_attrib(program, &attribs[i]);
	}
	// enable filters
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	// Initialize SDL_mixer 
	for (done = 0; !done;) {
		loop(&context, attribs);
	}
	// Cleanup
	Mix_FreeMusic(music);
	Mix_CloseAudio();
	quit(EXIT_SUCCESS, &context);
	return EXIT_SUCCESS;
}

void draw_triangles_3d_ao(const struct attrib *attr, GLuint buf, int count) {
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glEnableVertexAttribArray(attr->position);
	glEnableVertexAttribArray(attr->normal);
	glEnableVertexAttribArray(attr->uv);
	glVertexAttribPointer(attr->position, 3, GL_FLOAT, GL_FALSE,
		sizeof(GLfloat) * 10, 0);
	glVertexAttribPointer(attr->normal, 3, GL_FLOAT, GL_FALSE,
		sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 3));
	glVertexAttribPointer(attr->uv, 4, GL_FLOAT, GL_FALSE,
		sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 6));
	glDrawArrays(GL_TRIANGLES, 0, count);
	glDisableVertexAttribArray(attr->position);
	glDisableVertexAttribArray(attr->normal);
	glDisableVertexAttribArray(attr->uv);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_cube(struct attrib *attr, GLuint buf) {
	draw_triangles_3d_ao(attr, buf, 36);
}

void render_item(struct attrib *attrib) {
    float mvp[16];
    //set_matrix_item(matrix, g->width, g->height, g->scale);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, mvp);
    glUniform3f(attrib->camera, 0, 0, 5);
    glUniform1i(attrib->sampler, 0);
    //glUniform1f(attrib->timer, time_of_day());

/*
    int w = items[g->item_index];
    if (is_plant(w)) {
        GLuint buffer = gen_plant_buffer(0, 0, 0, 0.5, w);
        draw_plant(attrib, buffer);
        del_buffer(buffer);
    }
    else {
        GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
        draw_cube(attrib, buffer);
        del_buffer(buffer);
    }
*/
}

m4f set_matrix_item(int width, int height, int scale) {
	const float aspect = (float)width / height;
	const float size = 64 * scale;
	const float box = height / size / 2;
	const float xoffset = 1 - size / width * 2;
	const float yoffset = 1 - size / height * 2;
	m4f a = eyem4f();
	m4f b = rotm4f(0, _v3f(1, 0, -M_PI / 4));
	a = mulm4f(a, b);
	b = rotm4f(1, _v3f(0, 0, -M_PI / 9));
	a = mulm4f(a, b);
	b = orthom4f(-box * aspect, box * aspect, -box, box, -1, 1);
	// b = perspm4f(45, (float)width / (float)height, 0.01, 100);
	a = mulm4f(b, a);
	b = translatem4f(_v3f(-xoffset, -yoffset, 0));
	a = mulm4f(b, a);
	return mulm4f(eyem4f(), a);
}
