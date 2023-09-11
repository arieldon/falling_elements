#ifndef GL_LOAD_H
#define GL_LOAD_H

/* ---
 * NOTE(ariel) This OpenGL extension loader is based on Fabian Giesen's Bink GL
 * loader and Apoorva Joshi's loader for Papaya.
 * https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
 * https://apoorvaj.io/loading-opengl-without-glew/
 * ---
 */

#if defined(__linux__)

#include <GL/glx.h>
#include <dlfcn.h>

#define DESIRED_GL_PROCEDURES_W32

#elif defined(_WIN64)

#include <GL/gl.h>

typedef char GLchar;
typedef uintptr GLsizeiptr; StaticAssert(sizeof(GLsizeiptr) == sizeof(void *));
typedef sintptr GLintptr; StaticAssert(sizeof(GLintptr) == sizeof(void *));

// NOTE(ariel) https://www.opengl.org/registry/api/GL/glext.h contains these
// definitions and more.
#define GL_ARRAY_BUFFER                   0x8892
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_COMPILE_STATUS                 0x8B81
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_LINK_STATUS                    0x8B82
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_STATIC_DRAW                    0x88E4
#define GL_STREAM_DRAW                    0x88E0
#define GL_TEXTURE0                       0x84C0
#define GL_VERTEX_SHADER                  0x8B31

#define DESIRED_GL_PROCEDURES_W32 \
	GLE(void, ActiveTexture, GLenum texture) \

#else
#error OpenGL is not loaded on this platform.
#endif

typedef void APIENTRY DEBUGPROC(
	GLenum source, GLenum Type, GLuint Id, GLenum Severity,
	GLsizei Length, const GLchar *Message, const void *UserParam);

#define DESIRED_GL_PROCEDURES \
	GLE(GLint, GetUniformLocation, GLuint program, const GLchar *name) \
	GLE(GLuint, CreateProgram, void) \
	GLE(GLuint, CreateShader, GLenum type) \
	GLE(void, AttachShader, GLuint program, GLuint shader) \
	GLE(void, BindBuffer, GLenum target, GLuint buffer) \
	GLE(void, BindVertexArray, GLuint array) \
	GLE(void, BufferData, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
	GLE(void, BufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) \
	GLE(void, CompileShader, GLuint shader) \
	GLE(void, DebugMessageCallback, DEBUGPROC callback, const void *UserParam) \
	GLE(void, DebugMessageControl, GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) \
	GLE(void, DeleteBuffers, GLsizei n, const GLuint *buffers) \
	GLE(void, DeleteProgram, GLuint program) \
	GLE(void, DeleteShader, GLuint shader) \
	GLE(void, DeleteVertexArrays, GLsizei n, const GLuint *arrays) \
	GLE(void, DrawArraysInstanced, GLenum mode, GLint first, GLsizei count, GLsizei instancecount) \
	GLE(void, EnableVertexAttribArray, GLuint index) \
	GLE(void, GenBuffers, GLsizei n, GLuint *buffers) \
	GLE(void, GenVertexArrays, GLsizei n, GLuint *arrays) \
	GLE(void, GetProgramiv, GLuint program, GLenum pname, GLint *params) \
	GLE(void, GetShaderInfoLog, GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
	GLE(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params) \
	GLE(void, LinkProgram, GLuint program) \
	GLE(void, ShaderSource, GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) \
	GLE(void, Uniform2f, GLint location, GLfloat v0, GLfloat v1) \
	GLE(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	GLE(void, UseProgram, GLuint program) \
	GLE(void, VertexAttribDivisor, GLuint index, GLuint divisor) \
	GLE(void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer) \

// NOTE(ariel) Concatenate procedure name (`Name`) with "proc" to get the name
// of the procedure in the dynamic library.
#define GLE(ReturnType, Name, ...) typedef ReturnType Name##proc(__VA_ARGS__); extern Name##proc *gl##Name;
DESIRED_GL_PROCEDURES
DESIRED_GL_PROCEDURES_W32
#undef GLE

#define GLE(ReturnType, Name, ...) Name##proc *gl##Name;
DESIRED_GL_PROCEDURES
DESIRED_GL_PROCEDURES_W32
#undef GLE

// NOTE(ariel) Read https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
// for further information.
static void
LoadOpenGLExtensions(void)
{
#if defined(__linux__)

	void *GLSharedObject = dlopen("libGL.so", RTLD_LAZY);
	AssertAlways(GLSharedObject);

#define GLE(ReturnType, Name, ...) \
	gl##Name = (Name##proc *)dlsym(GLSharedObject, "gl"#Name); \
	AssertAlways(gl##Name);
	DESIRED_GL_PROCEDURES
#undef GLE

#elif defined(_WIN64)

	HINSTANCE GLDynamicallyLinkedLibrary = LoadLibraryA("opengl32.dll");
	Assert(GLDynamicallyLinkedLibrary);

	PROC wglGetProcAddress = (PROC)GetProcAddress(GLDynamicallyLinkedLibrary, "wglGetProcAddress");
	AssertAlways(wglGetProcAddress);

#define GLE(ReturnType, Name, ...) \
	gl##Name = (Name##proc *)wglGetProcAddress("gl"#Name); \
	AssertAlways(gl##Name);
	DESIRED_GL_PROCEDURES
	DESIRED_GL_PROCEDURES_W32
#undef GLE

#endif
}

#endif
